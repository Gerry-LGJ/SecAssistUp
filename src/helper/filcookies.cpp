#include "filcookies.h"

#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>

#include "settingshelper.h"

FilCookies::FilCookies(QObject *parent)
    : QNetworkCookieJar{parent}
{
    _mutex = new QMutex();
    debugMode = false; //true;
    load();
}

FilCookies::~FilCookies()
{
    qDebug() << __func__ << "destroy";
}

QList<QNetworkCookie> FilCookies::cookiesForUrl(const QUrl &url) const
{
    // QMutexLocker locker(_mutex);
    if (debugMode)
        qDebug() << "[" << __func__ << "]:" << "url:" << url;
    QList<QNetworkCookie> result = QNetworkCookieJar::cookiesForUrl(url);
    if (result.size() > 0) {
        for (const QNetworkCookie &cookie: result) {
            if (debugMode)
            qDebug() << "[" << __func__ << "]:" << cookie.name() << "=" << cookie.value();
        }
    }
    if (debugMode)
        qDebug() << "[" << __func__ << "]:" << "size:" << result.size();
    return result;
}
bool FilCookies::setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url)
{
    // QMutexLocker locker(_mutex);
    if (debugMode)
        qDebug() << "[" << __func__ << "]:" << "size:" << cookieList.size() << "url:" << url;
    bool result = QNetworkCookieJar::setCookiesFromUrl(cookieList, url);
    if (cookieList.size() > 0) {
        for (const QNetworkCookie &cookie: cookieList) {
            if (debugMode)
                qDebug() << "[" << __func__ << "]:" << cookie.name() << "=" << cookie.value();
        }
    }
    return result;
}
bool FilCookies::insertCookie(const QNetworkCookie &cookie)
{
    // QMutexLocker locker(_mutex);
    bool result = QNetworkCookieJar::insertCookie(cookie);
    if (result) {
        if (debugMode) {
            qDebug() << "[" << __func__ << "]:" << cookie.name() << "=" << cookie.value();
        }
    }
    return result;
}
bool FilCookies::updateCookie(const QNetworkCookie &cookie)
{
    // QMutexLocker locker(_mutex);
    bool result = QNetworkCookieJar::updateCookie(cookie);
    if (result) {
        if (debugMode) {
            qDebug() << "[" << __func__ << "]:" << cookie.name() << "=" << cookie.value();
        }
    }
    return result;
}
bool FilCookies::deleteCookie(const QNetworkCookie &cookie)
{
    // QMutexLocker locker(_mutex);
    bool result = QNetworkCookieJar::deleteCookie(cookie);
    if (result) {
        if (debugMode) {
            qDebug() << "[" << __func__ << "]:" << cookie.name() << "=" << cookie.value();
        }
    }
    return result;
}
void FilCookies::save() {
}

void FilCookies::load() {
}
