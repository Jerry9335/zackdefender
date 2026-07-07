#pragma once
#include <QObject>

/// Manages Windows Explorer right-click context menu entries for Zack Defender.
/// Uses HKCU\Software\Classes (per-user, no admin required).
class ContextMenuManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool registered READ isRegistered NOTIFY registeredChanged)

public:
    explicit ContextMenuManager(QObject *parent = nullptr);

    bool isRegistered() const { return m_registered; }

    /// Toggle: Switch binding calls this
    Q_INVOKABLE void setRegistered(bool reg);

    /// Force register / unregister
    Q_INVOKABLE void registerMenu();
    Q_INVOKABLE void unregisterMenu();

    /// Check current registry state (called on construction + after operations)
    void refresh();

signals:
    void registeredChanged();

private:
    QString exePath() const;
    void writeKey(const QString &classPath, const QString &exe);
    void deleteKey(const QString &classPath);

    bool m_registered = false;
};
