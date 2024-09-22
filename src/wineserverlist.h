#pragma once

#include <QAbstractListModel>
#include <QString>

struct WineServerData
{
    WineServerData(pid_t pid);

    [[nodiscard]] auto toString() const -> QString;
    void kill() const;
    void taskmgr() const;

    pid_t pid;
    QString exe;
    QString package;
    QString prefix;
};

class WineServerListModel : public QT_PREPEND_NAMESPACE(QAbstractListModel)
{
    Q_OBJECT

public:
    WineServerListModel(QObject *parent = nullptr);

    [[nodiscard]] auto rowCount(const QModelIndex &parent = {}) const -> int override;
    [[nodiscard]] auto columnCount(const QModelIndex &parent = {}) const -> int override;
    [[nodiscard]] auto data(const QModelIndex &index, int role) const -> QVariant override;
    [[nodiscard]] auto headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const -> QVariant override;
    [[nodiscard]] auto server(int row) -> WineServerData &;

    Q_SLOT void serverRunning(pid_t pid);
    Q_SLOT void serverStopped(pid_t pid, bool lastServer);

private:
    QList<WineServerData> listData_;
};
