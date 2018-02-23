#ifndef INOTIFYWATCHER_HPP
#define INOTIFYWATCHER_HPP

#include <QObject>
#include <QHash>
#include <QMultiHash>

class QSocketNotifier;
class INotifyWatcher : public QObject
{
    Q_OBJECT
public:
    explicit INotifyWatcher(QObject *parent = nullptr);
    virtual ~INotifyWatcher();

    QStringList addPaths(const QStringList &paths);
    QStringList removePaths(const QStringList &paths);

signals:
    void directoryChanged(const QString &path, bool removed);
    void fileChanged(const QString &path, bool removed);
    void contentChanged(const QString &path, bool created);

private slots:
    void readFromInotify();

private:
    QString getPathFromID(int id) const;

private:
    int inotifyFd = -1;
    QHash<QString, int> pathToID;
    QMultiHash<int, QString> idToPath;
    QSocketNotifier *notifier = nullptr;

    QStringList files;
    QStringList directories;
};

#endif // INOTIFYWATCHER_HPP
