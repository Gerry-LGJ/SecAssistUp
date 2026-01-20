#ifndef DOWNLOADSERVICE_H
#define DOWNLOADSERVICE_H

#include <QObject>
#include <QElapsedTimer>

#include "../helper/network.h"
#include "../common/singleton.h"
#include "../window/mainwindow.h"
#include "../helper/settingshelper.h"
#include "../helper/filtools.h"
#include "msevent.h"

class DownloadService : public QObject
{
    Q_OBJECT
public:
    enum {
        STATE_TYPE_IDLE,
        STATE_TYPE_DOWNLOADING
    };

    SINGLETON(DownloadService)
    void init();

    // 判断是否html内容
    static bool checkTagDensity(const QString &content);
    static bool isLikelyHtmlByPrefix(const QString &content);
    static bool isHtmlContent(const QString &content);
    static bool isHtmlContentEnhanced(const QString &content);

protected:
    bool event(QEvent *e);

private:
    uint16_t         mState;
    bool             mDownloading;
    QObject         *mDownloadReqSender;
    Network         *mNetwork;
    NetworkCallable *mDownloadReqCallable;
    MainWindow      *mMainWindow;
    SettingsHelper  *mSettingsHelper;
    FilTools        *mFilTools;
    QString          mUserAgent;
    QStringList      mFiles;
    QString          mDir;
    QString          mWebDir;
    QString          mActiveFile;
    QString          mScript;
    QElapsedTimer    mTakeTimer;

    // func
    explicit DownloadService(QObject *parent = nullptr);
    void transition(uint16_t state);
    bool downloadServiceEventHandler(MSEvent *e);
    void initDownloadReqCallable();
    void sendMsgToObject(QObject *object, uint16_t code, QVariantMap map);
    void download();
    void downloadNext();
    int  runDownloadScript();
};

#endif // DOWNLOADSERVICE_H
