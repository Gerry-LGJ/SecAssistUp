#ifndef UPLOADSERVICE_H
#define UPLOADSERVICE_H

#include <QObject>
#include <QElapsedTimer>

#include "../helper/network.h"
#include "../common/singleton.h"
#include "../window/mainwindow.h"
#include "../helper/settingshelper.h"
#include "../helper/filtools.h"
#include "msevent.h"

class UploadService : public QObject
{
    Q_OBJECT
public:
    enum {
        STATE_TYPE_IDLE,
        STATE_TYPE_UPLOADING,
    };
    SINGLETON(UploadService)
    void init();
    static QString getCurrentTimeStringWithDynamicTimezone();

protected:
    bool event(QEvent *e);

private:
    uint16_t         mState;
    bool             mUploading;
    QObject         *mUploadReqSender;
    Network         *mNetwork;
    NetworkCallable *mUploadReqCallable;
    NetworkCallable *mUploadChunkReqCb;
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
    // Chunk Uploader
    qint64           mFileId, mSize, mChunk, mChunks, mReadSize, mChunkSize;
    QFileInfo        mActiveFileInfo;
    QString          mActiveUrl;

    // func
    explicit UploadService(QObject *parent = nullptr);
    void transition(uint16_t state);
    bool uploadServiceEventHandler(MSEvent *e);
    void initUploadReqCallable();
    void initUploadChunkReqCb();
    void sendMsgToObject(QObject *object, uint16_t code, QVariantMap map);
    void upload();
    void uploadNext();
    // Uploader Chunk
    void uploadChunks();
    void uploadChunkNextFile();
    void uploadChunkNext();
    NetworkParams *createHttpPartsByMap(NetworkParams *params, QList<QPair<QString, QVariant>> &list);
    bool uploadChunkCheckRspJsonSuccess(QString text);
    bool uploadChunkCheckRspHtml(NetworkCallable *cb, QString text);
    QJsonObject uploadChunkGetRspJsonInfo(QString text);
    // Uploader Chunk End
    int  runUploadScript();
};

#endif // UPLOADSERVICE_H
