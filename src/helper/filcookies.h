#ifndef FILCOOKIES_H
#define FILCOOKIES_H

#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QMutex>

#include "../common/singleton.h"

class FilCookies : public QNetworkCookieJar
{
    Q_OBJECT
public:
    SINGLETON(FilCookies);
    explicit FilCookies(QObject *parent = nullptr);
    ~FilCookies();

    QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const override;
    bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url) override;
    bool insertCookie(const QNetworkCookie &cookie) override;
    bool updateCookie(const QNetworkCookie &cookie) override;
    bool deleteCookie(const QNetworkCookie &cookie) override;

private:
    QMutex *_mutex;
    bool debugMode;
    void load();
    void save();
};

#endif // FILCOOKIES_H
