#pragma once

#include <QObject>
#include <QEvent>
#include <QSystemTrayIcon>

#include "msevent.h"
#include "../common/singleton.h"
#include "../window/loginwindow.h"
#include "../window/mainwindow.h"
#include "../widgets/settingsdialog.h"
#include "../widgets/projectinfodialog.h"
#include "loginservice.h"
#include "refreshservice.h"
#include "downloadservice.h"
#include "uploadservice.h"
#include "filewatcherservice.h"

class MSEvent;

class MainService : public QObject
{
    Q_OBJECT
private:
    explicit MainService(QObject *parent = nullptr);
    void mainServiceMsgHandler(void *data);

protected:
    bool event(QEvent *e) override;

public:
    SINGLETON(MainService);
    void init();
    LoginWindow     *getLoginWindowInst()    { return mLoginWindow;    }
    MainWindow      *getMainWindowInst()     { return mMainWindow;     }
    SettingsDialog  *getSettingsDialogInst() { return mSettingsDialog; }
    QSystemTrayIcon *getSystemTrayIcon()     { return mSystemTrayIcon; }

private:
    // Login Service
    LoginService        *mLoginService;
    // Refresh Service
    RefreshService      *mRefreshService;
    // Download Service
    DownloadService     *mDownloadService;
    // Upload Service
    UploadService       *mUploadService;
    // File Watcher Service
    FileWatcherService  *mFileWatcherService;
    // System Tray Icon
    QSystemTrayIcon     *mSystemTrayIcon;
    // Window
    enum {
        ACTIVE_WINDOW_TYPE_LOGIN,
        ACTIVE_WINDOW_TYPE_MAIN,
        ACTIVE_WINDOW_TYPE_SETTINGS,
        ACTIVE_WINDOW_TYPE_MODIFY_PROJECT
    } ;
    uint16_t             currentActiveWindowType;
    LoginWindow         *mLoginWindow;
    MainWindow          *mMainWindow;
    void requestShowActiveWindow(int type);
    // Settings UI Service
    SettingsDialog      *mSettingsDialog;
    // Project Info UI Service
    ProjectInfoDialog   *mProjectInfoDialog;
    // System Tray Icon
    void initSystemTrayIcon(void);
    // Handle Function
    bool mSEventHandler(MSEvent *e);
    // Refresh Service

    // Download Service

    // Upload Service
};

