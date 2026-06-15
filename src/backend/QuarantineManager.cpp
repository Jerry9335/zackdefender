#include "QuarantineManager.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

QuarantineManager::QuarantineManager(QObject *parent)
    : QObject(parent)
{
}

QuarantineManager::~QuarantineManager()
{
    if (m_process) {
        m_process->kill();
        m_process->deleteLater();
    }
}

QVariantList QuarantineManager::items() const
{
    QVariantList result;
    for (const auto &item : m_items) {
        QVariantMap map;
        map["threatName"] = item.threatName;
        map["filePath"] = item.filePath;
        map["detectionTime"] = item.detectionTime;
        map["action"] = item.action;
        map["source"] = item.source;
        result.append(QVariant::fromValue(map));
    }
    return result;
}

QString QuarantineManager::quarantinePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
           + "/ZackDefender/quarantine";
}

void QuarantineManager::ensureQuarantineDir()
{
    QDir().mkpath(quarantinePath());
}

void QuarantineManager::refresh()
{
    // Local query is fast — do it sync
    m_items.clear();
    queryLocalQuarantine();

    // Defender query is async
    if (m_process) {
        m_process->disconnect();
        m_process->deleteLater();
    }

    m_loading = true;
    emit loadingChanged();

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &QuarantineManager::onDefenderQueryFinished);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        m_loading = false;
        emit loadingChanged();
        if (m_process) { m_process->deleteLater(); m_process = nullptr; }
    });

    m_process->start("powershell", {
        "-NoProfile", "-Command",
        "Get-MpThreatDetection | "
        "Where-Object { $_.ActionTaken -in @(2,3,5,6) } | "
        "Sort-Object DetectionTime -Descending | "
        "Select-Object -First 100 | "
        "ForEach-Object { "
        "  [PSCustomObject]@{ "
        "    ThreatName = $_.ThreatName; "
        "    Resources = ($_.Resources -join ';'); "
        "    ActionTaken = $_.ActionTaken; "
        "    DetectionTime = $_.DetectionTime.ToString('o') "
        "  } "
        "} | ConvertTo-Json"
    });
}

void QuarantineManager::onDefenderQueryFinished(int exitCode)
{
    Q_UNUSED(exitCode)
    m_loading = false;
    emit loadingChanged();

    if (!m_process) return;

    QByteArray data = m_process->readAllStandardOutput();
    m_process->deleteLater();
    m_process = nullptr;

    if (data.trimmed().isEmpty()) {
        emit quarantineChanged();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(data);

    auto actionToString = [](int action) -> QString {
        switch (action) {
            case 2: return QStringLiteral("已隔离");
            case 3: return QStringLiteral("已删除");
            case 5: return QStringLiteral("已清理");
            case 6: return QStringLiteral("已阻止");
            default: return QString::number(action);
        }
    };

    auto parseArray = [&](const QJsonArray &arr) {
        for (const auto &val : arr) {
            QJsonObject obj = val.toObject();
            QuarantineItem item;
            item.threatName = obj["ThreatName"].toString();
            item.filePath = obj["Resources"].toString();
            item.detectionTime = QDateTime::fromString(obj["DetectionTime"].toString(), Qt::ISODate);
            item.action = actionToString(obj["ActionTaken"].toInt());
            item.source = "defender";
            m_items.append(item);
        }
    };

    if (doc.isArray()) {
        parseArray(doc.array());
    } else if (doc.isObject() && !doc.object().isEmpty()) {
        QJsonArray arr;
        arr.append(doc.object());
        parseArray(arr);
    }

    emit quarantineChanged();
}

void QuarantineManager::queryLocalQuarantine()
{
    ensureQuarantineDir();
    QDir dir(quarantinePath());
    QStringList files = dir.entryList({"*"}, QDir::Files | QDir::NoDotAndDotDot);

    for (const auto &f : files) {
        QuarantineItem item;
        item.filePath = dir.absoluteFilePath(f);
        item.threatName = f;
        item.action = QStringLiteral("已隔离");
        item.source = "local";
        item.detectionTime = QFileInfo(dir.absoluteFilePath(f)).birthTime();
        m_items.append(item);
    }
}

void QuarantineManager::quarantineFile(const QString &filePath, const QString &threatName)
{
    ensureQuarantineDir();

    QString fileName = QFileInfo(filePath).fileName();
    QString dest = quarantinePath() + "/" + fileName;

    if (QFile::exists(dest)) {
        QString base = QFileInfo(fileName).completeBaseName();
        QString ext = QFileInfo(fileName).suffix();
        int counter = 1;
        do {
            dest = quarantinePath() + "/" + base + "_" + QString::number(counter);
            if (!ext.isEmpty()) dest += "." + ext;
            counter++;
        } while (QFile::exists(dest));
    }

    if (QFile::copy(filePath, dest)) {
        QFile::remove(filePath);
        QuarantineItem item;
        item.filePath = dest;
        item.threatName = threatName.isEmpty() ? fileName : threatName;
        item.action = QStringLiteral("已隔离");
        item.source = "local";
        item.detectionTime = QDateTime::currentDateTime();
        m_items.prepend(item);
        emit quarantineChanged();
    }
}

void QuarantineManager::restoreFile(const QString &filePath)
{
    QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString fileName = QFileInfo(filePath).fileName();
    QString dest = desktop + "/" + fileName;

    // Handle duplicate filenames on desktop
    if (QFile::exists(dest)) {
        QString base = QFileInfo(fileName).completeBaseName();
        QString ext = QFileInfo(fileName).suffix();
        int counter = 1;
        do {
            dest = desktop + "/" + base + "_" + QString::number(counter);
            if (!ext.isEmpty()) dest += "." + ext;
            counter++;
        } while (QFile::exists(dest));
    }

    if (QFile::copy(filePath, dest)) {
        QFile::remove(filePath);
        emit fileRestored(dest);
        refresh();
    }
}

void QuarantineManager::deleteFile(const QString &filePath)
{
    if (QFile::remove(filePath)) {
        emit fileDeleted(filePath);
        refresh();
    }
}

void QuarantineManager::emptyQuarantine()
{
    ensureQuarantineDir();
    QDir dir(quarantinePath());
    for (const auto &f : dir.entryList(QDir::Files | QDir::NoDotAndDotDot))
        dir.remove(f);

    m_items.erase(std::remove_if(m_items.begin(), m_items.end(),
        [](const QuarantineItem &item) { return item.source == "local"; }),
        m_items.end());

    emit quarantineEmptied();
    emit quarantineChanged();
}
