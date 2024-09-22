#pragma once

#include "ui_maindialog.h"
#include <QDialog>
#include <QPointer>

class WineServerListModel;
class WineManager;

class MainDialog : public QT_PREPEND_NAMESPACE(QDialog)
{
    Q_OBJECT

public:
    explicit MainDialog(WineManager *manager, QT_PREPEND_NAMESPACE(QWidget) *parent = nullptr);

    Q_SLOT void killServer();
    Q_SLOT void startTaskManager();

private:
    QT_PREPEND_NAMESPACE(QPointer)<WineManager> manager_;

    QT_PREPEND_NAMESPACE(Ui::MainDialog) ui;
};
