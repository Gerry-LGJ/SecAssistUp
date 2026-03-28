#include "updateservice.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QApplication>
#include <QProcess>
#include <QSystemTrayIcon>

#include "mainservice.h"

static bool mDebug = true;

UpdateService::UpdateService(QObject *parent)
    : QObject{parent}
{
    mNetwork           = Network::getInstance();
    mGetLatestCallable = new NetworkCallable(this);
    mDownloadCallable  = new NetworkCallable(this);
    mSettings          = SettingsHelper::getInstance();
    mAppInfo           = AppInfo::getInstance();
    mFilTools          = FilTools::getInstance();
}


void UpdateService::init()
{
    mCurrentVersion    = mDebug ? Version(0, 4, 2) : Version(mAppInfo->version());
    mState             = idle_s;
    mUserAgent         = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) "
                         "Chrome/138.0.0.0 Safari/537.36 Edg/138.0.0.0";
    mPercent           = -1;


    cleanupInstaller();

    initGetLatestCallable();

    initDownloadNetworkCallable();

    qDebug() << "UpdateService::init mCurrentVersion:" << mCurrentVersion.toString();

    QTimer::singleShot(10 * 1000, this, [=] {
        checkForUpdate();
    });
}


QString UpdateService::getUpdateMsg()
{
    return mUpdateMsg;
}


int UpdateService::getUpdateProgress()
{
    return mPercent;
}

bool UpdateService::checkForAutoUpdate(bool isManual)
{
    SettingsHelper *mSettings  = SettingsHelper::getInstance();
    AppInfo *mAppInfo          = AppInfo::getInstance();
    Version currentVer         = mDebug ? Version(0, 4, 2) : Version(mAppInfo->version());
    QString latestVerStr       = mSettings->getAppLatestVersion(currentVer.toString());
    Version cacheLatestVer     = Version(latestVerStr);
    int udmode                 = mSettings->getAppUpdateMode();

    qDebug() << __func__ <<
        "cur:" << currentVer.toString() <<
        "latest:" << cacheLatestVer.toString();

    if (udmode == 0 || isManual) {
        if (currentVer < cacheLatestVer) {
            QString installer = getInstallerLocation() + "/" + getInstallerFileByVersion(cacheLatestVer);
            QFile     file(installer);
            QFileInfo info(installer);

            if (!info.exists()) {
                qDebug() << __func__ << "installer not exist.";
                return false;
            }

#if 1
            // if (mAppInfo->debugEnable()) {
                QMessageBox::StandardButton reply = QMessageBox::question(
                    QApplication::activeWindow(),
                    tr("Developer"),
                    tr("Have you found the new version installation package? Should we proceed with the update?"));
                if (reply == QMessageBox::No) {
                    return false;
                }
            // }
#endif

            if (info.exists()) {
                QStringList arguments;
                arguments << installer << "/SILENT" << "/AUTOUNINSTALL" << "/LAUNCHAPP";
                QProcess::startDetached(installer, arguments);
                return true;
            }

        } else {
            qDebug() << __func__ << "The cached version is not the latest one.";
        }
    }

    return false;
}

void UpdateService::initGetLatestCallable()
{
    connect(mGetLatestCallable, &NetworkCallable::start, this, [=] {
        mChecking = true;
        qDebug() << "start check update...";
        transition(checking_s);

    });
    connect(mGetLatestCallable, &NetworkCallable::finish, this, [=] {
        mChecking = false;
        qDebug() << "check update finish";
        transition(idle_s);

    });
    connect(mGetLatestCallable, &NetworkCallable::error, this, [=] (int status, QString errorString, QString result) {
        qDebug() << "initGetLatestCallable error" <<
            "status:" << status <<
            "errorString:" << errorString <<
            "result:" << result;
        QString msg = QString("%1;%2;%3").arg(status).arg(errorString, result);
        notifyUpdateMsgChanged(msg);

    });
    connect(mGetLatestCallable, &NetworkCallable::success, this, [=] (QString result) {
        onGetLatestInfoSuccess(result);

    });
}


void UpdateService::initDownloadNetworkCallable()
{
    connect(mDownloadCallable, &NetworkCallable::start, this, [=] {
        mDownloading = true;
        transition(downloading_s);
        notifyUpdateProgressChanged(0);
        notifyUpdateMsgChanged(tr("Downloading ......"));

    });
    connect(mDownloadCallable, &NetworkCallable::finish, this, [=] {
        mDownloading = false;
        transition(idle_s);
        notifyUpdateProgressChanged(-1);

    });
    connect(mDownloadCallable, &NetworkCallable::error, this, [=] (int status, QString errorString, QString result) {
        qDebug() << "initDownloadNetworkCallable error" <<
            "status:" << status <<
            "errorString:" << errorString <<
            "result:" << result;
        QString msg = QString("%1;%2;%3").arg(status).arg(errorString, result);
        notifyUpdateMsgChanged(msg);

    });
    connect(mDownloadCallable, &NetworkCallable::success, this, [=] (QString result) {
        onDownloadSuccess(result);

    });
    connect(mDownloadCallable, &NetworkCallable::downloadProgress, this, [=] (qint64 recv, qint64 total) {
        int progress = (double)((double)recv / (double)total) * 100;
        notifyUpdateProgressChanged(progress);

    });
}


void UpdateService::transition(uint16_t state)
{
    qDebug() << "UpdateService::transition" << "state :" << state;
    mState = state;
}

void UpdateService::notifyUpdateMsgChanged(QString msg)
{
    mUpdateMsg = msg;
    emit updateMsgChanged(msg);
}

void UpdateService::notifyUpdateProgressChanged(int progress)
{
    mPercent = progress;
    emit updateProgressChanged(progress);
}


bool UpdateService::checkForUpdate(bool silent)
{
    if (mState != idle_s) {
        qDebug() << __func__ << "mState:" << mState << "not allow";
        return false;
    }
    mSilent = silent;

    QString url = QString("https://api.github.com/repos/Gerry-LGJ/SecAssistUp/releases/latest");
    mNetwork->get(url)
    ->addHeader("User-Agent", mUserAgent)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ->addAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy)
#endif
    ->bind(this)
    ->go(mGetLatestCallable);

    return true;
}


bool UpdateService::download()
{
    if (mState != idle_s) {
        qDebug() << __func__ << "mState:" << mState << "not allow";
        return false;
    }

    if (mDownloadUrl.length() <= 0) {
        qDebug() << __func__ << "invalid mDownloadUrl";
        return false;
    }
    if (mDownloadFile.length() <= 0) {
        qDebug() << __func__ << "invalid mDownloadFile";
        return false;
    }
    QString file = getInstallerLocation() + "/" + mDownloadFile;

    // Try to check if the installer already exists.
    // We can verify it by comparing its SHA256 hash value.

    QString downFileSha256 = mFilTools->sha256CalculateFile(file);

    if (mInstallerSha256.length() > 0 && downFileSha256.length() > 0) {
        if (mInstallerSha256 == downFileSha256) {
            qDebug() << __func__ << "The latest version of the installer is already available locally. "
                                    "Skip the download process.";
            QTimer::singleShot(0, this, [=] {
                QString file = getInstallerLocation() + "/" + mDownloadFile;
                onDownloadSuccess(file);
            });
            return true;
        }
    }

    qDebug() << __func__ << "mDownloadUrl:" << mDownloadUrl << "file:" << file;

    mNetwork->get(mDownloadUrl)
    ->addHeader("User-Agent", mUserAgent)
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ->addAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy)
#endif
    ->toDownload(file)
    ->bind(this)
    ->go(mDownloadCallable);


    return true;
}


void UpdateService::onGetLatestInfoSuccess(QString &rsp)
{
    qDebug() << __func__;

#if 1
    QJsonDocument doc = QJsonDocument::fromJson(rsp.toUtf8());
    qDebug().noquote() << "\n" << doc.toJson(QJsonDocument::Indented);
#endif

    QString tagname        = getLatestValueFromRsp(rsp, tag_name_n);
    QString body           ;
    QString downloadurl    ;
    Version latestVersion  = Version(tagname);

    if (latestVersion.isValid()) {
        qDebug() << __func__ << "The update check will be aborted due to an invalid "
                                "version number retrieved from the remote server.";
        return ;
    }

    qDebug() << __func__ << "CurrentVersion:" << mCurrentVersion.toString() <<
        "LatestVersion:" << latestVersion.toString();

    if (mCurrentVersion < latestVersion || (mDebug && mCurrentVersion == latestVersion)) {

        mLatestVersion   = latestVersion;
        mDownloadFile    = getInstallerFileByVersion(mLatestVersion);
        mDownloadUrl     = getLatestValueFromRsp(rsp, browser_download_url_n);
        mInstallerSha256 = getLatestValueFromRsp(rsp, digest_n);
        body             = getLatestValueFromRsp(rsp, body_n    );
        QString msg      = QString(tr("New version %1 found.")).arg(latestVersion.toString());

        mSettings->saveAppLatestVersion(latestVersion.toString());

        notifyUpdateMsgChanged(msg);

        int mode = mSettings->getAppUpdateMode();

        if (mode == 0) {
            // 设置自动更新的情况下，直接开始下载更新即可

            // 由于是在槽函数中更新，此时还是updating状态，无法直接调用download()，待当前槽函数调度完成后自动调用download()
            QTimer::singleShot(0, this, [=] { download(); });

        } else if (mode == 1) {

            if  (mSilent == true) {
                // 通知模式下，如果是通过静默方式请求的检查更新，则不弹框询问用户
                MainService     *mMainService    = MainService::getInstance();
                QSystemTrayIcon *mSystemTrayIcon = mMainService->getSystemTrayIcon();

                mSystemTrayIcon->showMessage(msg,
                                             tr("Please obtain the updated information on the relevant page."),
                                             QIcon(":/image/favicon_nbg.png"));


            } else {
                // 通知模式下，如果非静默方式下，直接弹框询问用户是否需要更新应用程序
                QString questionMsg = QString(
                    msg + '\n' +
                    '\n' +
                    body);

                QMessageBox::StandardButton reply = QMessageBox::question(
                    QApplication::activeWindow(),
                    tr("Update"),
                    questionMsg);

                if (reply == QMessageBox::Yes) {
                    qDebug() << __func__ << "Start downloading update file ......";

                    // 由于是在槽函数中更新，此时还是updating状态，无法直接调用download()，待当前槽函数调度完成后自动调用download()
                    QTimer::singleShot(0, this, [=] { download(); });
                } else {
                    qDebug() << __func__ << "User cancel continue update.";
                }
            }

            // reset silent state
            mSilent = true;

        }
    } else if (latestVersion < mCurrentVersion) {
        QString msg      = QString(tr("The latest version is lower than the current version."));
        notifyUpdateMsgChanged(msg);
    } else {
        qDebug() << __func__ << "";
        QString msg      = QString(tr("The current version is already up to date."));
        notifyUpdateMsgChanged(msg);
        mSettings->saveAppLatestVersion(latestVersion.toString());
    }
}


void UpdateService::onDownloadSuccess(QString result)
{
    qDebug() << __func__ << result;

    int udmode            = mSettings->getAppUpdateMode();
    MainService *mService = MainService::getInstance();
    QString file          = getInstallerLocation() + "/" + mDownloadFile;

    // Check the integrity of the files
    bool integrity = false;
    QString downFileSha256 = mFilTools->sha256CalculateFile(file);
    if (mInstallerSha256.length() > 0 && downFileSha256.length() > 0) {
        if (mInstallerSha256 == downFileSha256) {
            integrity = true;

        }
    }
    if (!integrity) {
        qDebug() << __func__ << "mInstallerSha256:" << mInstallerSha256 << "downFileSha256:" << downFileSha256;
        qDebug() << __func__ << "The comparison of the file's Sha256 value failed. "
                                "This is the update process of the terminal.";
        cleanupInstaller(mLatestVersion);
        return ;

    } else {
        qDebug() << __func__ << "The file has passed the legality verification and "
                                "the new version installer has been downloaded successfully.";
        QString msg = QString(tr("%1 The installer has been downloaded successfully.")).arg(mLatestVersion.toString());
        notifyUpdateMsgChanged(msg);

    }

    if (udmode == 1) {

        QString questionMsg = QString(tr("Version %1 has been downloaded. \n"
                                         "Do you want to proceed with the update?"))
                                  .arg(mLatestVersion.toString());

        QMessageBox::StandardButton reply = QMessageBox::question(
            QApplication::activeWindow(),
            tr("Update"),
            questionMsg);

        if (reply == QMessageBox::Yes) {
            qDebug() << __func__ << "The user agrees to proceed with the update process.";
            mService->exit(932);
        }

    }
}

void UpdateService::onMessageClicked()
{
    qDebug() << __func__;
    download();
}


QString UpdateService::getLatestValueFromRsp(QString &rsp, latest_info_t type)
{
    QString         result;
    QJsonParseError err;
    QJsonDocument   doc = QJsonDocument::fromJson(rsp.toUtf8(), &err);

    if (err.error != QJsonParseError::NoError) {
        qDebug() << __func__ << "Json Parse Error:" << err.errorString();
        return QString();
    }

    switch (type) {
    case tag_name_n: {

        result = doc.object().value("tag_name").toString();
        qDebug() << __func__ << "Get tag_name:" << result;

        break;
    }
    case body_n: {

        result = doc.object().value("body").toString();

        break;
    }
    case browser_download_url_n: {

        QJsonArray assets = doc.object().value("assets").toArray();
        if (assets.isEmpty()) {
            qDebug() << __func__ << "Asset object not found.";
            break;
        }

        int     found    = -1;
        QString wantfind = QString("SecAssistUp-%1_setup.exe").arg(mLatestVersion.toString());

        qDebug() << __func__ << "wantfind:" << wantfind;
        for (int i  = 0; i < assets.size(); ++i) {
            QJsonValue value = assets.at(i);
            QString name     = value.toObject().value("name").toString();
            if (name == wantfind) {
                found = i;
                break;
            }
        }
        if (found == -1) {
            qDebug() << __func__ << "The latest version information does not include the file download link.";
            break;
        }
        QJsonObject foundObj = assets.at(found).toObject();

        result = foundObj.value("browser_download_url").toString();

        break;
    }
    case digest_n: {

        QJsonArray assets = doc.object().value("assets").toArray();
        if (assets.isEmpty()) {
            qDebug() << __func__ << "Asset object not found.";
            break;
        }

        int found = -1;
        QString wantfind = QString("SecAssistUp-%1_setup.exe").arg(mLatestVersion.toString());
        qDebug() << __func__ << "wantfind:" << wantfind;
        for (int i  = 0; i < assets.size(); ++i) {
            QJsonValue value = assets.at(i);
            QString name     = value.toObject().value("name").toString();
            if (name == wantfind) {
                found = i;
                break;
            }
        }
        if (found == -1) {
            qDebug() << __func__ << "The latest version information does not include the file download link.";
            break;
        }

        QJsonObject foundObj = assets.at(found).toObject();

        QString digest = foundObj.value("digest").toString();
        if (digest.length() <= 0) {
            qDebug() << __func__ << "Not found digest";
            break;
        }

        result = digest.section(':', 1);

        break;
    }
    default:
        break;
    }

    return result;
}


QString UpdateService::getInstallerFileByVersion(Version &version)
{
    return QString("SecAssistUp-%1_setup.exe").arg(version.toString());
}


QString UpdateService::getInstallerLocation()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

bool UpdateService::checkInstallerExists(Version& version)
{
    QString installer = getInstallerLocation() + "/" + getInstallerFileByVersion(version);
    QFileInfo info(installer);
    return info.exists();
}


void UpdateService::cleanupInstaller()
{
    cleanupInstaller(mCurrentVersion);
}

void UpdateService::cleanupInstaller(Version &version)
{
    QString installer = getInstallerLocation() + "/" + getInstallerFileByVersion(version);

    QFileInfo info(installer);
    QFile     file(installer);
    qDebug() << __func__ << "file:" << installer;

    if (!info.exists()) {
        qDebug() << __func__ << "The file that needs to be deleted does not exist.";
        return ;
    }

    if (!info.isWritable()) {
        qDebug() << __func__ << "You do not have the permission to delete this file.";
        return ;
    }

    if (file.remove()) {
        qDebug() << __func__ << "The file has been permanently deleted.";
        return ;
    } else {
        qWarning() << "File deletion failed :" << installer;
        qWarning() << "Error message        :" << file.errorString();
    }
}













