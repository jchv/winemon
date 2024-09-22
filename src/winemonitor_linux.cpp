#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include <QDir>
#include <QFileSystemWatcher>

#include "winemonitor_linux.h"

QT_USE_NAMESPACE

constexpr QStringView kWineServerPrefixFormat = u"/tmp/.wine-%1";
constexpr int kEpollSize = 0x1000;

namespace {

auto pidfd_open(pid_t pid, unsigned int flags) -> int
{
    static constexpr int kSyscallPidfdOpen = 434;
    return static_cast<int>(syscall(kSyscallPidfdOpen, pid, flags)); // NOLINT(cppcoreguidelines-pro-type-vararg)
}

auto getWineserverPid(QStringView socketPath) -> pid_t
{
    socklen_t len {};
    int sock {};

    struct sockaddr_un addr = {};
    struct ucred ucred = {};

    QByteArray socketPathUtf8 = socketPath.toUtf8();

    if (socketPathUtf8.size() > sizeof(addr.sun_path) - 1) {
        qWarning("Path is too long for UNIX socket: %s", socketPathUtf8.constData());
        return -1;
    }

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        qWarning("Unable to create UNIX socket (errno=%d)", errno);
        return -1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(static_cast<char *>(addr.sun_path), socketPathUtf8.data(), sizeof(addr.sun_path) - 1);

    len = sizeof(struct ucred);

    if (connect(sock, reinterpret_cast<struct sockaddr *>(&addr), sizeof(struct sockaddr_un)) == -1) { // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        qDebug("Failed to connect to wineserver socket (errno=%d)", errno);
        if (errno == ECONNREFUSED) {
            // The socket is disconnected, so unlink it.
            // That way we'll get notified when a new wineserver re-creates it.
            unlink(socketPathUtf8.data());
        }
        return -1;
    }

    if (getsockopt(sock, SOL_SOCKET, SO_PEERCRED, &ucred, &len) == -1) {
        qDebug("Failed to acquire wineserver socket peer credentials (errno=%d)", errno);
        return -1;
    }

    // NOTE: This closes the socket before the pidfd is created.
    // However, holding the socket open keeps wineserver from quitting.
    // Thus, it may be nicer if we could avoid doing that.
    close(sock);

    return ucred.pid;
}

}

WineMonitorLinux::WineMonitorLinux(QObject *parent) : WineMonitor(parent)
{
    serverPrefix_ = kWineServerPrefixFormat.arg(QString::number(getuid()));
    if (!QDir { serverPrefix_ }.exists()) {
        qInfo("Creating wine server directory at %s", qPrintable(serverPrefix_));
        QDir {}.mkdir(serverPrefix_, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    }
}

WineMonitorLinux::~WineMonitorLinux()
{
    if (epollFd_ != -1) {
        struct epoll_event closeEvent = {};
        closeEvent.events = EPOLLIN;
        closeEvent.data.ptr = nullptr;
        int evfd = eventfd(1, 0);
        if (evfd == -1) {
            qWarning("Unexpected error trying to create close evenfd (errno=%d)", errno);
        } else if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, evfd, &closeEvent) == -1) {
            qWarning("Unexpected error trying to add close eventfd (fd=%d) to epoll (fd=%d, errno=%d)", evfd, epollFd_, errno);
        }
    }

    if (epollThread_) {
        epollThread_->wait();
    }
}

void WineMonitorLinux::start()
{
    if (epollThread_) {
        return;
    }

    serverWatcher_ = { new QFileSystemWatcher(this) };

    QObject::connect(serverWatcher_, &QFileSystemWatcher::directoryChanged, this, &WineMonitorLinux::directoryChanged);
    QObject::connect(serverWatcher_, &QFileSystemWatcher::fileChanged, this, &WineMonitorLinux::fileChanged);

    serverWatcher_->addPath(serverPrefix_);

    epollFd_ = epoll_create(kEpollSize);

    checkWineserverDirectories();
    emit initialized();

    epollThread_.reset(QThread::create([&] { epollThread(); }));
    epollThread_->start();
}

void WineMonitorLinux::checkWineserverDirectories()
{
    QDir wineserverParentDirectory { serverPrefix_ };

    for (const QString &filePath :
         wineserverParentDirectory.entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::Hidden)) {
        checkWineserverDirectory(wineserverParentDirectory.absoluteFilePath(filePath));
    }
}

void WineMonitorLinux::checkWineserverDirectory(const QString &serverPath)
{
    QDir wineserverDirectory { serverPath };
    if (!wineserverDirectory.exists()) {
        return;
    }

    serverWatcher_->addPath(serverPath);

    QFile socketFile { wineserverDirectory.absoluteFilePath("socket") };
    if (!socketFile.exists()) {
        return;
    }

    pid_t wineserverPid = getWineserverPid(socketFile.fileName());
    if (wineserverPid < 0) {
        return;
    }
    addWineserverProcessToEpoll(wineserverPid);
}

void WineMonitorLinux::addWineserverProcessToEpoll(pid_t pid)
{
    struct sockaddr_un addr = {};
    struct ucred ucred = {};

    if (havePid(pid)) {
        return;
    }

    int pidfd = pidfd_open(pid, 0);

    struct epoll_event pidEvent = {};
    pidEvent.events = EPOLLIN;
    pidEvent.data.ptr = reinterpret_cast<void *>(new PidfdPair { .pid = pid, .fd = pidfd }); // NOLINT

    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, pidfd, &pidEvent) == -1) {
        qWarning("Unexpected error trying to watch wineserver process pid=%d (errno=%d)", pid, errno);
        return;
    }

    qDebug("Watching wineserver process pid=%d", pid);

    addPid(pid);
    emit serverRunning(pid);
}

void WineMonitorLinux::epollThread()
{
    while (true) {
        struct epoll_event event = {};

        int timeout = -1;

        errno = 0;
        if (epoll_wait(epollFd_, &event, 1, timeout) < 1) {
            if (errno == EINTR) {
                continue;
            }
            if (errno != 0) {
                return;
            }
        }

        // Empty ptr == time to exit
        if (event.data.ptr == nullptr) {
            break;
        }

        auto *pair = reinterpret_cast<PidfdPair *>(event.data.ptr); // NOLINT
        auto pid = pair->pid;

        // Close the pidfd itself
        close(pair->fd);

        // Remove the pidfd from the epoll fd
        epoll_ctl(epollFd_, EPOLL_CTL_DEL, pair->fd, nullptr);

        // Delete the pair; carefully avoid a double-free
        bool lastServer = false;
        if (havePid(pid)) {
            delete pair; // NOLINT

            // By running delete before removePid, we try to ensure that our old
            // pidfd is fully cleaned up before a pidfd for a conflicting pid
            // could be created.
            //
            // Note that only this thread calls removePid. If this invariant
            // changes, care will need to be taken.
            lastServer = removePid(pid) == 0;
        }

        qDebug("Wineserver process pid=%d stopped", pid);
        QMetaObject::invokeMethod(this, &WineMonitor::serverStopped, Qt::QueuedConnection, pid, lastServer);
    }

    close(epollFd_);
    epollFd_ = -1;
}

void WineMonitorLinux::addPid(pid_t pid)
{
    QMutexLocker locker(&wineserversMutex_);
    wineservers_.insert(pid);
}

auto WineMonitorLinux::removePid(pid_t pid) -> qsizetype
{
    QMutexLocker locker(&wineserversMutex_);
    wineservers_.remove(pid);

    return wineservers_.size();
}

auto WineMonitorLinux::havePid(pid_t pid) -> bool
{
    QMutexLocker locker(&wineserversMutex_);
    return wineservers_.contains(pid);
}

void WineMonitorLinux::directoryChanged(const QString &path)
{
    if (QFileInfo { path } == QFileInfo { serverPrefix_ }) {
        checkWineserverDirectories();
    } else {
        checkWineserverDirectory(path);
    }
}

void WineMonitorLinux::fileChanged(const QString &path)
{
    Q_UNUSED(this);
}
