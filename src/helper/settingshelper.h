#ifndef SETTINGSHELPER_H
#define SETTINGSHELPER_H

#include <QObject>
#include <QFileInfo>
#include <QScopedPointer>
#include <QSettings>
#include <QNetworkProxy>
#include <QStandardPaths>

#include "../common/singleton.h"

/* define prase string */
#define USER_WEB_URL            "USER/web_url"
#define USER_NAME               "USER/name"
#define USER_PASSWORD           "USER/password"
#define USER_AUTO_LOGIN         "USER/auto_login"
#define USER_SAVE_PASSWORD      "USER/save_password"
#define USER_PHPSESSID          "USER/PHPSESSID"
#define USER_COOKIES            "USER/cookies"
#define USER_HEARTBEAT_INTERNAL "USER/heartbeat_interval"
#define USER_LAST_SELECT_PID    "USER/last_select_pid"
#define APP_LANGUAGE            "APP/language"
#define APP_AGENT_MODE          "APP/agent_mode"
#define APP_AGENT_SERVER        "APP/agent_server"
#define APP_AGENT_PORT          "APP/agent_port"
#define APP_AGENT_USERNAME      "APP/agent_username"
#define APP_AGENT_PASSWORD      "APP/agent_password"
#define APP_TOUR                "APP/tour"
#define APP_DOWNLOAD_MODE       "APP/download_mode"
#define APP_DOWNLOAD_DIR        "APP/download_dir"
#define APP_DOWNLOAD_OVERRIDE   "APP/download_override"
#define APP_ACCENT_COLOR        "APP/accent_color"
#define APP_NATIVE_TEXT         "APP/native_text"
#define APP_ANIMATION_ENABLED   "APP/animation_enabled"
#define APP_DEBOUND_DELAY       "APP/debound_delay"
#define WINDOW_BLUR             "WINDOW/blur"
#define WINDOW_OPACITY          "WINDOW/opacity"
#define WINDOW_BLUR_RADIUS      "WINDOW/blur_radius"
#define WINDOW_EFFECT           "WINDOW/effect"
#define DEBUG_ENABLE            "DEBUG/enable"

class SettingsHelper : public QObject
{
    Q_OBJECT
private:
    explicit SettingsHelper(QObject *parent = nullptr);

public:
    SINGLETON(SettingsHelper);
    ~SettingsHelper() override;
    void init(char *argv[]);
    void autoSetDefaultConfig(QString filePath);
    Q_INVOKABLE QString getIniFilePath();


    // USER_WEB_URL
    Q_INVOKABLE void saveWebUrl(QString weburl) {
        setValue(USER_WEB_URL, weburl);
    }
    Q_INVOKABLE QString getWebUrl() {
        return value(USER_WEB_URL, QVariant("")).toString();
    }


    // USER_NAME
    Q_INVOKABLE void saveUserName(QString name) {
        setValue(USER_NAME, name);
    }
    Q_INVOKABLE QString getUserName() {
        return value(USER_NAME, QVariant("")).toString();
    }


    // USER_PASSWORD
    Q_INVOKABLE void saveUserPassword(QString password) {
        setValue(USER_PASSWORD, password);
    }
    Q_INVOKABLE QString getUserPassword() {
        return value(USER_PASSWORD, QVariant("")).toString();
    }


    // USER_AUTO_LOGIN
    Q_INVOKABLE void saveAutoLogin(bool autoLogin) {
        setValue(USER_AUTO_LOGIN, autoLogin);
    }
    Q_INVOKABLE bool getAutoLogin() {
        return value(USER_AUTO_LOGIN, QVariant(false)).toBool();
    }


    // USER_SAVE_PASSWORD
    Q_INVOKABLE void saveSavePassword(bool savePassword) {
        setValue(USER_SAVE_PASSWORD, savePassword);
    }
    Q_INVOKABLE bool getSavePassword() {
        return value(USER_SAVE_PASSWORD, QVariant(false)).toBool();
    }


    // USER_PHPSESSID
    Q_INVOKABLE void savePHPSESSID(QString phpsessid) {
        setValue(USER_PHPSESSID, phpsessid);
    }
    Q_INVOKABLE QString getPHPSESSID() {
        return value(USER_PHPSESSID, QVariant("")).toString();
    }


    // USER_COOKIES
    // void saveUserCookies(QVariant cookies) {
    //     setValue(USER_COOKIES, cookies);
    // }
    // QVariant getUserCookies() {
    //     return value(USER_COOKIES, QVariant());
    // }


    // USER_HEARTBEAT_INTERNAL
    Q_INVOKABLE void saveHeartbeatInterval(unsigned int interval) {
        setValue(USER_HEARTBEAT_INTERNAL, interval);
    }
    Q_INVOKABLE unsigned int getHeartbeatInterval() {
        return value(USER_HEARTBEAT_INTERNAL, QVariant(300)).toInt();
    }

    // USER_LAST_SELECT_PID
    Q_INVOKABLE void saveLastSelectPid(QString pid) {
        setValue(USER_LAST_SELECT_PID, pid);
    }
    Q_INVOKABLE QString getLastSelectPid() {
        return value(USER_LAST_SELECT_PID, QVariant("")).toString();
    }


    // APP_LANGUAGE
    Q_INVOKABLE void saveLanguage(const QString &language) {
        setValue(APP_LANGUAGE, language);
    }
    Q_INVOKABLE QString getLanguage(const QVariant def = QVariant("en_US")) {
        return value(APP_LANGUAGE, def).toString();
    }


    // APP_AGENT_MODE
    Q_INVOKABLE void saveAppAgentMode(unsigned int mode) {
        setValue(APP_AGENT_MODE, mode);
    }
    Q_INVOKABLE unsigned int getAppAgentMode() {
        return value(APP_AGENT_MODE, QVariant(QNetworkProxy::NoProxy)).toUInt();
    }


    // APP_AGENT_SERVER
    Q_INVOKABLE void saveAppAgentServer(QString server) {
        setValue(APP_AGENT_SERVER, server);
    }
    Q_INVOKABLE QString getAppAgentServer() {
        return value(APP_AGENT_SERVER, QVariant("")).toString();
    }


    // APP_AGENT_PORT
    Q_INVOKABLE void saveAppAgentPort(unsigned int port) {
        setValue(APP_AGENT_PORT, port);
    }
    Q_INVOKABLE unsigned int getAppAgentPort() {
        return value(APP_AGENT_PORT, QVariant(80)).toUInt();
    }


    // APP_AGENT_USERNAME
    Q_INVOKABLE void saveAppAgentUsername(QString username) {
        setValue(APP_AGENT_USERNAME, username);
    }
    Q_INVOKABLE QString getAppAgentUsername() {
        return value(APP_AGENT_USERNAME, QVariant("")).toString();
    }


    // APP_AGENT_PASSWORD
    Q_INVOKABLE void saveAppAgentPassword(QString password) {
        setValue(APP_AGENT_PASSWORD, password);
    }
    Q_INVOKABLE QString getAppAgentPassword() {
        return value(APP_AGENT_PASSWORD, QVariant("")).toString();
    }


    // APP_TOUR
    Q_INVOKABLE void saveAppTour(QString s) {
        setValue(APP_TOUR, s);
    }
    Q_INVOKABLE QString getAppTour() {
        return value(APP_TOUR, QVariant("")).toString();
    }


    // APP_DOWNLOAD_MODE
    Q_INVOKABLE void saveAppDownloadMode(unsigned int mode) {
        setValue(APP_DOWNLOAD_MODE, mode);
    }
    Q_INVOKABLE unsigned int getAppDownloadMode(unsigned int def=0) {
        return value(APP_DOWNLOAD_MODE, def).toUInt();
    }


    // APP_DOWNLOAD_DIR
    Q_INVOKABLE void saveAppDownloadDir(const QString &dir) {
        setValue(APP_DOWNLOAD_DIR, dir);
    }
    Q_INVOKABLE QString getAppDownloadDir(
        const QVariant def=QVariant(
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
            )
        ) {
        return value(APP_DOWNLOAD_DIR, def).toString();
    }


    // APP_DOWNLOAD_OVERRIDE
    Q_INVOKABLE void saveAppDownloadOverride(bool override) {
        setValue(APP_DOWNLOAD_OVERRIDE, override);
    }
    Q_INVOKABLE bool getAppDownloadOverride(const bool def=false) {
        return value(APP_DOWNLOAD_OVERRIDE, def).toBool();
    }


    // APP_ACCENT_COLOR
    Q_INVOKABLE void saveAppAccentColor(const QString &str) {
        setValue(APP_ACCENT_COLOR, str);
    }
    Q_INVOKABLE QString getAppAccentColor(const QString &def=QString("#0078d4")) {
        return value(APP_ACCENT_COLOR, def).toString();
    }


    // APP_NATIVE_TEXT
    Q_INVOKABLE void saveAppNativeText(bool act) {
        setValue(APP_NATIVE_TEXT, act);
    }
    Q_INVOKABLE bool getAppNativeText(bool def=false) {
        return value(APP_NATIVE_TEXT, def).toBool();
    }


    // APP_ANIMATION_ENABLED
    Q_INVOKABLE void saveAppAnimationEnabled(bool enable) {
        setValue(APP_ANIMATION_ENABLED, enable);
    }
    Q_INVOKABLE bool getAppAnimationEnabled(bool def=true) {
        return value(APP_ANIMATION_ENABLED, def).toBool();
    }


    // APP_DEBOUND_DELAY
    Q_INVOKABLE void saveAppDeboundDelay(unsigned int delay) {
        setValue(APP_DEBOUND_DELAY, delay);
    }
    Q_INVOKABLE unsigned int getAppDeboundDelay(unsigned int def = 500) {
        return value(APP_DEBOUND_DELAY, def).toInt();
    }


    // WINDOW_BLUR
    Q_INVOKABLE void saveWindowBlur(bool blur) {
        setValue(WINDOW_BLUR, blur);
    }
    Q_INVOKABLE bool getWindowBlur(bool def=false) {
        return value(WINDOW_BLUR, def).toBool();
    }


    // WINDOW_OPACITY
    Q_INVOKABLE void saveWindowOpacity(qreal opacity) {
        setValue(WINDOW_OPACITY, opacity);
    }
    Q_INVOKABLE qreal getWindowOpacity(qreal def=0.75) {
        return value(WINDOW_OPACITY, def).toReal();
    }


    // WINDOW_BLUR_RADIUS
    Q_INVOKABLE void saveWindowBlurRadius(qreal radius) {
        setValue(WINDOW_BLUR_RADIUS, radius);
    }
    Q_INVOKABLE qreal getWindowBlurRadius(qreal def=60) {
        return value(WINDOW_BLUR_RADIUS, def).toReal();
    }


    // WINDOW_EFFECT
    Q_INVOKABLE void saveWindowEffect(const QString &effect) {
        setValue(WINDOW_EFFECT, effect);
    }
    Q_INVOKABLE QString getWindowEffect(QString def=QString("normal")) {
        return value(WINDOW_EFFECT, def).toString();
    }


    // DEBUG_ENABLE
    Q_INVOKABLE void saveDebugEnable(bool enable) {
        setValue(DEBUG_ENABLE, enable);
    }
    Q_INVOKABLE bool getDebugEnable() {
        return value(DEBUG_ENABLE, QVariant(false)).toBool();
    }


signals:

private:
    // var
    QScopedPointer<QSettings> mSettings;
    QString mIniFilePath;

    // func
    void setValue(const QString &key, const QVariant val);
    QVariant value(const QString &key, const QVariant def = {});
};

#endif // SETTINGSHELPER_H
