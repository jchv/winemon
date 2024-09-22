#pragma once

#include <QObject>

/**
 * Class that monitors running wineserver instances.
 */
class WineMonitor : public QT_PREPEND_NAMESPACE(QObject)
{
    Q_OBJECT

public:
    explicit WineMonitor(QObject *parent = nullptr);

    static auto create(QObject *parent = nullptr) -> QPointer<WineMonitor>;

    /**
     * Starts the wineserver monitor.
     */
    virtual void start() = 0;

    /**
     * Signal is sent any time a server is detected that was not previously.
     * On initialization, you will receive one serverRunning signal for each
     * detected wineserver instance.
     */
    Q_SIGNAL void serverRunning(pid_t pid);

    /**
     * Signal is sent any time a previously-running server is detected to have
     * stopped. You should only ever receive this for servers that you have
     * been notified were running, and only ever once.
     *
     * If this is the last known wineserver, lastServer will be set to true.
     */
    Q_SIGNAL void serverStopped(pid_t pid, bool lastServer);

    /**
     * Sent when initialization is finished, after the initial serverRunning
     * signals are sent.
     */
    Q_SIGNAL void initialized();
};
