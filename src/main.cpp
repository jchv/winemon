#include <QSystemTrayIcon>
#include <QtDBus/QtDBus>
#include <QtWidgets>

#include "winemanager.h"

auto setupInstance(WineManager &manager) -> bool;

auto main(int argc, char *argv[]) -> int
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("jchw");
    QCoreApplication::setOrganizationDomain("io.jchw");
    QCoreApplication::setApplicationName("Winemon");
    QGuiApplication::setQuitOnLastWindowClosed(false);

    WineManager manager;
    if (!setupInstance(manager)) {
        return 0;
    }

    return QApplication::exec();
}

auto setupInstance(WineManager &manager) -> bool
{
    static constexpr const char *kServiceName = "io.jchw.winemon";
    auto connection = QDBusConnection::sessionBus();
    if (!connection.isConnected()) {
        qWarning("Cannot connect to the D-Bus session bus.");
        return true;
    }
    if (!connection.registerService(kServiceName)) {
        // If we can't register the service try to invoke the existing instance.
        QDBusInterface iface(kServiceName, "/");
        if (!iface.isValid()) {
            qWarning("Invalid DBus interface at: %s\n", kServiceName);
            return false;
        }
        QDBusReply<void> reply = iface.call("invoke");
        if (!reply.isValid()) {
            qWarning("DBus call failed: %s\n", qPrintable(reply.error().message()));
            return false;
        }
        qDebug("Invoked existing instance via DBus");
        return false;
    }
    connection.registerObject("/", &manager, QDBusConnection::ExportAllSlots);
    return true;
}
