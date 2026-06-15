#pragma once
#include <QObject>

/// Detects first run after update: compares stored version with current version
class UpdateManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool showChangelog READ showChangelog CONSTANT)

public:
    explicit UpdateManager(QObject *parent = nullptr);

    bool showChangelog() const { return m_show; }

    Q_INVOKABLE void dismiss();   // mark current version as seen

private:
    bool m_show = false;
};
