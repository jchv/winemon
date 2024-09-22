#include <QPointer>

#include "winemonitor.h"
#include "winemonitor_linux.h"

QT_USE_NAMESPACE

auto WineMonitor::create(QObject *parent) -> QPointer<WineMonitor>
{
    return { new WineMonitorLinux(parent) };
}

WineMonitor::WineMonitor(QObject *parent) : QObject(parent) { }
