#ifndef LOGINSERVICE_H
#define LOGINSERVICE_H

#include <QObject>

#include "../helper/network.h"
#include "../common/singleton.h"
#include "../window/loginwindow.h"
#include "../window/mainwindow.h"
#include "msevent.h"

class LoginService : public QObject
{
    Q_OBJECT
public:
    enum {
        STATE_TYPE_LOGOUT,
        STATE_TYPE_LOGGING_IN,
        STATE_TYPE_LOGIN,
        STATE_TYPE_LOGGING_OUT
    };
    enum {
        MAX_USERNAME_LENGTH = 128,
        MAX_PASSWORD_LENGTH = 128,
        MAX_WEBURL_LENGTH   = 128
    };

    SINGLETON(LoginService)
    void init();

protected:
    bool event(QEvent *e);

private:
    uint16_t         mState;
    bool             mLoginRequesting;
    QObject         *mLoginReqSender;
    Network         *mNetwork;
    NetworkCallable *mLoginReqCallable;
    LoginWindow     *mLoginWindow;
    MainWindow      *mMainWindow;

    // func
    explicit LoginService(QObject *parent = nullptr);
    bool loginServiceEventHandler(MSEvent *e);
    bool login(QString username, QString password, QString weburl);
    void transition(uint16_t state);
    void sendMsgToLoginSender(uint16_t code, QVariantMap map);
    void sendMsgToObject(QObject *object, uint16_t code, QVariantMap map);
    QString extractUsername(const QString &htmlString);
    QString extractDivContent(const QString& htmlString, const QString& className);
    void initLoginNetworkCallable();
};

#endif // LOGINSERVICE_H
