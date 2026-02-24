#include "mainservice.h"

#include <QDebug>
#include <QApplication>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSharedMemory>

#include "../helper/settingshelper.h"
#include "../helper/filicontools.h"
#include "../version.h"

MainService::MainService(QObject *parent)
    : QObject{parent}
{
    mLoginWindow        = LoginWindow::getInstance();
    mMainWindow         = MainWindow::getInstance();
    mProjectInfoDialog  = ProjectInfoDialog::getInstance();
    mLoginService       = LoginService::getInstance();
    mRefreshService     = RefreshService::getInstance();
    mDownloadService    = DownloadService::getInstance();
    mUploadService      = UploadService::getInstance();
    mFileWatcherService = FileWatcherService::getInstance();
                          FilIconTools::getInstance();
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
    mProjectInfoDialog->init();
    mLoginService->init();
    mRefreshService->init();
    mDownloadService->init();
    mUploadService->init();
    mFileWatcherService->init();
    initSystemTrayIcon();
    SettingsDialog::initRunAtSystemStartup();
    currentActiveWindowType = ACTIVE_WINDOW_TYPE_LOGIN;

    SettingsHelper *settings = SettingsHelper::getInstance();
    if (settings->getAppStartMinimizedToTray()) {
        showSystemTrayMessage(tr("Friendly Reminder"),
                              tr("SecAssistUp is hidden from the tray, click on the tray to activate the window again."),
                              QIcon(":/image/favicon_nbg.png"));
    } else {
        requestShowActiveWindow(currentActiveWindowType);
    }
}

void MainService::closeEvent(QWidget *widget, QCloseEvent *event)
{
    // If the minimize-to-tray function is enabled, the window will be hidden instead of being closed.

    enum {
        ACTION_TRAY,
        ACTION_EXIT,
        ACTION_CANCEL
    }; // define action
    int action = ACTION_CANCEL;

    // get Settings Helper Instance
    SettingsHelper *mSettings = SettingsHelper::getInstance();

    QString closeMode         = mSettings->getAppCloseMainWindow();

    QCheckBox *checkBox_Remb  = new QCheckBox(tr("Remember my choice."));

    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Quit"));
    msgBox.setText(tr("Are you sure you want to exit the program ?"));
    msgBox.setIcon(QMessageBox::Question);
    // The message box takes ownership of the checkbox.
    msgBox.setCheckBox(checkBox_Remb);

    // add button
    QPushButton *cancelBtn   = msgBox.addButton(tr("Cancel"),   QMessageBox::ActionRole);
    QPushButton *minimizeBtn = msgBox.addButton(tr("Minimize"), QMessageBox::ActionRole);
    QPushButton *quitBtn     = msgBox.addButton(tr("Quit"),     QMessageBox::ActionRole);
    msgBox.setDefaultButton(quitBtn);

    if (closeMode == "tray") {
        action = ACTION_TRAY;
    } else if (closeMode == "exit") {
        action = ACTION_EXIT;
    } else {
        // run exec()
        msgBox.exec();
        bool checked = checkBox_Remb->checkState() == Qt::Checked;
        qDebug() << __func__ << "checked:" << checked;
        if (msgBox.clickedButton() == minimizeBtn) {
            action = ACTION_TRAY;
            if (checked) {
                mSettings->saveAppCloseMainWindow("tray");
            }
        } else if (msgBox.clickedButton() == quitBtn) {
            action = ACTION_EXIT;
            if (checked) {
                mSettings->saveAppCloseMainWindow("exit");
            }
        } else if (msgBox.clickedButton() == cancelBtn) {
            action = ACTION_CANCEL;
        }
    }

    // progress action
    qDebug() << __func__ << "action:" << action;
    switch (action) {
        case ACTION_TRAY: {
            if (mSystemTrayIcon->isVisible()) {
                qDebug() << "LoginWindow::closeEvent" << "hide window";
                widget->hide();
                event->ignore();
                // Display the prompt message
                showSystemTrayMessage(tr("Friendly Reminder"),
                                      tr("SecAssistUp is hidden from the tray, click on the tray to activate the window again."),
                                      QIcon(":/image/favicon_nbg.png"),
                                      3000);
            }
            break;
        }
        case ACTION_EXIT: {
            mSettings->sync();
            this->exit();
            event->accept();
            break;
        }
        case ACTION_CANCEL: {
            event->ignore();
            break;
        }
        default: {

            break;
        }
    }
}

void MainService::exit(int code)
{
    qDebug() << __func__ << "code:" << code;
    mSystemTrayIcon->hide();
    QCoreApplication::exit(code);
}

int MainService::runOnceOnly()
{
    QSharedMemory *shareMemory = new QSharedMemory(APPLICATION_GUID);
    if (shareMemory->attach()) {
        QMessageBox::information(nullptr, QApplication::applicationName() + " - "  + tr("Friendly Reminder"),
                                 tr("Please do not launch again. The application is already running and can be restored from the system tray."));
        return -1;
    }
    if (!shareMemory->create(1)) {
        QMessageBox::critical(nullptr, tr("Critical"), shareMemory->errorString());
        return -2;
    }
    return 0;
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
#ifdef QT_DEBUG
    QAction *testAction     = new QAction("Test 1");
#endif

    // Add menu item
#ifdef QT_DEBUG
    menu->addAction(testAction);
    menu->addSeparator();
#endif
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
        if (type == ACTIVE_WINDOW_TYPE_LOGIN) {
            SettingsDialog settings(mLoginWindow);
            settings.exec();
        } else if (type == ACTIVE_WINDOW_TYPE_MAIN) {
            SettingsDialog settings(mMainWindow);
            settings.exec();
        }
    });
    connect(quitAction, &QAction::triggered, this, [=] {
        mSystemTrayIcon->hide();
        QCoreApplication::exit();
    });
#ifdef QT_DEBUG
    connect(testAction, &QAction::triggered, this, [=] {

    });
#endif
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
