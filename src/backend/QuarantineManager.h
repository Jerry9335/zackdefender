#pragma once
#include <QObject>
#include <QStringList>
#include <QDateTime>
#include <QVariantList>
#include <QProcess>

struct QuarantineItem {
    Q_GADGET
    Q_PROPERTY(QString threatName MEMBER threatName)
    Q_PROPERTY(QString filePath MEMBER filePath)
    Q_PROPERTY(QDateTime detectionTime MEMBER detectionTime)
    Q_PROPERTY(QString action MEMBER action)
    Q_PROPERTY(QString source MEMBER source)
public:
    QString threatName;
    QString filePath;
    QDateTime detectionTime;
    QString action;
    QString source;
};

/// Manages quarantined files — both local and Windows Defender (async)
class QuarantineManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList items READ items NOTIFY quarantineChanged)
    Q_PROPERTY(int itemCount READ itemCount NOTIFY quarantineChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)

public:
    explicit QuarantineManager(QObject *parent = nullptr);
    ~QuarantineManager() override;

    QVariantList items() const;
    int itemCount() const { return m_items.size(); }
    bool isLoading() const { return m_loading; }

    static QString quarantinePath();

public slots:
    void refresh();
    void quarantineFile(const QString &filePath, const QString &threatName = {});
    void restoreFile(const QString &filePath);
    void deleteFile(const QString &filePath);
    void emptyQuarantine();

signals:
    void quarantineChanged();
    void fileRestored(const QString &originalPath);
    void fileDeleted(const QString &filePath);
    void quarantineEmptied();
    void loadingChanged();

private slots:
    void onDefenderQueryFinished(int exitCode);

private:
    void queryLocalQuarantine();
    void ensureQuarantineDir();

    QList<QuarantineItem> m_items;
    bool m_loading = false;
    QProcess *m_process = nullptr;
};
