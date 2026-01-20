#include "appinfo.h"

#include <QGuiApplication>

#include "../Version.h"
#include "settingshelper.h"

AppInfo::AppInfo(QObject *parent)
    : QObject{parent}
{
    version(APPLICATION_VERSION);
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
