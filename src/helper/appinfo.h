#ifndef APPINFO_H
#define APPINFO_H

#include <QObject>
#include "../common/singleton.h"
#include "../common/stdafx.h"

class AppInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY_AUTO(QString, version);
    Q_PROPERTY_AUTO(bool, debugEnable)
    Q_PROPERTY_AUTO(QString, buildDate);
    Q_PROPERTY_AUTO(QString, buildTime);
    Q_PROPERTY_AUTO(QString, buildQtVersion);

    SINGLETON(AppInfo)

    [[maybe_unused]] Q_INVOKABLE void testCrash();

private:
    explicit AppInfo(QObject *parent = nullptr);

signals:
};

#endif // APPINFO_H
