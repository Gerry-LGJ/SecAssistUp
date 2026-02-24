#include "settingshelper.h"

#include <QStandardPaths>
#include <QFile>

SettingsHelper::SettingsHelper(QObject *parent)
    : QObject{parent}
{}

SettingsHelper::~SettingsHelper() = default;

void SettingsHelper::setValue(const QString &key, const QVariant val)
{
    mSettings->setValue(key, val);
}

QVariant SettingsHelper::value(const QString &key, const QVariant def)
{
    return mSettings->value(key, def);
}

void SettingsHelper::init(char *argv[])
{
    QString applicationPath = QString::fromStdString(argv[0]);
    const QFileInfo fileInfo(applicationPath);
    const QString iniFileName = fileInfo.completeBaseName() + ".ini";
    const QString iniFilePath =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/" + iniFileName;
    qDebug() << __func__ << "Path:" << iniFilePath;
    mIniFilePath = iniFilePath;
    autoSetDefaultConfig(iniFilePath);
    mSettings.reset(new QSettings(iniFilePath, QSettings::IniFormat));
}

void SettingsHelper::autoSetDefaultConfig(QString filePath)
{
    // qDebug() << "SettingsHelper::" << __FUNCTION__;
    if (!QFile::exists(filePath)) {
        QSettings defConfig = QSettings(filePath, QSettings::IniFormat);
        /* set default value */
        defConfig.setValue(QString(USER_WEB_URL),               "http://192.168.10.88");
        defConfig.setValue(QString(USER_NAME),                  "");
        defConfig.setValue(QString(USER_PASSWORD),              "");
        defConfig.setValue(QString(USER_AUTO_LOGIN),            "false");
        defConfig.setValue(QString(USER_SAVE_PASSWORD),         "false");
        defConfig.setValue(QString(USER_PHPSESSID),             "");
        defConfig.setValue(QString(USER_HEARTBEAT_INTERNAL),    300);
        defConfig.sync();
    }
    /* set default value done */
    // qDebug() << "SettingsHelper::::" << __FUNCTION__;
}

QString SettingsHelper::getIniFilePath()
{
    return mIniFilePath;
}

void SettingsHelper::sync()
{
    mSettings->sync();
}
