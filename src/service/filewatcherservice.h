#ifndef FILEWATCHERSERVICE_H
#define FILEWATCHERSERVICE_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QFileInfo>
#include <QTimer>

#include "../common/singleton.h"
#include "../helper/settingshelper.h"
#include "../helper/filtools.h"
#include "msevent.h"

class FileWatcherService : public QObject
{
    Q_OBJECT
public:
    typedef struct
    {
        QFileInfo fileInfo;
        bool      exists;
    } watcher_t;

    SINGLETON(FileWatcherService)
    void init();
    int restart();
    int stop();
    void addPath(const QString &path);
    void addPaths(const QStringList &paths);
    void removePath(const QString &path);
    void removePaths(const QStringList &paths);
    void cleanup();

protected:
    // bool event(QEvent *e);

private:
    QFileSystemWatcher mWatcher;
    QList<watcher_t>   mFiles;
    bool               mStarting;
    QTimer            *mAntishakeTimer;
    QStringList        mPendingPaths;

    // func
    explicit FileWatcherService(QObject *parent = nullptr);
    void     onDirectoryChanged(const QString &path);
    void     onFileChanged(const QString &path);
    void     antishake();
    void     antishakeTrigger();
    void     addPendingQueue(const QString &path);
    void     removePendingQueue(const QString &path);
    void     sendMsgToObject(QObject *object, uint16_t code, QVariantMap map);
};

#endif // FILEWATCHERSERVICE_H
