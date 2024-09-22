#include <QWidget>

#include "maindialog.h"
#include "winemanager.h"
#include "winemonitor.h"
#include "wineserverlist.h"

QT_USE_NAMESPACE

constexpr QStringView kShouldNotifyOnStartKey = u"shouldNotifyOnStart";
constexpr QStringView kShouldNotifyOnStopKey = u"shouldNotifyOnStop";
constexpr QStringView kShouldAlwaysShowKey = u"shouldAlwaysShow";

WineManager::WineManager(QObject *parent)
    : QObject(parent)
    , shouldNotifyOnStart_ { settings_.value(kShouldNotifyOnStartKey, true).toBool() }
    , shouldNotifyOnStop_ { settings_.value(kShouldNotifyOnStopKey, false).toBool() }
    , shouldAlwaysShow_ { settings_.value(kShouldAlwaysShowKey, false).toBool() }
    , wineMonitor_ { WineMonitor::create(this) }
    , listModel_ { new WineServerListModel(this) }
    , mainDialog_ { new MainDialog(this) }
{
    trayIcon_.setIcon(QIcon::fromTheme("wine"));
    QObject::connect(wineMonitor_, &WineMonitor::initialized, this, &WineManager::monitorInitialized);
    QObject::connect(wineMonitor_, &WineMonitor::serverRunning, this, &WineManager::serverRunning);
    QObject::connect(wineMonitor_, &WineMonitor::serverStopped, this, &WineManager::serverStopped);
    QObject::connect(wineMonitor_, &WineMonitor::serverRunning, listModel_, &WineServerListModel::serverRunning);
    QObject::connect(wineMonitor_, &WineMonitor::serverStopped, listModel_, &WineServerListModel::serverStopped);
    QObject::connect(&trayIcon_, &QSystemTrayIcon::activated, this, &WineManager::invoke);
    wineMonitor_->start();
}

WineManager::~WineManager() = default;

void WineManager::monitorInitialized()
{
    monitorInitialized_ = true;
}

void WineManager::serverRunning(pid_t pid)
{
    trayIcon_.setVisible(true);
    if (!monitorInitialized_) {
        return;
    }
    if (shouldNotifyOnStart_) {
        trayIcon_.showMessage("Wine Server Started", QString("Wine server started with PID %1").arg(pid));
    }
}

void WineManager::serverStopped(pid_t pid, bool lastServer)
{
    if (shouldNotifyOnStop_) {
        trayIcon_.showMessage("Wine Server Stopped", QString("Wine server (PID %1) has stopped").arg(pid));
    }
    if (!shouldAlwaysShow_) {
        // Even with quitOnLastWindowClosed set to false, at least with the
        // DBus tray icon implementation, hiding the tray icon appears to
        // quit the application, but we want to continue running.
        QWidget widget;
        widget.setVisible(true);
        trayIcon_.setVisible(!lastServer);
    }
}

auto WineManager::listModel() const -> WineServerListModel *
{
    return listModel_;
}

auto WineManager::shouldNotifyOnStart() const -> bool
{
    return shouldNotifyOnStart_;
}

void WineManager::setShouldNotifyOnStart(bool value)
{
    shouldNotifyOnStart_ = value;
    settings_.setValue(kShouldNotifyOnStartKey, value);
    settings_.sync();
}

auto WineManager::shouldNotifyOnStop() const -> bool
{
    return shouldNotifyOnStop_;
}

void WineManager::setShouldNotifyOnStop(bool value)
{
    shouldNotifyOnStop_ = value;
    settings_.setValue(kShouldNotifyOnStopKey, value);
    settings_.sync();
}

auto WineManager::shouldAlwaysShow() const -> bool
{
    return shouldAlwaysShow_;
}

void WineManager::setShouldAlwaysShow(bool value)
{
    shouldAlwaysShow_ = value;
    settings_.setValue(kShouldAlwaysShowKey, value);
    settings_.sync();

    if (value) {
        if (!trayIcon_.isVisible()) {
            trayIcon_.show();
        }
    } else {
        if (trayIcon_.isVisible() && listModel_->rowCount() == 0) {
            trayIcon_.hide();
        }
    }
}

void WineManager::invoke()
{
    if (mainDialog_->isVisible()) {
        mainDialog_->activateWindow();
        return;
    }

    mainDialog_->show();
}
