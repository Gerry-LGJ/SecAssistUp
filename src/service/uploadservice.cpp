#include "uploadservice.h"

#include <QDebug>
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QDir>
#include <QThreadPool>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QDateTime>
#include <QLocale>
#include <QTimeZone>

#include "mainservice.h"
#include "../helper/appinfo.h"
#include "../window/mainwindow.h"
#include "downloadservice.h"

UploadService::UploadService(QObject *parent)
    : QObject{parent}
{
    mNetwork             = Network::getInstance();
    mMainWindow          = MainWindow::getInstance();
    mSettingsHelper      = SettingsHelper::getInstance();
    mFilTools            = FilTools::getInstance();
}

void UploadService::init()
{
    mUserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, "
                 "like Gecko) Chrome/138.0.0.0 Safari/537.36 Edg/138.0.0.0";
    mUploadReqSender = nullptr;
    mState           = STATE_TYPE_IDLE;
    initUploadReqCallable();
    initUploadChunkReqCb();
    mFileId          = mSize = mChunk = mChunks = mReadSize = 0;
    mChunkSize       = (1 * 1024 * 1024); // 1M

}

QString UploadService::getCurrentTimeStringWithDynamicTimezone()
{
    // Power By DeepSeek
    QDateTime now = QDateTime::currentDateTime();
    QLocale englishLocale(QLocale::English, QLocale::UnitedStates);

    // 获取本地时区
    QTimeZone localTimeZone = QTimeZone::systemTimeZone();

    // 计算时区偏移（毫秒转换为小时和分钟）
    int offsetSeconds = localTimeZone.offsetFromUtc(now);
    int offsetHours = offsetSeconds / 3600;
    int offsetMinutes = abs(offsetSeconds % 3600) / 60;

    // 格式化时区偏移（例如：GMT+0800 或 GMT-0500）
    QString timeZone = QString("GMT%1%2%3")
                           .arg(offsetHours >= 0 ? "+" : "-")
                           .arg(qAbs(offsetHours), 2, 10, QChar('0'))
                           .arg(offsetMinutes, 2, 10, QChar('0'));

    // 获取时区显示名称
    QString timeZoneName = QString("(%1)").arg(localTimeZone.displayName(QTimeZone::StandardTime));

    // 组合字符串
    QString ret = QString("%1 %2 %3 %4")
        .arg(englishLocale.toString(now.date(), "ddd MMM dd yyyy"), now.toString("HH:mm:ss"), timeZone, timeZoneName);
    qDebug() << __func__ << ret;
    return ret;
}

void UploadService::initUploadReqCallable()
{
    MainWindow *win      = mMainWindow;
    mUploadReqCallable   = new NetworkCallable(this);
    connect(mUploadReqCallable, &NetworkCallable::start, this, [=] {
        mUploading = true;
    });
    connect(mUploadReqCallable, &NetworkCallable::finish, this, [=] {
        mUploading = false;
    });
    connect(mUploadReqCallable, &NetworkCallable::error, this, [=] (int status, QString errorString, QString result) {
        qDebug() << "initUploadReqCallable" << "error " <<
            "status:" << status <<
            "errorString:" << errorString <<
            "result:" << result;
        QVariantMap map;
        map["resultCode"]  = MSEvent::RESULT_CODE_NETWORK_ONERROR;
        map["status"]      = status;
        map["errorString"] = errorString;
        map["result"]      = result;
        sendMsgToObject(mUploadReqSender, MSEvent::EVENT_TYPE_UPLOAD_CFM, map);

        mUploading = false;
        win->setProgressBarUpload(0, "");
        transition(STATE_TYPE_IDLE);
    });
    connect(mUploadReqCallable, &NetworkCallable::success, this, [=] (QString result) {
        qDebug() << "Upload result:" << result;
        uint16_t resultCode = MSEvent::RESULT_CODE_FAIL;
        QString errorString = "";
        if (result.length() > 0) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8(), &err);
            if (err.error == QJsonParseError::NoError) {
                bool code = doc.object().value("code").toBool(false);
                QString data = doc.object().value("data").toString();
                if (code) {
                    if (mFiles.size() == 0) {
                        resultCode = MSEvent::RESULT_CODE_SUCCESS;
                        qint64 takeTime = mTakeTimer.elapsed();
                        qDebug() << "Upload Succeed" << takeTime / 1000 << "ms.";
                    } else {
                        uploadNext();
                        return ;
                    }
                } else {
                    qDebug() << "The server returned that the upload failed. msg:" << data;
                }
                errorString = data;
            } else {
                // 尝试判断是否返回了登录页
                bool isHtml = false, isSecError = false;
                QString contentType = mUploadReqCallable->replyGetHeader("Content-Type");
                if (contentType.length() > 0 && contentType.contains("text/html")) {
                    qDebug() << "includes text/html, start check HTML format";
                    isHtml = DownloadService::isHtmlContent(result);
                    if (isHtml && result.contains("文件防泄密网关") &&
                        result.contains("用户名") &&
                        result.contains("密码") &&
                        result.contains("登录")) {
                        qDebug() << "The server returned the login page information, but there might be an error in the network request.";
                        isSecError = true;
                        errorString = tr("Server has returned to the login page. Please try to log in again.");
                    } else {
                        errorString = err.errorString();
                    }
                } else {
                    errorString = err.errorString();
                }
            }
        }
        QVariantMap map;
        map["resultCode"]  = resultCode;
        map["errorString"] = errorString;
        sendMsgToObject(mUploadReqSender, MSEvent::EVENT_TYPE_UPLOAD_CFM, map);
        mMainWindow->setProgressBarUpload(0, "");
        mUploading = false;
        transition(STATE_TYPE_IDLE);
    });
    connect(mUploadReqCallable, &NetworkCallable::uploadProgress, this, [=] (qint64 recv, qint64 total) {
        int progress = (total > 0) ? (recv * 100) / total : 0;
        qDebug() << "NetworkCallable::uploadProgress:" << progress;
        win->setProgressBarUpload(progress, mActiveFile);
    });
}

void UploadService::initUploadChunkReqCb()
{
    MainWindow *win      = mMainWindow;
    mUploadChunkReqCb    = new NetworkCallable(this);

    connect(mUploadChunkReqCb, &NetworkCallable::start,  this, [=] {
        mUploading = true;
    });
    connect(mUploadChunkReqCb, &NetworkCallable::finish, this, [=] {
        mUploading = false;
    });
    connect(mUploadChunkReqCb, &NetworkCallable::error,  this, [=] (int status, QString errorString, QString result) {
        qDebug() << "initUploadChunkReqCb" << "error " <<
            "status:" << status <<
            "errorString:" << errorString <<
            "result:" << result;
        QVariantMap map;
        map["resultCode"]  = MSEvent::RESULT_CODE_NETWORK_ONERROR;
        map["status"]      = status;
        map["errorString"] = errorString;
        map["result"]      = result;
        sendMsgToObject(mUploadReqSender, MSEvent::EVENT_TYPE_UPLOAD_CFM, map);

        mUploading = false;
        win->setProgressBarUpload(0, "");
        transition(STATE_TYPE_IDLE);
    });
    connect(mUploadChunkReqCb, &NetworkCallable::success,  this, [=] (QString result) {
        qDebug() << "Upload result:" << result;
        uint16_t resultCode = MSEvent::RESULT_CODE_FAIL;
        QString errorString = "";
        if (result.length() > 0) {
            QJsonObject jsonObj = uploadChunkGetRspJsonInfo(result);
            if (!jsonObj.isEmpty()) {
                bool code = uploadChunkCheckRspJsonSuccess(result);
                qDebug() << "initUploadChunkReqCb::success" <<
                    "data:" << jsonObj.value("data").toString() <<
                    "info:" << jsonObj.value("info").toString() <<
                    "mChunk:" << mChunk << "mChunks:" << mChunks <<
                    "mFiles.size():" << mFiles.size();
                if (code) {
                    if (mChunk < mChunks) {
                        uploadChunkNext();
                        return ;
                    } else if (mChunk == mChunks && mFiles.size() != 0) {
                        uploadChunkNextFile();
                        return ;
                    } else if (mFiles.size() == 0) {
                        resultCode = MSEvent::RESULT_CODE_SUCCESS;
                        qint64 takeTime = mTakeTimer.elapsed();
                        qDebug() << "Upload Succeed" << takeTime / 1000 << "ms. md5:" << mFilTools->md5CalculateFile(mActiveFileInfo.filePath());
                    } else {
                        qint64 takeTime = mTakeTimer.elapsed();
                        qWarning() << "An abnormal error has occurred." << takeTime / 1000 << "ms.";
                        errorString = "An abnormal error has occurred.";
                    }
                } else {
                    qDebug() << "The server returned that the upload failed. msg:" << jsonObj.value("data").toString();
                }
                errorString = jsonObj.value("data").toString();
            } else {
                if (uploadChunkCheckRspHtml(mUploadChunkReqCb, result)) {
                    qDebug() << "The server returned the login page information, but there might be an error in the network request.";
                    errorString = tr("Server has returned to the login page. Please try to log in again.");
                } else {
                    errorString = "An abnormal error has occurredd.";
                }
            }
        }
        QVariantMap map;
        map["resultCode"]  = resultCode;
        map["errorString"] = errorString;
        sendMsgToObject(mUploadReqSender, MSEvent::EVENT_TYPE_UPLOAD_CFM, map);
        mMainWindow->setProgressBarUpload(0, "");
        mUploading = false;
        transition(STATE_TYPE_IDLE);
    });
    connect(mUploadChunkReqCb, &NetworkCallable::uploadProgress, this, [=] (qint64 recv, qint64 total) {
        int pro = 100.0 * (1.0/(double)mChunks) * ((double)mChunk - 1.0) + 100 * (1.0/(double)mChunks) * ((double)recv/(double)total);
        qDebug() << "NetworkCallable::uploadProgress:" << pro << "recv:" << recv << "total:" << total;
        win->setProgressBarUpload(pro, mActiveFileInfo.fileName());
    });
}

void UploadService::upload()
{
    mTakeTimer.start();
    uploadNext();
}

void UploadService::uploadNext()
{
    QString name = mFiles.takeFirst();
    QString host = mSettingsHelper->getWebUrl();
    QString url  = host + QString("/index.php?explorer/fileUpload&path=%1").arg(mWebDir);

    qDebug() << __func__ << url << name;

    if (!QFile::exists(name)) {
        QVariantMap map;
        map["resultCode"]  = MSEvent::RESULT_CODE_FAIL;
        map["errorString"] = "A non-existent file is being uploaded. Stopping the upload. " + name;
        sendMsgToObject(mUploadReqSender, MSEvent::EVENT_TYPE_UPLOAD_CFM, map);
        mUploading = false;
        mMainWindow->setProgressBarUpload(0, "");
        transition(STATE_TYPE_IDLE);
        return ;
    }

    if (AppInfo::getInstance()->debugEnable()) {
        qDebug() << "Debug Mode, not request upload";
        return ;
    }

    mActiveFile = QFileInfo(name).fileName();
    mNetwork->postForm(url)
    ->addHeader("User-Agent", mUserAgent)
    ->setRetry(1)
    ->addFile("file", name)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ->addAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy)
#endif
    ->bind(this)
    ->go(mUploadReqCallable);

}

void UploadService::uploadChunks()
{
    mTakeTimer.start();
    uploadChunkNextFile();
}

void UploadService::uploadChunkNextFile()
{
    QString name = mFiles.takeFirst();
    QString host = mSettingsHelper->getWebUrl();
    QString url  = host + QString("/index.php?explorer/fileUpload&path=%1").arg(mWebDir);
    qDebug() << __func__ << url << name;

    if (!QFile::exists(name)) {
        QVariantMap map;
        map["resultCode"]  = MSEvent::RESULT_CODE_FAIL;
        map["errorString"] = "A non-existent file is being uploaded. Stopping the upload. " + name;
        sendMsgToObject(mUploadReqSender, MSEvent::EVENT_TYPE_UPLOAD_CFM, map);
        mUploading = false;
        mMainWindow->setProgressBarUpload(0, "");
        transition(STATE_TYPE_IDLE);
        return ;
    }

    if (AppInfo::getInstance()->debugEnable()) {
        qDebug() << "Debug Mode, not request upload";
        return ;
    }

    mActiveUrl = url;
    mActiveFileInfo.setFile(name);
    mSize = mActiveFileInfo.size(); // 获取文件大小
    // 计算需要分几块上传
    mChunk  = mReadSize = 0;
    mChunks = (mSize + mChunkSize - 1) / mChunkSize;
    qDebug() << __func__ <<
        "size:"      << mSize      <<
        "chunks:"    << mChunks    <<
        "chunksize:" << mChunkSize <<
        "md5:"       << mFilTools->md5CalculateFile(name);
    uploadChunkNext();
    mFileId += 1;
}

void UploadService::uploadChunkNext()
{
    // 构建QHttpMultiPart
    QList<QPair<QString, QVariant>> list;

    // Read Data From Local File
    QFile file(mActiveFileInfo.filePath());
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << __func__ << "Open Upload file Failure.";
        return ;
    }
    if (!file.seek(mReadSize)) {
        qWarning() << __func__ << "QFile seek failure.";
        return ;
    }
    QByteArray chunkData = file.read(mChunkSize);
    if (chunkData.size() == 0) {
        qWarning() << __func__ << "QFile read data length is zero.";
        return ;
    }
    mReadSize += chunkData.size();
    file.close();

    qDebug() << __func__ <<
        "mReadSize:" << mReadSize <<
        "fileid:" << mFileId <<
        "chunk:" << mChunk;

    // Build Param Map
    // name="id"
    QPair<QString, QVariant> id("id", QString("WU_FILE_%1").arg(mFileId));
    list.append(id);
    // name="name"
    QPair<QString, QVariant> name("name", mActiveFileInfo.fileName());
    list.append(name);
    // name="type"
    QPair<QString, QVariant> type("type", QString("application/octet-stream"));
    list.append(type);
    // name="lastModifiedDate"
    QPair<QString, QVariant> lastModifiedDate("lastModifiedDate", getCurrentTimeStringWithDynamicTimezone().toUtf8());
    list.append(lastModifiedDate);
    // name="size"
    QPair<QString, QVariant> size("size", QString("%1").arg(mSize));
    list.append(size);
    if (mChunks > 1) {
        // name="chunks"
        QPair<QString, QVariant> chunks("chunks", QString("%1").arg(mChunks));
        list.append(chunks);
        // name="chunk"
        QPair<QString, QVariant> chunk("chunk", QString("%1").arg(mChunk));
        list.append(chunk);
    }
    ++mChunk;
    // name="fullPath"
    QPair<QString, QVariant> fullPath("fullPath", QString());
    list.append(fullPath);

    // Start Network Request
    NetworkParams *params = mNetwork->postForm(mActiveUrl);

    // Create multi part
    params = createHttpPartsByMap(params, list);

    // Manually set the file stream
    QHttpPart part_http;
    part_http.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QString(R"(form-data; name="file"; filename="%2")").arg(mActiveFileInfo.fileName()));
    part_http.setHeader(QNetworkRequest::ContentTypeHeader,
                        QString("application/octet-stream"));
    part_http.setBody(chunkData);
    params = params->addHttpPart(part_http);

    params->addHeader("User-Agent", mUserAgent)
    ->setTimeout(60 * 1000)
    ->setRetry(1)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ->addAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy)
#endif
    ->bind(this)
    ->go(mUploadChunkReqCb);
}

NetworkParams *UploadService::createHttpPartsByMap(NetworkParams *params, QList<QPair<QString, QVariant>> &list)
{
    for (const auto &each : list) {
        QHttpPart part;
        // qDebug() << __func__ << QString(R"(form-data; name="%1")").arg(each.first);
        // qDebug() << __func__ << each.second.toString().toUtf8();
        part.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QString(R"(form-data; name="%1")").arg(each.first));
        part.setBody(each.second.toByteArray());
        params->addHttpPart(part);
    }
    return params;
}

bool UploadService::uploadChunkCheckRspJsonSuccess(QString text)
{
    bool ret = false;
    QJsonParseError jsonErr;
    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &jsonErr);
    if (jsonErr.error == QJsonParseError::NoError) {
        ret = doc.object().value("code").toBool(false);
    }
    return ret;
}

bool UploadService::uploadChunkCheckRspHtml(NetworkCallable *cb, QString text)
{
    bool isHtml = false, isSecError = false;
    QString errorString;
    QString contentType = cb->replyGetHeader("Content-Type");
    if (contentType.length() > 0 && contentType.contains("text/html")) {
        qDebug() << "includes text/html, start check HTML format";
        isHtml = DownloadService::isHtmlContent(text);
        if (isHtml && text.contains("文件防泄密网关") &&
            text.contains("用户名") &&
            text.contains("密码") &&
            text.contains("登录")) {
            qDebug() << "The server returned the login page information, but there might be an error in the network request.";
            isSecError = true;
            errorString = tr("Server has returned to the login page. Please try to log in again.");
        }
    }
    return isSecError;
}

QJsonObject UploadService::uploadChunkGetRspJsonInfo(QString text)
{
    QJsonParseError jsonErr;
    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &jsonErr);
    if (jsonErr.error == QJsonParseError::NoError) {
        return doc.object();
    }
    return QJsonObject();
}

int UploadService::runUploadScript()
{
    if (mScript.length() <= 0) {
        qDebug() << __func__ << "Not found script file path";
        return -1;
    }
    if (mDir.length() <= 0) {
        qDebug() << __func__ << "Not found script work dir";
        return -2;
    }
    QFileInfo scriptInfo(mScript);
    QDir mmDir(mDir);
    if (!scriptInfo.exists() || !mmDir.exists()) {
        qWarning() << "scriptPath or scriptWorkDir not exists, stop execute script";
        return -3;
    }
    QString cmdStr = "cmd.exe /c \"" + mScript + "\"";
    return system(cmdStr.toUtf8().data());
}

void UploadService::transition(uint16_t state)
{
    qDebug() << "LoginService::transition" << "state :" << state;
    mState = state;
}

void UploadService::sendMsgToObject(QObject *object, uint16_t code, QVariantMap map)
{
    map["resultSupplier"] = "UploadService";
    if (object) {
        MSEvent *event = new MSEvent(this, code);
        event->setData(QVariant(map));
        QCoreApplication::postEvent(object, event);
    } else {
        qWarning() << __func__ << "Not Object to send.";
    }
}

bool UploadService::uploadServiceEventHandler(MSEvent *e)
{
    bool result = true;
    uint16_t event = e->getMSEventType();
    qDebug("%s 0x%04x state:%d", __func__, event, mState);

    switch (event) {
    case MSEvent::EVENT_TYPE_UPLOAD_REQ: {
        if (mState == STATE_TYPE_IDLE) {
            QVariantMap map = e->getData().toMap();

            mFiles  = map["files"].toStringList();
            mDir    = map["dir"].toString();
            mWebDir = map["webdir"].toString();
            mScript = map["script"].toString();
            if (mFiles.size() == 0) {
                qWarning() << "Invalid mFiles";
                break;
            }
            if (mWebDir.length() == 0) {
                qWarning() << "Invalid mWebDir";
                break;
            }

            // upload();
            uploadChunks();
            mUploadReqSender = e->getSender();
            transition(STATE_TYPE_UPLOADING);
        } else {
            qDebug() << "Error State";
        }
        result = true;
        break;
    }
    default:
        qWarning("%s Unknow Event Type 0x%04x", __func__, event);
    }

    return result;
}

bool UploadService::event(QEvent *e)
{
    uint16_t eventType = 0x0000;

    if (e->type() == MSEvent::MSEVENT_BASE_TYPE) {
        MSEvent *event = static_cast<MSEvent *>(e);
        return uploadServiceEventHandler(event);
    }
    return QObject::event(e);
}
