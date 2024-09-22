#include <QListView>

#include "maindialog.h"
#include "winemanager.h"
#include "wineserverlist.h"

QT_USE_NAMESPACE

MainDialog::MainDialog(WineManager *manager, QWidget *parent) : QDialog { parent }, manager_ { manager }, ui {}
{
    ui.setupUi(this);
    ui.serverView->setModel(manager->listModel());
    ui.serverStartedNotificationCheckBox->setChecked(manager->shouldNotifyOnStart());
    ui.serverStoppedNotificationCheckBox->setChecked(manager->shouldNotifyOnStop());
    ui.alwaysShowCheckBox->setChecked(manager->shouldAlwaysShow());
    QObject::connect(ui.closeButton, &QAbstractButton::clicked, this, &QDialog::hide);
    QObject::connect(ui.quitButton, &QAbstractButton::clicked, qApp, &QApplication::quit);
    QObject::connect(ui.killServerButton, &QAbstractButton::clicked, this, &MainDialog::killServer);
    QObject::connect(ui.taskManagerButton, &QAbstractButton::clicked, this, &MainDialog::startTaskManager);
    QObject::connect(ui.serverStartedNotificationCheckBox, &QAbstractButton::clicked, manager, &WineManager::setShouldNotifyOnStart);
    QObject::connect(ui.serverStoppedNotificationCheckBox, &QAbstractButton::clicked, manager, &WineManager::setShouldNotifyOnStop);
    QObject::connect(ui.alwaysShowCheckBox, &QAbstractButton::clicked, manager, &WineManager::setShouldAlwaysShow);
}

void MainDialog::killServer()
{
    auto selectedRows = ui.serverView->selectionModel()->selectedRows();
    for (const auto &selectedRow : selectedRows) {
        manager_->listModel()->server(selectedRow.row()).kill();
    }
}

void MainDialog::startTaskManager()
{
    auto selectedRows = ui.serverView->selectionModel()->selectedRows();
    for (const auto &selectedRow : selectedRows) {
        manager_->listModel()->server(selectedRow.row()).taskmgr();
    }
}
