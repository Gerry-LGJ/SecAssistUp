#include "loginservice.h"

#include <QDebug>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QNetworkRequest>

#include "../window/loginwindow.h"
#include "../window/mainwindow.h"
#include "mainservice.h"

LoginService::LoginService(QObject *parent)
    : QObject{parent}
{
    mNetwork = Network::getInstance();
    mLoginReqCallable = new NetworkCallable(this);
    mLoginWindow = LoginWindow::getInstance();
    mMainWindow = MainWindow::getInstance();
}

void LoginService::init()
{
    mLoginReqSender = nullptr;
    initLoginNetworkCallable();
    transition(STATE_TYPE_LOGOUT);
}

void LoginService::initLoginNetworkCallable()
{
    connect(mLoginReqCallable, &NetworkCallable::start, this, [=] {
        mLoginRequesting = true;
    });
    connect(mLoginReqCallable, &NetworkCallable::finish, this, [=] {
        mLoginRequesting = false;
    });
    connect(mLoginReqCallable, &NetworkCallable::error, this, [=] (int status, QString errorString, QString result) {

        qDebug() << "initLoginNetworkCallable" << "error " <<
            "status:" << status <<
            "errorString:" << errorString <<
            "result:" << result;
        QVariantMap map;
        map["resultCode"]  = MSEvent::RESULT_CODE_NETWORK_ONERROR;
        map["status"]      = status;
        map["errorString"] = errorString;
        map["result"]      = result;
        sendMsgToLoginSender(MSEvent::EVENT_TYPE_LOGIN_CFM, map);

        transition(STATE_TYPE_LOGOUT);
    });
    connect(mLoginReqCallable, &NetworkCallable::success, this, [=] (QString result) {
        QString website_user = extractUsername(result);
        qDebug() << QString("Get Website UserName[%1]: %2").arg(website_user.length()).arg(website_user);
        if (website_user.length() > 0) {

            QVariantMap map;
            map["resultCode"] = MSEvent::RESULT_CODE_SUCCESS;
            sendMsgToLoginSender(MSEvent::EVENT_TYPE_LOGIN_CFM, map);
            sendMsgToObject(MainService::getInstance(), MSEvent::EVENT_TYPE_LOGIN_CFM, map);

            transition(STATE_TYPE_LOGIN);
        } else {
            // 尝试获取web返回的错误消息
            QVariantMap map;
            QString msg = extractDivContent(result, "msg");
            map["resultCode"] = MSEvent::RESULT_CODE_FAIL;
            if (msg.length() > 0) {
                qWarning() << "Login Error:" << msg;
                map["msg"] = msg;
            }
            sendMsgToLoginSender(MSEvent::EVENT_TYPE_LOGIN_CFM, map);

            transition(STATE_TYPE_LOGOUT);
        }
    });
}

bool LoginService::loginServiceEventHandler(MSEvent *e)
{
    bool result = true;
    uint16_t event = e->getMSEventType();
    qDebug("%s 0x%04x state:%d", __func__, event, mState);
    switch (event) {
    case MSEvent::EVENT_TYPE_LOGIN_REQ: {
        if (mState == STATE_TYPE_LOGOUT) {
            QString username, password, weburl;
            QVariantMap map = e->getData().toMap();

            username = map["username"].toString();
            password = map["password"].toString();
            weburl   = map["weburl"]  .toString();

            login(username, password, weburl);
            mLoginReqSender = e->getSender();
            transition(STATE_TYPE_LOGGING_IN);
        } else {
            qDebug() << "Error State";
        }
        result = true;
        break;
    }
    case MSEvent::EVENT_TYPE_LOGOUT_REQ: {
        transition(STATE_TYPE_LOGGING_IN);
        QVariantMap map;
        map["resultCode"] = MSEvent::RESULT_CODE_SUCCESS;
        sendMsgToObject(e->getSender(), MSEvent::EVENT_TYPE_LOGOUT_CFM, map);
        transition(STATE_TYPE_LOGOUT);
        sendMsgToObject(MainService::getInstance(), MSEvent::EVENT_TYPE_LOGOUT_CFM, map);
        break;
    }
    default:
        qWarning("%s Unknow Event Type 0x%04x", __func__, event);
    }

    return result;
}

bool LoginService::login(QString username, QString password, QString weburl)
{
    qDebug() <<
        "LoginService::login()" <<
        "username:" << username <<
        "password:" << password <<
        "weburl:" << weburl;
    if (username.length() <= 0 ||
        password.length() <= 0 ||
        weburl.length() <= 0) {
        qWarning() << QString("Invalid string length, abort login. [%1,%2,%3]")
            .arg(username.length(), password.length(), weburl.length());
    }
    // start request website
    QString req_url = QString("%1/index.php?user/loginSubmit&name=%2&check_code=undefined&password=%3&rember_password=0")
        .arg(weburl, username, password);
    qDebug() << "url=" << req_url;

    mNetwork->get(req_url)
    ->addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36 Edg/138.0.0.0")
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ->addAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy)
#endif
    ->bind(this)
    ->go(mLoginReqCallable);

    return true;
}

void LoginService::transition(uint16_t state)
{
    qDebug() << "LoginService::transition" << "state :" << state;
    mState = state;
}

void LoginService::sendMsgToLoginSender(uint16_t code, QVariantMap map)
{
    map["resultSupplier"] = "LoginService";
    if (mLoginReqSender) {
        MSEvent *event = new MSEvent(this, code);
        QVariant var = map;
        event->setData(var);
        QCoreApplication::postEvent(mLoginReqSender, event);
    } else {
        qWarning() << __func__ << "No mLoginReqSender";
    }
}

void LoginService::sendMsgToObject(QObject *object, uint16_t code, QVariantMap map)
{
    map["resultSupplier"] = "LoginService";
    if (object) {
        MSEvent *event = new MSEvent(this, code);
        QVariant var = map;
        event->setData(var);
        QCoreApplication::postEvent(object, event);
    } else {
        qWarning() << __func__ << "Not Object to send.";
    }
}

QString LoginService::extractUsername(const QString &htmlString)
{
    static QRegularExpression pattern("当前用户:\\s*([^<]*)\\s*</div>");
    QRegularExpressionMatch match = pattern.match(htmlString);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }
    return "";
}

QString LoginService::extractDivContent(const QString &htmlString, const QString &className)
{
    // 构建正则表达式，注意转义
    QString pattern = QString(R"(<div class="%1">([^<]*)</div>)").arg(QRegularExpression::escape(className));

    QRegularExpression regex(pattern);
    QRegularExpressionMatch match = regex.match(htmlString);

    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }
    return "";
}

bool LoginService::event(QEvent *e)
{
    uint16_t eventType = 0x0000;

    if (e->type() == MSEvent::MSEVENT_BASE_TYPE) {
        MSEvent *event = static_cast<MSEvent *>(e);
        return loginServiceEventHandler(event);
    }
    return QObject::event(e);
}
