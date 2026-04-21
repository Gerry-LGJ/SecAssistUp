#ifndef REFRESHSERVICE_H
#define REFRESHSERVICE_H

#include <QObject>
#include <QTimer>

#include "../helper/network.h"
#include "../helper/settingshelper.h"
#include "../common/singleton.h"
#include "../common/filedata_t.h"
#include "../window/loginwindow.h"
#include "../window/mainwindow.h"
#include "msevent.h"

class RefreshService : public QObject
{
    Q_OBJECT
public:
    enum {
        STATE_TYPE_IDLE,
        STATE_TYPE_REFRESHING,
    };
    SINGLETON(RefreshService)
    void init();
    QList<file_t> getFileList();
    QStringList   getPathList();
    QString       getPathByPathList(QStringList pathList);
    void          restartCountdown();
    void          stopCountdown();
    void          recoverSelects();

protected:
    bool event(QEvent *e);

private:
    uint16_t         mState;
    bool             mRefreshRequesting;
    QStringList      mRequestPathList;
    QStringList      mPathList;
    QList<file_t>    mFileList;
    QList<file_t>    mPreviousSelects;

    // inst
    QObject         *mRefreshReqSender;
    SettingsHelper  *mSettingsHelper;
    Network         *mNetwork;
    NetworkCallable *mRefreshReqCallable;
    LoginWindow     *mLoginWindow;
    MainWindow      *mMainWindow;

    // func
    explicit RefreshService(QObject *parent = nullptr);
    void initRefreshNetworkCallable();
    bool refreshServiceEventHandler(MSEvent *e);
    bool refresh(QStringList requestPathList);
    bool entryFolder(QString name);
    bool returnParentDirectory();
    void transition(uint16_t state);
    void sendMsgToObject(QObject *object, uint16_t code, QVariantMap map);
    bool onParseJsonToList(QString result);
    void recordSelects();

    // countdown
    int              mCountdown;
    QTimer          *mCountdownTimer;
    void             setCountdownLabelText(QString text);

};

#endif // REFRESHSERVICE_H
