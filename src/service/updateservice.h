#ifndef UPDATESERVICE_H
#define UPDATESERVICE_H

#include <QObject>
#include <QDebug>
#include <QTimer>

#include "../common/singleton.h"
#include "../helper/settingshelper.h"
#include "../helper/filtools.h"
#include "../helper/network.h"
#include "../helper/appinfo.h"
#include "../helper/versionclass.h"
#include "msevent.h"

class Version;

class UpdateService : public QObject
{
    Q_OBJECT
public:
    enum {
        idle_s,
        checking_s,
        downloading_s,
        canceling_s
    } state_t;
    typedef enum {
        tag_name_n,
        body_n,
        browser_download_url_n,
        digest_n
    } latest_info_t;
    typedef enum {
        NO_ERROR,
        NETWORK_ERROR,
        CANCEL_ERROR,
        MAX_ERROR
    } ERROR_CODE_T;

    SINGLETON(UpdateService)

    void    init();
    QString getUpdateMsg();
    int     getUpdateProgress();
    static  bool checkForAutoUpdate(bool isManual = false);
    static QString getInstallerFileByVersion(Version& version);
    static QString getInstallerLocation();
    static bool    checkInstallerExists(Version& version);
    bool    checkForUpdate(bool silent = true);

    Q_SIGNAL void updateProgressChanged(int progress);
    Q_SIGNAL void updateMsgChanged     (QString msg);

private:
    // global
    SettingsHelper  *mSettings;
    FilTools        *mFilTools;
    Network         *mNetwork;
    bool             mSilent;
    uint16_t         mState;
    bool             mChecking;
    bool             mDownloading;
    Version          mCurrentVersion;
    Version          mLatestVersion;
    AppInfo         *mAppInfo;
    QString          mUserAgent;
    QString          mUpdateMsg;
    int              mPercent;
    QString          mDownloadUrl;
    QString          mDownloadFile;
    QString          mInstallerSha256;
    int              mLastErrorCode;


    // Request Update Information
    NetworkCallable *mGetLatestCallable;

    // Download File
    NetworkCallable *mDownloadCallable;

    // func
    explicit UpdateService(QObject *parent = nullptr);
    void    initGetLatestCallable();
    void    initDownloadNetworkCallable();
    void    transition(uint16_t state);
    void    notifyUpdateMsgChanged(QString msg);
    void    notifyUpdateProgressChanged(int progress);
    bool    download();
    void    onGetLatestInfoSuccess(QString &rsp);
    void    onDownloadSuccess(QString result);
    void    onMessageClicked();
    // tools function
    QString getLatestValueFromRsp(QString &rsp, latest_info_t type);
    void    cleanupInstaller();
    void    cleanupInstaller(Version &version);




signals:
};





#endif // UPDATESERVICE_H
