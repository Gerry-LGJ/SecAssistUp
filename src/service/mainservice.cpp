#include "mainservice.h"

#include <QDebug>
#include <QApplication>
#include <QMenu>
#include <QAction>

#include "../helper/settingshelper.h"

MainService::MainService(QObject *parent)
    : QObject{parent}
{
    mLoginWindow        = LoginWindow::getInstance();
    mMainWindow         = MainWindow::getInstance();
    mSettingsDialog     = SettingsDialog::getInstance();
    mProjectInfoDialog  = ProjectInfoDialog::getInstance();
    mLoginService       = LoginService::getInstance();
    mRefreshService     = RefreshService::getInstance();
    mDownloadService    = DownloadService::getInstance();
    mUploadService      = UploadService::getInstance();
    mFileWatcherService = FileWatcherService::getInstance();
}

bool MainService::event(QEvent *e)
{
    if (e->type() == MSEvent::MSEVENT_BASE_TYPE) {
        MSEvent *event = static_cast<MSEvent *>(e);
        mSEventHandler(event);
        return true;
    }
    return QObject::event(e);
}

void MainService::init()
{
    qDebug() << "MainService::init";
    // init window
    mLoginWindow->init();
    mMainWindow->init();
    mSettingsDialog->init();
    mProjectInfoDialog->init();
    mLoginService->init();
    mRefreshService->init();
    mDownloadService->init();
    mUploadService->init();
    mFileWatcherService->init();
    initSystemTrayIcon();
    currentActiveWindowType = ACTIVE_WINDOW_TYPE_LOGIN;
}

void MainService::requestShowActiveWindow(int type)
{
    qDebug() << __func__ << "type:" << type;
    switch (type) {
    case ACTIVE_WINDOW_TYPE_LOGIN:
        mLoginWindow->show();
        mLoginWindow->activateWindow();
        break;
    case ACTIVE_WINDOW_TYPE_MAIN:
        mMainWindow->show();
        mMainWindow->activateWindow();
        break;
    case ACTIVE_WINDOW_TYPE_SETTINGS:
        mSettingsDialog->show();
        mSettingsDialog->activateWindow();
        break;
    default:
        qWarning() << __func__ << "Unknow Window Type";
        break;
    }
}

void MainService::initSystemTrayIcon()
{
    // System Tray Icon

    mSystemTrayIcon = new QSystemTrayIcon(QIcon(":/image/favicon_nbg.png"), this);
    mSystemTrayIcon->setToolTip("SecAssistUp");
    mSystemTrayIcon->setVisible(true);
    connect(mSystemTrayIcon, &QSystemTrayIcon::activated, this, [=] (QSystemTrayIcon::ActivationReason reason) {
        qDebug() << "SystemTrayIcon::ActivationReason" << reason;
        uint16_t type = currentActiveWindowType;
        if (reason == QSystemTrayIcon::Trigger) {
            requestShowActiveWindow(type);
        } else if (reason == QSystemTrayIcon::Context) {

        }
    });

    // Create Menu
    QMenu *menu = new QMenu();

    // Create menu item
    QAction *settingsAction = new QAction("Settings");
    QAction *showAction     = new QAction("Display window");
    QAction *quitAction     = new QAction("Quit");

    // Add menu item
    menu->addAction(showAction);
    menu->addAction(settingsAction);
    menu->addSeparator();
    menu->addAction(quitAction);

    // Set tray menu
    mSystemTrayIcon->setContextMenu(menu);

    // Connect the menu signal
    connect(showAction, &QAction::triggered, this, [=] {
        uint16_t type = currentActiveWindowType;
        requestShowActiveWindow(type);
    });
    connect(settingsAction, &QAction::triggered, this, [=] {
        uint16_t type = currentActiveWindowType;
        requestShowActiveWindow(type);
        requestShowActiveWindow(ACTIVE_WINDOW_TYPE_SETTINGS);
    });
    connect(quitAction, &QAction::triggered, this, [=] {
        QCoreApplication::exit();
    });
}

bool MainService::mSEventHandler(MSEvent *e)
{
    uint16_t event = e->getMSEventType();
    qDebug("MainService::mSEventHandler event:0x%04x", event);
    switch (event) {
    case MSEvent::EVENT_TYPE_LOGIN_REQ: {
        if (AppInfo::getInstance()->debugEnable()) {
            qDebug() << __func__ << "debugEnable";
            mLoginWindow->hide();
            mMainWindow->show();
            currentActiveWindowType = ACTIVE_WINDOW_TYPE_MAIN;
            QVariantMap map;
            map["resultCode"] = MSEvent::RESULT_CODE_SUCCESS;
            MSEvent *ee = new MSEvent(this, MSEvent::EVENT_TYPE_LOGIN_CFM);
            ee->setData(map);
            QCoreApplication::postEvent(e->getSender(), ee);
        } else {
            MSEvent *loginEvent = new MSEvent(e->getSender(), e->getMSEventType());
            loginEvent->setData(e->getData());
            QCoreApplication::postEvent(mLoginService, loginEvent);
        }
        break;
    }
    case MSEvent::EVENT_TYPE_LOGOUT_REQ: {
        MSEvent *loginEvent = new MSEvent(e->getSender(), e->getMSEventType());
        loginEvent->setData(e->getData());
        QCoreApplication::postEvent(mLoginService, loginEvent);
        break;
    }
    case MSEvent::EVENT_TYPE_REFRESH_REQ: {
        if (AppInfo::getInstance()->debugEnable()) {
        } else {
            MSEvent *refreshEvent = new MSEvent(e->getSender(), e->getMSEventType());
            refreshEvent->setData(e->getData());
            QCoreApplication::postEvent(mRefreshService, refreshEvent);
        }
        break;
    }
    case MSEvent::EVENT_TYPE_ENTRY_FOLDER_REQ: {
        if (AppInfo::getInstance()->debugEnable()) {

        } else {
            MSEvent *entryEvent = new MSEvent(e->getSender(), e->getMSEventType());
            entryEvent->setData(e->getData());
            QCoreApplication::postEvent(mRefreshService, entryEvent);
        }
        break;
    }
    case MSEvent::EVENT_TYPE_RETURN_PARENT_DIR_REQ: {
        if (AppInfo::getInstance()->debugEnable()) {

        } else {
            MSEvent *returnEvent = new MSEvent(e->getSender(), e->getMSEventType());
            returnEvent->setData(e->getData());
            QCoreApplication::postEvent(mRefreshService, returnEvent);
        }
        break;
    }
    case MSEvent::EVENT_TYPE_DOWNLOAD_REQ: {
        if (AppInfo::getInstance()->debugEnable()) {

        } else {
            MSEvent *downloadEvent = new MSEvent(e->getSender(), e->getMSEventType());
            downloadEvent->setData(e->getData());
            QCoreApplication::postEvent(mDownloadService, downloadEvent);
        }
        break;
    }
    case MSEvent::EVENT_TYPE_UPLOAD_REQ: {
        if (AppInfo::getInstance()->debugEnable()) {

        } else {
            MSEvent *uploadEvent = new MSEvent(e->getSender(), e->getMSEventType());
            uploadEvent->setData(e->getData());
            QCoreApplication::postEvent(mUploadService, uploadEvent);
        }
        break;
    }
    case MSEvent::EVENT_TYPE_LOGIN_CFM: {
        SettingsHelper *helper = SettingsHelper::getInstance();
        QVariantMap map        = e->getData().toMap();
        uint16_t resultCode    = map["resultCode"].toUInt();
        qDebug() << "resultCode:" << resultCode;
        if (resultCode == MSEvent::RESULT_CODE_SUCCESS) {
            MSEvent *refreshEvent = new MSEvent(this, MSEvent::EVENT_TYPE_REFRESH_REQ);
            QCoreApplication::postEvent(mRefreshService, refreshEvent);
            mLoginWindow->hide();
            mMainWindow->show();
            currentActiveWindowType = ACTIVE_WINDOW_TYPE_MAIN;
            mMainWindow->setLabelLoginUser(helper->getUserName());
        }
        break;
    }
    case MSEvent::EVENT_TYPE_REFRESH_CFM: {
        break;
    }
    case MSEvent::EVENT_TYPE_LOGOUT_CFM: {
        QVariantMap map = e->getData().toMap();
        uint16_t resultCode = map["resultCode"].toUInt();
        qDebug() << "resultCode:" << resultCode;
        if (resultCode == MSEvent::RESULT_CODE_SUCCESS) {
            mMainWindow->hide();
            mLoginWindow->show();
            currentActiveWindowType = ACTIVE_WINDOW_TYPE_LOGIN;
        }
        break;
    }
    default:
        qWarning("%s Unknow Event Type 0x%04x", __func__, event);
    }
    return true;
}
