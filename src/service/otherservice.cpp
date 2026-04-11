#include "otherservice.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include "mainservice.h"


OtherService::OtherService(QObject *parent)
    : QObject{parent}
{
    mNetwork        = Network::getInstance();
    mDeleteCallable = new NetworkCallable(this);
    mRenameCallable = new NetworkCallable(this);
    mCutCallable    = new NetworkCallable(this);
    mCopyCallable   = new NetworkCallable(this);
    mPasteCallable  = new NetworkCallable(this);
    mMkdirCallable  = new NetworkCallable(this);
    mLoginWindow    = LoginWindow::getInstance();
    mMainWindow     = MainWindow::getInstance();
    mSettings       = SettingsHelper::getInstance();
}


void OtherService::init()
{
    initCallable();
}

void OtherService::initCallable()
{
    initDeleteCallable();
    initRenameCallable();
    initCutCallable();
    initCopyCallable();;
    initPasteCallable();
    initMkdirCallable();
}

void OtherService::initDeleteCallable()
{
    MainService *mService = MainService::getInstance();

    connect(mDeleteCallable, &NetworkCallable::start, this, [=] {
        mDeleting = true;
    });
    connect(mDeleteCallable, &NetworkCallable::finish, this, [=] {
        mDeleting = false;
    });
    connect(mDeleteCallable, &NetworkCallable::error, this, [=] (int status, QString errorString, QString result) {
        qDebug() << "initDeleteCallable" << "error" <<
            "status:" << status <<
            "errorString:" << errorString <<
            "result:" << result;
        QVariantMap map;
        map["resultCode"]  = MSEvent::RESULT_CODE_NETWORK_ONERROR;
        map["status"]      = status;
        map["errorString"] = errorString;
        map["result"]      = result;
        sendMsgToObject(mSender, MSEvent::EVENT_TYPE_DELETE_CFM, map);

        mDeleting = false;
        transition(STATE_TYPE_IDLE);
    });
    connect(mDeleteCallable, &NetworkCallable::success, this, [=] (QString result) {
        qDebug() << "Delete result:" << result;
        uint16_t resultCode = MSEvent::RESULT_CODE_FAIL;
        QString errorString = "";

        if (result.length() > 0) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8(), &err);

            if (err.error == QJsonParseError::NoError) {
                bool code    = doc.object().value("code").toBool(false);
                QString data = doc.object().value("data").toString();
                if (code) {
                    resultCode = MSEvent::RESULT_CODE_SUCCESS;
                } else {
                    qDebug() << "The server returned that the delete failed. msg:" << data;
                }
                errorString = data;

            } else {
                if (returnLoginPage(mDeleteCallable, result)) {
                    resultCode = MSEvent::RESULT_CODE_SESSION_INVALID;
                    errorString = tr("Server has returned to the login page. Please try to log in again.");
                } else {
                    errorString = err.errorString();
                }
            }
        }

        QVariantMap map;
        map["resultCode"]  = resultCode;
        map["errorString"] = errorString;

        sendMsgToObject(mSender, MSEvent::EVENT_TYPE_DELETE_CFM, map);
        mDeleting          = false;
        transition(STATE_TYPE_IDLE);

    });
}

void OtherService::initRenameCallable()
{
    MainService *mService = MainService::getInstance();

    connect(mRenameCallable, &NetworkCallable::start, this, [=] {
        mRenaming = true;
    });
    connect(mRenameCallable, &NetworkCallable::finish, this, [=] {
        mRenaming = false;
    });
    connect(mRenameCallable, &NetworkCallable::error, this, [=] (int status, QString errorString, QString result) {
        qDebug() << "initRenameCallable" << "error" <<
            "status:" << status <<
            "errorString:" << errorString <<
            "result:" << result;
        QVariantMap map;
        map["resultCode"]  = MSEvent::RESULT_CODE_NETWORK_ONERROR;
        map["status"]      = status;
        map["errorString"] = errorString;
        map["result"]      = result;
        sendMsgToObject(mSender, MSEvent::EVENT_TYPE_RENAME_CFM, map);

        mRenaming = false;
        transition(STATE_TYPE_IDLE);
    });
    connect(mRenameCallable, &NetworkCallable::success, this, [=] (QString result) {
        qDebug() << "Rename result:" << result;
        uint16_t resultCode = MSEvent::RESULT_CODE_FAIL;
        QString errorString = "";

        if (result.length() > 0) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8(), &err);

            if (err.error == QJsonParseError::NoError) {
                bool code    = doc.object().value("code").toBool(false);
                QString data = doc.object().value("data").toString();
                if (code) {
                    resultCode = MSEvent::RESULT_CODE_SUCCESS;
                } else {
                    qDebug() << "The server returned that the rename failed. msg:" << data;
                }
                errorString = data;

            } else {
                if (returnLoginPage(mRenameCallable, result)) {
                    resultCode = MSEvent::RESULT_CODE_SESSION_INVALID;
                    errorString = tr("Server has returned to the login page. Please try to log in again.");
                } else {
                    errorString = err.errorString();
                }
            }
        }

        QVariantMap map;
        map["resultCode"]  = resultCode;
        map["errorString"] = errorString;

        sendMsgToObject(mSender, MSEvent::EVENT_TYPE_RENAME_CFM, map);
        mDeleting          = false;
        transition(STATE_TYPE_IDLE);
    });
}

void OtherService::initCutCallable()
{
    MainService *mService = MainService::getInstance();

    connect(mCutCallable, &NetworkCallable::start, this, [=] {
        mCutting = true;
    });
    connect(mCutCallable, &NetworkCallable::finish, this, [=] {
        mCutting = false;
    });
    connect(mCutCallable, &NetworkCallable::error, this, [=] (int status, QString errorString, QString result) {
        qDebug() << "initCutCallable" << "error" <<
            "status:" << status <<
            "errorString:" << errorString <<
            "result:" << result;
        QVariantMap map;
        map["resultCode"]  = MSEvent::RESULT_CODE_NETWORK_ONERROR;
        map["status"]      = status;
        map["errorString"] = errorString;
        map["result"]      = result;
        sendMsgToObject(mSender, MSEvent::EVENT_TYPE_CUT_CFM, map);

        mCutting = false;
        transition(STATE_TYPE_IDLE);
    });
    connect(mCutCallable, &NetworkCallable::success, this, [=] (QString result) {
        qDebug() << "Cut result:" << result;
        uint16_t resultCode = MSEvent::RESULT_CODE_FAIL;
        QString errorString = "";

        if (result.length() > 0) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8(), &err);

            if (err.error == QJsonParseError::NoError) {
                bool code    = doc.object().value("code").toBool(false);
                QString data = doc.object().value("data").toString();
                if (code) {
                    resultCode = MSEvent::RESULT_CODE_SUCCESS;
                } else {
                    qDebug() << "The server returned that the cut failed. msg:" << data;
                }
                errorString = data;

            } else {
                if (returnLoginPage(mCutCallable, result)) {
                    resultCode = MSEvent::RESULT_CODE_SESSION_INVALID;
                    errorString = tr("Server has returned to the login page. Please try to log in again.");
                } else {
                    errorString = err.errorString();
                }
            }
        }

        QVariantMap map;
        map["resultCode"]  = resultCode;
        map["errorString"] = errorString;

        sendMsgToObject(mSender, MSEvent::EVENT_TYPE_CUT_CFM, map);
        mCutting          = false;
        transition(STATE_TYPE_IDLE);
    });
}

void OtherService::initCopyCallable()
{
    MainService *mService = MainService::getInstance();

    connect(mCopyCallable, &NetworkCallable::start, this, [=] {
        mCopying = true;
    });
    connect(mCopyCallable, &NetworkCallable::finish, this, [=] {
        mCopying = false;
    });
    connect(mCopyCallable, &NetworkCallable::error, this, [=] (int status, QString errorString, QString result) {
        qDebug() << "initCopyCallable" << "error" <<
            "status:" << status <<
            "errorString:" << errorString <<
            "result:" << result;
        QVariantMap map;
        map["resultCode"]  = MSEvent::RESULT_CODE_NETWORK_ONERROR;
        map["status"]      = status;
        map["errorString"] = errorString;
        map["result"]      = result;
        sendMsgToObject(mSender, MSEvent::EVENT_TYPE_COPY_CFM, map);

        mCopying = false;
        transition(STATE_TYPE_IDLE);
    });
    connect(mCopyCallable, &NetworkCallable::success, this, [=] (QString result) {
        qDebug() << "Copy result:" << result;
        uint16_t resultCode = MSEvent::RESULT_CODE_FAIL;
        QString errorString = "";

        if (result.length() > 0) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8(), &err);

            if (err.error == QJsonParseError::NoError) {
                bool code    = doc.object().value("code").toBool(false);
                QString data = doc.object().value("data").toString();
                if (code) {
                    resultCode = MSEvent::RESULT_CODE_SUCCESS;
                } else {
                    qDebug() << "The server returned that the copy failed. msg:" << data;
                }
                errorString = data;

            } else {
                if (returnLoginPage(mCopyCallable, result)) {
                    resultCode = MSEvent::RESULT_CODE_SESSION_INVALID;
                    errorString = tr("Server has returned to the login page. Please try to log in again.");
                } else {
                    errorString = err.errorString();
                }
            }
        }

        QVariantMap map;
        map["resultCode"]  = resultCode;
        map["errorString"] = errorString;

        sendMsgToObject(mSender, MSEvent::EVENT_TYPE_COPY_CFM, map);
        mCopying          = false;
        transition(STATE_TYPE_IDLE);
    });
}

void OtherService::initPasteCallable()
{
    MainService *mService = MainService::getInstance();

    connect(mPasteCallable, &NetworkCallable::start, this, [=] {
        mPasting = true;
    });
    connect(mPasteCallable, &NetworkCallable::finish, this, [=] {
        mPasting = false;
    });
    connect(mPasteCallable, &NetworkCallable::error, this, [=] (int status, QString errorString, QString result) {
        qDebug() << "initPasteCallable" << "error" <<
            "status:" << status <<
            "errorString:" << errorString <<
            "result:" << result;
        QVariantMap map;
        map["resultCode"]  = MSEvent::RESULT_CODE_NETWORK_ONERROR;
        map["status"]      = status;
        map["errorString"] = errorString;
        map["result"]      = result;
        sendMsgToObject(mSender, MSEvent::EVENT_TYPE_PASTE_CFM, map);

        mPasting = false;
        transition(STATE_TYPE_IDLE);
    });
    connect(mPasteCallable, &NetworkCallable::success, this, [=] (QString result) {
        qDebug() << "Paste result:" << result;
        uint16_t resultCode = MSEvent::RESULT_CODE_FAIL;
        QString errorString = "";
        QStringList           infos;

        if (result.length() > 0) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8(), &err);

            if (err.error == QJsonParseError::NoError) {
                bool code    = doc.object().value("code").toBool(false);
                QString data = doc.object().value("data").toString();
                if (code) {
                    resultCode = MSEvent::RESULT_CODE_SUCCESS;

                    QJsonArray arr = doc.object().value("info").toArray();
                    for (int i = 0; i < arr.size(); ++i) {
                        infos.append(arr.at(i).toString());
                    }

                } else {
                    qDebug() << "The server returned that the paste failed. msg:" << data;
                }
                errorString = data;

            } else {
                if (returnLoginPage(mPasteCallable, result)) {
                    resultCode = MSEvent::RESULT_CODE_SESSION_INVALID;
                    errorString = tr("Server has returned to the login page. Please try to log in again.");
                } else {
                    errorString = err.errorString();
                }
            }
        }

        QVariantMap map;
        map["resultCode"]  = resultCode;
        map["errorString"] = errorString;
        if (resultCode == MSEvent::RESULT_CODE_SUCCESS && !infos.isEmpty()) {
            map["info"] = infos;
        }

        sendMsgToObject(mSender, MSEvent::EVENT_TYPE_PASTE_CFM, map);
        mPasting          = false;
        transition(STATE_TYPE_IDLE);
    });

}

void OtherService::initMkdirCallable()
{
    MainService *mService = MainService::getInstance();

    connect(mMkdirCallable, &NetworkCallable::start, this, [=] {
        mMkdiring = true;
    });
    connect(mMkdirCallable, &NetworkCallable::finish, this, [=] {
        mMkdiring = false;
    });
    connect(mMkdirCallable, &NetworkCallable::error, this, [=] (int status, QString errorString, QString result) {
        qDebug() << "initMkdirCallable" << "error" <<
            "status:" << status <<
            "errorString:" << errorString <<
            "result:" << result;
        QVariantMap map;
        map["resultCode"]  = MSEvent::RESULT_CODE_NETWORK_ONERROR;
        map["status"]      = status;
        map["errorString"] = errorString;
        map["result"]      = result;
        sendMsgToObject(mSender, MSEvent::EVENT_TYPE_MKDIR_CFM, map);

        mMkdiring = false;
        transition(STATE_TYPE_IDLE);
    });
    connect(mMkdirCallable, &NetworkCallable::success, this, [=] (QString result) {
        qDebug() << "Mkdir result:" << result;
        uint16_t resultCode = MSEvent::RESULT_CODE_FAIL;
        QString errorString = "";

        if (result.length() > 0) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8(), &err);

            if (err.error == QJsonParseError::NoError) {
                bool code    = doc.object().value("code").toBool(false);
                QString data = doc.object().value("data").toString();
                if (code) {
                    resultCode = MSEvent::RESULT_CODE_SUCCESS;
                } else {
                    qDebug() << "The server returned that the copy failed. msg:" << data;
                }
                errorString = data;

            } else {
                if (returnLoginPage(mMkdirCallable, result)) {
                    resultCode = MSEvent::RESULT_CODE_SESSION_INVALID;
                    errorString = tr("Server has returned to the login page. Please try to log in again.");
                } else {
                    errorString = err.errorString();
                }
            }
        }

        QVariantMap map;
        map["resultCode"]  = resultCode;
        map["errorString"] = errorString;
        sendMsgToObject(mSender, MSEvent::EVENT_TYPE_MKDIR_CFM, map);
        mMkdiring          = false;
        transition(STATE_TYPE_IDLE);
    });
}

bool OtherService::otherServiceEventHandler(MSEvent *e)
{
    uint16_t event      = e->getMSEventType();
    mSender             = e->getSender();
    QVariantMap map     = e->getData().toMap();
    qDebug("%s 0x%04x state:%d", __func__, event, mState);

    switch (event) {
    case MSEvent::EVENT_TYPE_DELETE_REQ: {
        if (mState == STATE_TYPE_IDLE) {

            QList<del_info_t> deleteList = map["list"].value<QList<del_info_t>>();
            if (deleteList.isEmpty()) {
                qWarning() << "invalid 'list' to delete files.";
                break;
            }

            del(deleteList);
            transition(STATE_TYPE_DELETING);

        } else {
            qDebug() << "Error State";
        }
        break;
    }
    case MSEvent::EVENT_TYPE_RENAME_REQ: {
        if (mState == STATE_TYPE_IDLE) {

            QString         path, rname_to;
            path          = map["path"].toString();
            rname_to      = map["rname_to"].toString();
            if (path.length() == 0 || rname_to.length() == 0) {
                qWarning() << "Invalid 'path' or 'rname_to'";
                break;
            }

            rename(path, rname_to);
            transition(STATE_TYPE_RENAMING);

        } else {
            qDebug() << "Error State";
        }
        break;
    }
    case MSEvent::EVENT_TYPE_CUT_REQ: {
        if (mState == STATE_TYPE_IDLE) {

            QList<cut_info_t> cutList = map["list"].value<QList<cut_info_t>>();
            if (cutList.isEmpty()) {
                qWarning() << "invalid 'list' to cut files.";
                break;
            }

            cut(cutList);
            transition(STATE_TYPE_CUTTING);

        } else {
            qDebug() << "Error State";
        }
        break;
    }
    case MSEvent::EVENT_TYPE_COPY_REQ: {
        if (mState == STATE_TYPE_IDLE) {

            QList<copy_info_t> copyList = map["list"].value<QList<copy_info_t>>();
            if (copyList.isEmpty()) {
                qWarning() << "invalid 'list' to cut files.";
                break;
            }

            copy(copyList);
            transition(STATE_TYPE_COPYING);

        } else {
            qDebug() << "Error State";
        }
        break;
    }
    case MSEvent::EVENT_TYPE_PASTE_REQ: {
        if (mState == STATE_TYPE_IDLE) {

            QString path = map["path"].toString();
            if (path.length() == 0) {
                qWarning() << "invalid 'path' to paste files.";
                break;
            }

            paste(path);
            transition(STATE_TYPE_PASTING);

        } else {
            qDebug() << "Error State";
        }
        break;
    }
    case MSEvent::EVENT_TYPE_MKDIR_REQ: {

        if (mState == STATE_TYPE_IDLE) {

            QString path = map["path"].toString();
            if (path.length() == 0) {
                qWarning() << "invalid 'path' to mkdir.";
                break;
            }

            mkdir(path);
            transition(STATE_TYPE_MKDIRING);

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

void OtherService::transition(uint16_t state)
{
    qDebug() << "OtherService::transition" << "state :" << state;
    mState = state;
}

bool OtherService::del(QList<del_info_t> &list)
{
    QJsonArray jsonArray;
    for (const auto &item : list) {
        QJsonObject obj;
        obj["type"] = item.type;
        obj["path"] = item.path;
        jsonArray.append(obj);
    }
    QJsonDocument       doc(jsonArray);
    QString listValue = doc.toJson(QJsonDocument::Compact);
    QString url       = mSettings->getWebUrl() + "/index.php?explorer/pathDelete";

    qDebug() << __func__ << "url:" << url;
    qDebug() << __func__ << "listValue:" << listValue;

    mNetwork->postForm(url)
    ->add("list", listValue)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ->addAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy)
#endif
    ->bind(this)
    ->go(mDeleteCallable);

    return true;
}

bool OtherService::rename(const QString &path, const QString &rname_to)
{
    QString url = mSettings->getWebUrl() + "/index.php?explorer/pathRname";

    qDebug() << __func__ << "url:" << url << "path:" << path << "rname_to:" << rname_to;

    mNetwork->postForm(url)
    ->add("path", path)
    ->add("rname_to", rname_to)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ->addAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy)
#endif
    ->bind(this)
    ->go(mRenameCallable);

    return true;
}

bool OtherService::cut(const QList<cut_info_t> &list)
{
    QJsonArray jsonArray;
    for (const auto &item : list) {
        QJsonObject obj;
        obj["type"] = item.type;
        obj["path"] = item.path;
        jsonArray.append(obj);
    }
    QJsonDocument       doc(jsonArray);
    QString listValue = doc.toJson(QJsonDocument::Compact);
    QString url       = mSettings->getWebUrl() + "/index.php?explorer/pathCute";

    qDebug() << __func__ << "url:" << url;
    qDebug() << __func__ << "listValue:" << listValue;

    mNetwork->postForm(url)
    ->add("list", listValue)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ->addAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy)
#endif
    ->bind(this)
    ->go(mCutCallable);

    return true;
}

bool OtherService::copy(const QList<copy_info_t> &list)
{
    QJsonArray jsonArray;
    for (const auto &item : list) {
        QJsonObject obj;
        obj["type"] = item.type;
        obj["path"] = item.path;
        jsonArray.append(obj);
    }
    QJsonDocument       doc(jsonArray);
    QString listValue = doc.toJson(QJsonDocument::Compact);
    QString url       = mSettings->getWebUrl() + "/index.php?explorer/pathCopy";

    qDebug() << __func__ << "url:" << url;
    qDebug() << __func__ << "listValue:" << listValue;

    mNetwork->postForm(url)
    ->add("list", listValue)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ->addAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy)
#endif
    ->bind(this)
    ->go(mCopyCallable);

    return true;
}

bool OtherService::paste(const QString &path)
{
    QString url = mSettings->getWebUrl() + QString("/index.php?explorer/pathPast&path=%1").arg(path);

    qDebug() << __func__ << "url:" << url;

    mNetwork->get(url)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ->addAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy)
#endif
    ->bind(this)
    ->go(mPasteCallable);

    return true;
}

bool OtherService::mkdir(const QString &path)
{

    QString url = mSettings->getWebUrl() + QString("/index.php?explorer/mkdir&path=%1").arg(path);

    qDebug() << __func__ << "url:" << url;

    mNetwork->get(url)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ->addAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy)
#endif
    ->bind(this)
    ->go(mMkdirCallable);

    return true;
}

bool OtherService::returnLoginPage(NetworkCallable *callable, QString &result)
{
    // 尝试判断是否返回了登录页
    bool isHtml         = false;
    bool isSecError     = false;
    QString contentType = callable->replyGetHeader("Content-Type");

    if (contentType.length() > 0 && contentType.contains("text/html")) {

        isHtml = DownloadService::isHtmlContent(result);

        if (isHtml && result.contains("文件防泄密网关") &&
            result.contains("用户名") &&
            result.contains("密码") &&
            result.contains("登录")) {
            isSecError = true;

        }
    }
    return isSecError;
}

void OtherService::sendMsgToObject(QObject *object, uint16_t code, QVariantMap map)
{
    map["resultSupplier"] = "OtherService";
    if (object) {
        MSEvent *event = new MSEvent(this, code);
        event->setData(map);
        QCoreApplication::postEvent(object, event);
    }
}

bool OtherService::event(QEvent *e)
{
    uint16_t eventType = 0x0000;

    if (e->type() == MSEvent::MSEVENT_BASE_TYPE) {
        MSEvent *event = static_cast<MSEvent *>(e);
        return otherServiceEventHandler(event);
    }
    return QObject::event(e);
}
