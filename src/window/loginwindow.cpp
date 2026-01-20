#include "loginwindow.h"
#include "ui_loginwindow.h"

#include <QDebug>
#include <QMessageBox>
#include <QTimer>

#include "../service/mainservice.h"

LoginWindow::LoginWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::LoginWindow)
{
    ui->setupUi(this);
    mSettings       = SettingsHelper::getInstance();
    mSettingsDialog = SettingsDialog::getInstance();
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::init()
{
    qDebug() << "LoginWindow::" <<  __func__;

    mMainService    = MainService::getInstance();
    // Fill the control data from the existing configuration information.
    this->ui->lineEdit_Username->setText(mSettings->getUserName());
    this->ui->lineEdit_Password->setText(mSettings->getUserPassword());
    this->ui->lineEdit_WebUrl->setText(mSettings->getWebUrl());
    this->ui->checkBox_RememberMe->setChecked(mSettings->getSavePassword());
    this->ui->checkBox_AutoLogin->setChecked( mSettings->getAutoLogin());

    // Initialize signals and slots
    connect(this->ui->pushButton_SignIn,   &QPushButton::clicked, this, &LoginWindow::onClickPushButtonSignIn);
    connect(this->ui->pushButton_Settings, &QPushButton::clicked, this, &LoginWindow::onClickPushButtonSettings);
    connect(this->ui->checkBox_RememberMe, &QCheckBox::stateChanged, this, &LoginWindow::onStateChangedRememberMe);
    connect(this->ui->checkBox_AutoLogin,  &QCheckBox::stateChanged, this, &LoginWindow::onStateChangedAutoLogin);
    {
        QSignalBlocker blocker(this->ui->checkBox_RememberMe);
        this->ui->checkBox_RememberMe->setCheckState(mSettings->getSavePassword() ? Qt::Checked : Qt::Unchecked);
    }
    {
        QSignalBlocker blocker(this->ui->checkBox_AutoLogin);
        this->ui->checkBox_AutoLogin->setCheckState(mSettings->getAutoLogin() ? Qt::Checked : Qt::Unchecked);
    }

    // Start Auto Login Timer
    QTimer::singleShot(500, this, [=] {
        qDebug() << "onTriggered Auto Login";
        if (this->getAutoLogin()) {
            this->onClickPushButtonSignIn();
        }
    });
}

bool LoginWindow::getRememberMe()
{
    return this->ui->checkBox_RememberMe->isChecked();
}

bool LoginWindow::getAutoLogin()
{
    return this->ui->checkBox_AutoLogin->isChecked();
}

void LoginWindow::onClickPushButtonSignIn()
{
    qDebug() << __func__;
    QVariantMap map;
    MSEvent *event = new MSEvent(this, MSEvent::EVENT_TYPE_LOGIN_REQ);

    map["username"] = this->ui->lineEdit_Username->text();
    map["password"] = this->ui->lineEdit_Password->text();
    map["weburl"]   = this->ui->lineEdit_WebUrl->text();

    QVariant var = map;
    event->setData(var);
    QCoreApplication::postEvent(mMainService, event);
    this->ui->pushButton_SignIn->setEnabled(false);
}

void LoginWindow::onClickPushButtonSettings()
{
    qDebug() << __func__;
    mSettingsDialog->show();
}

void LoginWindow::onStateChangedRememberMe(int state)
{
    mRememberMe = state == Qt::Checked;
    // 当“记住密码”取消勾选时需要自动取消勾选“自动登录”，防止自动登录时密码是空的
    if (mRememberMe == false) {
        if (mAutoLogin == true) {
            mAutoLogin = false;
            {
                QSignalBlocker blocker(this->ui->checkBox_AutoLogin);
                this->ui->checkBox_AutoLogin->setCheckState(Qt::Unchecked);
            }
        }
    }
}

void LoginWindow::onStateChangedAutoLogin(int state)
{
    mAutoLogin = state == Qt::Checked;
    // 当“自动登录”被勾选时需要自动勾选“记住密码”，防止自动登录时密码是空的
    if (mAutoLogin == true) {
        if (mRememberMe == false) {
            mRememberMe = true;
            {
                QSignalBlocker blocker(this->ui->checkBox_RememberMe);
                this->ui->checkBox_RememberMe->setCheckState(Qt::Checked);
            }
        }
    } else {
        mSettings->saveAutoLogin(mAutoLogin);
    }
}

bool LoginWindow::event(QEvent *e)
{
    if (e->type() == MSEvent::MSEVENT_BASE_TYPE) {
        MSEvent *event = static_cast<MSEvent *>(e);
        mSEventHandler(event);
    }
    return QMainWindow::event(e);
}

void LoginWindow::closeEvent(QCloseEvent *event)
{
    // If the minimize-to-tray function is enabled, the window will be hidden instead of being closed.
    MainService *mMainService        = MainService::getInstance();
    QSystemTrayIcon *mSystemTrayIcon = mMainService->getSystemTrayIcon();
    if (mSystemTrayIcon->isVisible()) {
        qDebug() << "LoginWindow::closeEvent" << "hide window";
        hide();
        event->ignore();
        // Display the prompt message
        mSystemTrayIcon->showMessage("Friendly Reminder",
                                     "SecAssistUp is hidden from the tray, click on the tray to activate the window again.",
                                     QIcon(":/image/favicon_nbg.png"),
                                     3000);
    }
}

bool LoginWindow::mSEventHandler(MSEvent *e)
{
    uint16_t event  = e->getMSEventType();
    QVariantMap map = e->getData().toMap();
    qDebug("LoginWindow::mSEventHandler event:0x%04x", event);
    switch (event) {
    case MSEvent::EVENT_TYPE_LOGIN_CFM: {

        uint16_t resultCode = map["resultCode"].toUInt();

        qDebug() << "resultCode:" << resultCode;
        if (resultCode == MSEvent::RESULT_CODE_SUCCESS) {
            qDebug() << "Login Succeed.";
            // QMessageBox::information(this, "Reminder", "Login Succeed.");
            mSettings->saveUserName(this->ui->lineEdit_Username->text());
            if (getRememberMe()) {
                mSettings->saveUserPassword(this->ui->lineEdit_Password->text());
            } else {
                mSettings->saveUserPassword("");
            }
            mSettings->saveSavePassword(getRememberMe());
            mSettings->saveAutoLogin(getAutoLogin());

        } else if (resultCode == MSEvent::RESULT_CODE_FAIL) {
            QString msg = map["msg"].toString();
            if (msg.length() > 0) {
                qDebug() << "msg:" << msg;
                QMessageBox::critical(this, "Reminder", msg);
            } else {
                QMessageBox::critical(this, "Reminder", "Login Failure.");
            }

        } else if (resultCode == MSEvent::RESULT_CODE_NETWORK_ONERROR) {
            QString msg = QString("%1;%2;%3")
                .arg(map["status"].toString(), map["errorString"].toString(), map["result"].toString());
            qDebug() << "msg:" << msg;
            QMessageBox::critical(this, "Reminder", msg);

        } else {
            qWarning() << "Unknow Result Code." << resultCode;
        }

        this->ui->pushButton_SignIn->setEnabled(true);
        break;
    }
    }
    return true;
}

