#pragma once

#include <QObject>
#include <QPointer>
#include <QScopedPointer>
#include <QSettings>
#include <QSystemTrayIcon>

class MainDialog;
class WineMonitor;
class WineServerListModel;

/**
 * Class that monitors running wineserver instances.
 */
class WineManager : public QT_PREPEND_NAMESPACE(QObject)
{
    Q_OBJECT

public:
    explicit WineManager(QT_PREPEND_NAMESPACE(QObject) *parent = nullptr);
    ~WineManager() override;

    WineManager(WineManager &) = delete;
    WineManager(WineManager &&) = delete;
    auto operator=(WineManager &) -> WineManager = delete;
    auto operator=(WineManager &&) -> WineManager = delete;

    [[nodiscard]] auto listModel() const -> WineServerListModel *;

    [[nodiscard]] auto shouldNotifyOnStart() const -> bool;
    Q_SLOT void setShouldNotifyOnStart(bool value);

    [[nodiscard]] auto shouldNotifyOnStop() const -> bool;
    Q_SLOT void setShouldNotifyOnStop(bool value);

    [[nodiscard]] auto shouldAlwaysShow() const -> bool;
    Q_SLOT void setShouldAlwaysShow(bool value);

    Q_SLOT void invoke();

private:
    Q_SLOT void monitorInitialized();
    Q_SLOT void serverRunning(pid_t pid);
    Q_SLOT void serverStopped(pid_t pid, bool lastServer);

    bool monitorInitialized_ {};

    QT_PREPEND_NAMESPACE(QSettings) settings_;
    bool shouldNotifyOnStart_;
    bool shouldNotifyOnStop_;
    bool shouldAlwaysShow_;

    QT_PREPEND_NAMESPACE(QPointer)<WineMonitor> wineMonitor_;
    QT_PREPEND_NAMESPACE(QPointer)<WineServerListModel> listModel_;
    QT_PREPEND_NAMESPACE(QScopedPointer)<MainDialog> mainDialog_;
    QT_PREPEND_NAMESPACE(QSystemTrayIcon) trayIcon_;
};
