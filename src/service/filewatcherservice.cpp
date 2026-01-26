#include "filewatcherservice.h"

#include <QApplication>

#include "../window/mainwindow.h"
#include "../helper/settingshelper.h"

FileWatcherService::FileWatcherService(QObject *parent)
    : QObject{parent}
{}

void FileWatcherService::init()
{
    mAntishakeTimer = new QTimer(this);
    mAntishakeTimer->setSingleShot(true);
    connect(mAntishakeTimer, &QTimer::timeout, this, &FileWatcherService::antishakeTrigger);
    mStarting       = false;
    mFiles.clear();
    mPendingPaths.clear();
    connect(&mWatcher, &QFileSystemWatcher::fileChanged, this, &FileWatcherService::onFileChanged);
    connect(&mWatcher, &QFileSystemWatcher::directoryChanged, this, &FileWatcherService::onDirectoryChanged);
}

int FileWatcherService::restart()
{
    qDebug() << __func__;
    // clean up
    mAntishakeTimer->stop();
    mPendingPaths.clear();
    if (mWatcher.files().size() > 0) {
        mWatcher.removePaths(mWatcher.files());
    }
    if (mWatcher.directories().size() > 0) {
        mWatcher.removePaths(mWatcher.directories());
    }
    // to start
    for (int i = 0; i < mFiles.size(); ++i) {
        watcher_t w = mFiles.at(i);
        w.exists    = w.fileInfo.exists();
        mFiles[i]   = w;
        mWatcher.addPath(w.fileInfo.filePath());
        mWatcher.addPath(w.fileInfo.path());
    }
    mStarting = true;
    return 0;
}

int FileWatcherService::stop()
{
    qDebug() << __func__;
    mAntishakeTimer->stop();
    mPendingPaths.clear();
    if (mWatcher.files().size() > 0) {
        mWatcher.removePaths(mWatcher.files());
    }
    if (mWatcher.directories().size() > 0) {
        mWatcher.removePaths(mWatcher.directories());
    }
    mStarting = false;
    return 0;
}

void FileWatcherService::addPath(const QString &path)
{
    qDebug() << __func__ << path;
    for (int i = 0; i < mFiles.size(); ++i) {
        watcher_t w = mFiles.at(i);
        if (w.fileInfo.filePath() == path) {
            return ;
        }
    }
    QFileInfo info(path);
    watcher_t w = {info, info.exists()};
    mFiles.append(w);
    if (mStarting == true) {
        mWatcher.addPath(path);
        mWatcher.addPath(QFileInfo(path).path());
    }
}

void FileWatcherService::addPaths(const QStringList &paths)
{
    qDebug() << __func__ << paths;
    for (int i = 0; i < paths.size(); ++i) {
        watcher_t w = {QFileInfo(paths.at(i)), false};
        mFiles.append(w);
    }
    for (int i = 0; i < paths.size(); ++i) {
        bool found = false;
        for (int j = 0; j < mFiles.size(); ++j) {
            watcher_t w = mFiles.at(j);
            if (w.fileInfo.filePath() == paths.at(i)) {
                found = true;
            }
        }
        if (!found) {
            QFileInfo info(paths.at(i));
            watcher_t w = {info, info.exists()};
            mFiles.append(w);
            if (mStarting == true) {
                mWatcher.addPath(paths.at(i));
                mWatcher.addPath(QFileInfo(paths.at(i)).path());
            }
        }
    }
}

void FileWatcherService::removePath(const QString &path)
{
    qDebug() << __func__ << path;
    for (int i = 0; i < mFiles.size(); ) {
        watcher_t w = mFiles.at(i);
        if (w.fileInfo.filePath() == path) {
            qDebug() << __func__ << path;
            mFiles.removeAt(i);
            if (mStarting == true) {
                mWatcher.removePath(path);
                mWatcher.removePath(QFileInfo(path).path());
                if (mWatcher.files().size() == 0 || mWatcher.directories().size() == 0) {
                    mStarting = false;
                }
            }
        } else {
            ++i;
        }
    }
}

void FileWatcherService::removePaths(const QStringList &paths)
{
    qDebug() << __func__ << paths;
    for (int i = 0; i < paths.size(); ++i) {
        for (int j = 0; j < mFiles.size(); ++j) {
            watcher_t w = mFiles.at(j);
            if (w.fileInfo.filePath() == paths.at(i)) {
                qDebug() << __func__ << paths.at(i);
                mFiles.removeAt(j);
                if (mStarting == true) {
                    mWatcher.removePath(paths.at(i));
                    mWatcher.removePath(QFileInfo(paths.at(i)).path());
                    if (mWatcher.files().size() == 0 || mWatcher.directories().size() == 0) {
                        mStarting = false;
                    }
                }
                break;
            }
        }
    }
}

void FileWatcherService::cleanup()
{
    qDebug() << __func__;
    if (mWatcher.files().size() > 0) {
        mWatcher.removePaths(mWatcher.files());
    }
    if (mWatcher.directories().size() > 0) {
        mWatcher.removePaths(mWatcher.directories());
    }
    mFiles.clear();
    mAntishakeTimer->stop();
    mPendingPaths.clear();
    mStarting = false;
}

void FileWatcherService::onDirectoryChanged(const QString &path)
{
    // qDebug() << __func__ << path;
    if (!QFileInfo::exists(path)) {
        qDebug() << __func__ << "Directory:" << path << "has been deleted.";
    }
    for (int i = 0; i < mFiles.size(); ++i) {
        // 匹配监听列表中匹配的文件的目录
        watcher_t w = mFiles.at(i);
        if (w.fileInfo.filePath().contains(path)) {
            // 如果所在目录有文件发生变化，结合之前的exist结果判断文件是被新建还是被删除
            bool newExist  = QFileInfo::exists(w.fileInfo.filePath());
            if (w.exists && !newExist) {
                // 如果先前存在，现在已经不存在了，那么文件可能已经被删除了
                qDebug() << __func__ << "Delete:" << w.fileInfo.filePath();
                removePendingQueue(w.fileInfo.filePath());
                w.exists = newExist;
                mFiles[i] = w;

            } else if (!w.exists && newExist) {
                // 如果先前不存在，现在存在了，那么文件可能已经被创建了
                qDebug() << __func__ << "New:" << w.fileInfo.filePath();
                addPendingQueue(w.fileInfo.filePath());
                // 文件被新建后需要重新重新添加至Watcher
                mWatcher.addPath(w.fileInfo.filePath());
                w.exists = newExist;
                mFiles[i] = w;

            }
        }
    }
}

void FileWatcherService::onFileChanged(const QString &path)
{
    if (!mWatcher.files().contains(path) && QFileInfo::exists(path)) {
        qDebug() << __func__ << "re add path:" << path;
        mWatcher.addPath(path);
    }
    // qDebug() << __func__ << path << exist;
    if (QFileInfo::exists(path)) {
        qDebug() << __func__ << "Changed:" << path;
        addPendingQueue(path);
    }
}

/**
 * @brief FileWatcherService::antishake
 * 文件监视器会频繁产生消息，因此这里需要添加防抖以确保文件状态稳定
 * // TODO 在Debug界面添加修改防抖时间
 */
void FileWatcherService::antishake()
{
    SettingsHelper *settings = SettingsHelper::getInstance();
    mAntishakeTimer->stop();
    mAntishakeTimer->start(settings->getAppDeboundDelay());
}

void FileWatcherService::antishakeTrigger()
{
    MainWindow *win = MainWindow::getInstance();
    QVariantMap map;
    if (mPendingPaths.isEmpty()) {
        return ;
    }
    map["files"] = mPendingPaths;
    sendMsgToObject(win, MSEvent::EVENT_TYPE_FILE_CHANGED_IND, map);
    mPendingPaths.clear();
}

void FileWatcherService::addPendingQueue(const QString &path)
{
    if (!mPendingPaths.contains(path)) {
        mPendingPaths.append(path);
    }
    antishake();
}

void FileWatcherService::removePendingQueue(const QString &path)
{
    mPendingPaths.removeOne(path);
    antishake();
}

void FileWatcherService::sendMsgToObject(QObject *object, uint16_t code, QVariantMap map)
{
    map["resultSupplier"] = "FileWatcherService";
    if (object) {
        MSEvent *event = new MSEvent(this, code);
        event->setData(map);
        QCoreApplication::postEvent(object, event);
    } else {
        qWarning() << __func__ << "Not Object to send.";
    }
}



