#include "appinfo.h"

#include <QGuiApplication>

#include "../Version.h"
#include "settingshelper.h"

AppInfo::AppInfo(QObject *parent)
    : QObject{parent}
{
    QString ver = QString(APPLICATION_VERSION);
#ifdef QT_DEBUG
    ver += " Debug";
#else
    ver += " Release";
#endif
    version(ver);
#if 1
    debugEnable(SettingsHelper::getInstance()->getDebugEnable());
#else
    debugEnable(true);
#endif
    buildDate(__DATE__);
    buildTime(__TIME__);
    buildQtVersion(QT_VERSION_STR);
}

void AppInfo::testCrash()
{
    auto *crash = reinterpret_cast<volatile int *>(0);
    *crash = 0;
}
