#include "refreshservice.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

RefreshService::RefreshService(QObject *parent)
    : QObject{parent}
{
    mNetwork = Network::getInstance();
    mRefreshReqCallable = new NetworkCallable(this);
    mLoginWindow = LoginWindow::getInstance();
    mMainWindow = MainWindow::getInstance();
    mSettingsHelper = SettingsHelper::getInstance();
}


void RefreshService::init()
{
    mCountdownTimer = new QTimer(this);
    mCountdownTimer->setSingleShot(false);
    mCountdownTimer->setInterval(1000);
    connect(mCountdownTimer, &QTimer::timeout, this, [=] {
        mCountdown--;
        if (mCountdown == 0) {
            qDebug() << "The countdown has ended. The refresh process has begun.";
            mCountdownTimer->stop();
            refresh(mPathList);
        } else {
            setCountdownLabelText(QString("%1").arg(mCountdown));
        }
    });
    mPathList << "/";
    mFileList.clear();
    initRefreshNetworkCallable();
    mRefreshReqSender = nullptr;
    transition(STATE_TYPE_IDLE);
}

QList<file_t> RefreshService::getFileList()
{
    return mFileList;
}

QStringList RefreshService::getPathList()
{
    return mPathList;
}

bool RefreshService::event(QEvent *e)
{
    uint16_t eventType = 0x0000;

    if (e->type() == MSEvent::MSEVENT_BASE_TYPE) {
        MSEvent *event = static_cast<MSEvent *>(e);
        return refreshServiceEventHandler(event);
    }
    return QObject::event(e);
}

bool RefreshService::refreshServiceEventHandler(MSEvent *e)
{
    bool ret = true;
    uint16_t event = e->getMSEventType();
    qDebug("%s 0x%04x state:%d", __func__, event, mState);
    switch (event) {
    case MSEvent::EVENT_TYPE_REFRESH_REQ: {
        if (mState == STATE_TYPE_IDLE) {

            mRefreshReqSender = e->getSender();
            refresh(mPathList);

        } else {
            qDebug() << "Error State";
        }
        break;
    }
    case MSEvent::EVENT_TYPE_ENTRY_FOLDER_REQ: {
        if (mState == STATE_TYPE_IDLE) {
            QString name;
            QVariantMap map = e->getData().toMap();

            mRefreshReqSender = e->getSender();
            name = map["name"].toString();

            entryFolder(name);

        } else {
            qDebug() << "Error State";
        }
        break;
    }
    case MSEvent::EVENT_TYPE_RETURN_PARENT_DIR_REQ: {
        if (mState == STATE_TYPE_IDLE) {

            mRefreshReqSender = e->getSender();
            returnParentDirectory();

        } else {
            qDebug() << "Error State";
        }
        break;
    }
    default:
        break;
    }
    return true;
}

QString RefreshService::getPathByPathList(QStringList pathList)
{
    QString ret;
    if (pathList.length() == 1 && pathList.at(0) == "/") {
        // /
        ret = "/";
    } else if (pathList.length() == 2 && pathList.at(0) == "/") {
        // /a
        if (pathList.at(1).length() > 0) {
            ret = "/" + pathList.at(1);
        }
    } else if (pathList.length() > 2 && pathList.at(0) == "/") {
        // /a/b
        ret = "/" + pathList.at(1);
        for (int i = 2; i < pathList.size(); ++i) {
            if (pathList.at(i).length() > 0) {
                ret += "/" + pathList.at(i);
            }
        }
    }
    return ret;
}

bool RefreshService::onParseJsonToList(QString result)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        qDebug() << __func__ << "Json Parse Error:" << err.errorString();
        return false;
    }
    QJsonObject dataObj = doc.object().value("data").toObject();
    if (dataObj.empty()) {
        qDebug() << __func__ << "dataObj is empty:";
        return false;
    }
    mFileList.clear();
    /* 先解析文件类型 */
    QJsonArray fileArray = dataObj.value("filelist").toArray();
    for (int i = 0; i < fileArray.size(); ++i) {
        file_t f;
        QJsonObject obj = fileArray.at(i).toObject();
        f.name          = obj.value("name").toString();
        f.path          = obj.value("path").toString();
        f.type          = obj.value("type").toString();
        f.isFile        = true;
        f.mode          = obj.value("mode").toString();
        f.atime         = obj.value("atime").toVariant().toULongLong();
        f.ctime         = obj.value("ctime").toVariant().toULongLong();
        f.mtime         = obj.value("mtime").toVariant().toULongLong();
        f.is_readable   = obj.value("is_readable").toInt();
        f.is_writeable  = obj.value("is_writeable").toInt();
        f.size          = obj.value("size").toVariant().toULongLong();
        f.size_friendly = obj.value("size_friendly").toString();
        f.ext           = obj.value("ext").toString();
        mFileList.append(f);
    }
    /* 然后解析文件夹 */
    QJsonArray folderArray = dataObj.value("folderlist").toArray();
    for (int i = 0; i < folderArray.size(); ++i) {
        file_t d;
        QJsonObject obj = folderArray.at(i).toObject();
        d.name          = obj.value("name").toString();
        d.path          = obj.value("path").toString();
        d.type          = obj.value("type").toString();
        d.isFile        = false;
        d.mode          = obj.value("mode").toString();
        d.atime         = obj.value("atime").toVariant().toULongLong();
        d.ctime         = obj.value("ctime").toVariant().toULongLong();
        d.mtime         = obj.value("mtime").toVariant().toULongLong();
        d.is_readable   = obj.value("is_readable").toInt();
        d.is_writeable  = obj.value("is_writeable").toInt();\
        /* is folder, so default value */
        d.size          = 0;
        d.size_friendly = "";
        d.ext           = "";
        mFileList.append(d);
    }
    return true;
}

void RefreshService::restartCountdown()
{
    mCountdown = mSettingsHelper->getHeartbeatInterval();
    if (mCountdown <= 0) {
        qDebug() << __func__ << "cancel auto refresh";
        setCountdownLabelText("");
        mCountdownTimer->stop();
    } else {
        setCountdownLabelText(QString("%1").arg(mCountdown));
        mCountdownTimer->start();
    }
}

void RefreshService::stopCountdown()
{
    setCountdownLabelText("");
    mCountdownTimer->stop();
}

void RefreshService::setCountdownLabelText(QString text)
{
    mMainWindow->setCountdownLabelText(text);
}

void RefreshService::initRefreshNetworkCallable()
{
    connect(mRefreshReqCallable, &NetworkCallable::start, this, [=] {
        mRefreshRequesting = true;
        transition(STATE_TYPE_REFRESHING);
    });
    connect(mRefreshReqCallable, &NetworkCallable::finish, this, [=] {
        mRefreshRequesting = false;
        transition(STATE_TYPE_IDLE);
    });
    connect(mRefreshReqCallable, &NetworkCallable::error, this, [=] (int status, QString errorString, QString result) {

        qDebug() << "initRefreshNetworkCallable error" <<
            "status:" << status <<
            "errorString:" << errorString <<
            "result:" << result;
        QVariantMap map;
        map["resultCode"]  = MSEvent::RESULT_CODE_NETWORK_ONERROR;
        map["status"]      = status;
        map["errorString"] = errorString;
        map["result"]      = result;
        sendMsgToObject(mRefreshReqSender, MSEvent::EVENT_TYPE_REFRESH_CFM, map);

    });
    connect(mRefreshReqCallable, &NetworkCallable::success, this, [=] (QString result) {

        qDebug() << "RefreshService::initRefreshNetworkCallable\n" << result;
        if (onParseJsonToList(result)) {
            mPathList = mRequestPathList;
            // return succees message
            QVariantMap map;
            map["resultCode"] = MSEvent::RESULT_CODE_SUCCESS;
            map["msg"]        = "Success";
            sendMsgToObject(mRefreshReqSender, MSEvent::EVENT_TYPE_REFRESH_CFM, map);
            sendMsgToObject(mMainWindow, MSEvent::EVENT_TYPE_REFRESH_IND, map);
            restartCountdown();
        } else {
            // return fail message
            QVariantMap map;
            map["resultCode"] = MSEvent::RESULT_CODE_FAIL;
            map["msg"]        = "Json Parse Error";
            sendMsgToObject(mRefreshReqSender, MSEvent::EVENT_TYPE_REFRESH_CFM, map);
        }

    });
}

bool RefreshService::refresh(QStringList requestPathList)
{
    mRequestPathList = requestPathList;
    QString reqPath  = getPathByPathList(requestPathList);
    QString webUrl   = mSettingsHelper->getWebUrl();
    QString url      = webUrl + QString("/index.php?explorer/pathList&path=%1").arg(reqPath);
    qDebug() << "url:" << url;
    mNetwork->get(url)
    ->addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36 Edg/138.0.0.0")
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ->addAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy)
#endif
    ->bind(this)
    ->go(mRefreshReqCallable);
    return true;
}

bool RefreshService::entryFolder(QString name)
{
    if (name.length() <= 0) {
        qWarning() << __func__ << "Invalid Name";
        return false;
    }
    QStringList mList = mPathList;
    mList << name;
    refresh(mList);
    return true;
}

bool RefreshService::returnParentDirectory()
{
    QStringList mList = mPathList;
    if (mList.size() == 1) {
        qDebug() << __func__ << "Has returned to the top level.";
        return false;
    }

    mList.removeLast();
    refresh(mList);
    return true;
}

void RefreshService::transition(uint16_t state)
{
    qDebug() << "RefreshService::transition" << "state :" << state;
    mState = state;
}

void RefreshService::sendMsgToObject(QObject *object, uint16_t code, QVariantMap map)
{
    map["resultSupplier"] = "RefreshService";
    if (object) {
        MSEvent *event = new MSEvent(this, code);
        QVariant var = map;
        event->setData(var);
        QCoreApplication::postEvent(object, event);
    } else {
        qDebug() << "invalid object to reciver.";
    }
}
