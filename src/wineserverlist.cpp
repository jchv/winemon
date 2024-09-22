#include <QAbstractItemModel>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStringBuilder>

#include <signal.h>

#include "wineserverlist.h"

QT_USE_NAMESPACE

WineServerData::WineServerData(pid_t pid) : pid { pid }
{
    QFileInfo exeFile { QString { "/proc/%1/exe" }.arg(pid) };
    exe = exeFile.canonicalFilePath();

    QFile environFile { QString { "/proc/%1/environ" }.arg(pid) };
    if (environFile.open(QIODevice::ReadOnly)) {
        // TODO: This is inefficient out of laziness.
        // Should probably just do a string search for \0WINEPREFIX= over
        // blocks of data.
        auto environmentData = environFile.readAll();
        auto variables = environmentData.split(0);
        for (const auto &variableData : variables) {
            static constexpr QByteArrayView kWinePrefixEnvPrefix { "WINEPREFIX=" };
            if (variableData.startsWith(kWinePrefixEnvPrefix)) {
                prefix = { variableData.mid(kWinePrefixEnvPrefix.size()) };
            }
        }
    }

    QFile wineInf { QFileInfo { exeFile.canonicalFilePath() }.dir().absoluteFilePath("../share/wine/wine.inf") };
    if (wineInf.open(QIODevice::ReadOnly)) {
        QTextStream stream(&wineInf);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            static constexpr QStringView kWineInfVersionPrefix { u";; Version: " };
            if (line.startsWith(kWineInfVersionPrefix)) {
                package = line.sliced(kWineInfVersionPrefix.size());
                break;
            }
        }
        wineInf.close();
    }
}

auto WineServerData::toString() const -> QString
{
    QString nameText = package;
    if (nameText.isEmpty()) {
        nameText = "Wine (unknown version)";
    }

    QString prefixText;
    if (prefix.isEmpty()) {
        prefixText = "";
    } else {
        prefixText = QString { ", prefix %1" }.arg(prefix);
    }

    QString processText;
    if (exe.isEmpty()) {
        processText = QString { ", server PID %1, unknown binary" }.arg(pid);
    } else {
        processText = QString { ", server PID %1 (%2)" }.arg(pid).arg(exe);
    }

    return nameText % prefixText % processText;
}

void WineServerData::kill() const
{
    QProcess process;
    process.setProgram(exe);
    process.setArguments({ "-k" });
    QStringList environment;
    if (!prefix.isEmpty()) {
        environment.append(QString { "WINEPREFIX=%1" }.arg(prefix));
    }
    process.setEnvironment(environment);
    process.start();
    process.waitForStarted();
    process.waitForFinished();
}

void WineServerData::taskmgr() const
{
    QProcess process;
    QDir binDir { QFileInfo { exe }.dir() };
    process.setProgram(binDir.absoluteFilePath("wine"));
    process.setArguments({ "taskmgr.exe" });
    QStringList environment = QProcess::systemEnvironment();
    if (!prefix.isEmpty()) {
        environment.append(QString { "WINEPREFIX=%1" }.arg(prefix));
    }
    process.setEnvironment(environment);
    process.startDetached();
}

WineServerListModel::WineServerListModel(QObject *parent) : QAbstractListModel(parent) { }

auto WineServerListModel::rowCount(const QModelIndex &parent) const -> int
{
    if (parent.isValid()) {
        return 0;
    }

    return static_cast<int>(listData_.count());
}

auto WineServerListModel::columnCount(const QModelIndex &parent) const -> int
{
    if (parent.isValid()) {
        return 0;
    }

    return 4;
}

auto WineServerListModel::data(const QModelIndex &index, int role) const -> QVariant
{
    if (index.parent().isValid() || index.row() < 0 || index.row() >= rowCount() || role != Qt::DisplayRole) {
        return {};
    }

    const auto &row = listData_.at(index.row());

    switch (index.column()) {
    case 0:
        return row.package;
    case 1:
        return row.prefix;
    case 2:
        return QString::number(row.pid);
    case 3:
        return row.exe;
    default:
        return {};
    }
}

auto WineServerListModel::headerData(int section, Qt::Orientation orientation, int role) const -> QVariant
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
    case 0:
        return "Version";
    case 1:
        return "Prefix";
    case 2:
        return "PID";
    case 3:
        return "Server Path";
    default:
        return {};
    }
}

auto WineServerListModel::server(int row) -> WineServerData &
{
    return listData_[row];
}

void WineServerListModel::serverRunning(pid_t pid)
{
    int newIndex = static_cast<int>(listData_.size());
    beginInsertRows(QModelIndex {}, newIndex, newIndex);
    listData_.append(WineServerData { pid });
    endInsertRows();
}

void WineServerListModel::serverStopped(pid_t pid, bool lastServer)
{
    // O(n) removal cost is probably OK in this context.
    QMutableListIterator<WineServerData> iterator { listData_ };
    int row = 0;

    while (iterator.hasNext()) {
        if (iterator.next().pid == pid) {
            beginRemoveRows(QModelIndex {}, row, row);
            iterator.remove();
            endRemoveRows();
        }
        row++;
    }
}
