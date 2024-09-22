#pragma once

#include <memory>

#include <QMutex>
#include <QSet>
#include <QThread>
#include <unistd.h>

#include "winemonitor.h"

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;
QT_END_NAMESPACE

class WineMonitorLinux : public WineMonitor
{
    Q_OBJECT

public:
    explicit WineMonitorLinux(QObject *parent = nullptr);
    ~WineMonitorLinux() override;

    WineMonitorLinux(WineMonitorLinux &) = delete;
    WineMonitorLinux(WineMonitorLinux &&) = delete;
    auto operator=(WineMonitorLinux &) -> WineMonitorLinux = delete;
    auto operator=(WineMonitorLinux &&) -> WineMonitorLinux = delete;

    void start() override;

private:
    void checkWineserverDirectories();
    void checkWineserverDirectory(const QString &serverPath);
    void addWineserverProcessToEpoll(pid_t pid);
    Q_SLOT void directoryChanged(const QT_PREPEND_NAMESPACE(QString) & path);
    Q_SLOT void fileChanged(const QT_PREPEND_NAMESPACE(QString) & path);

    struct PidfdPair
    {
        pid_t pid;
        int fd;
    };

    void epollThread();
    void addPid(pid_t pid);
    auto removePid(pid_t pid) -> qsizetype;
    [[nodiscard]] auto havePid(pid_t pid) -> bool;

    QT_PREPEND_NAMESPACE(QFileSystemWatcher) *serverWatcher_ = nullptr;
    QT_PREPEND_NAMESPACE(QString) serverPrefix_;
    int epollFd_ = -1;

    QT_PREPEND_NAMESPACE(QSet)<pid_t> wineservers_;
    QT_PREPEND_NAMESPACE(QMutex) wineserversMutex_;
    std::unique_ptr<QT_PREPEND_NAMESPACE(QThread)> epollThread_;
};
