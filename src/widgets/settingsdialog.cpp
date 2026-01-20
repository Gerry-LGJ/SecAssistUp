#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QDebug>
#include <QStandardPaths>
#include <QFileDialog>
#include <QNetworkProxy>
#include <QMessageBox>


SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    mSettingsHelper    = SettingsHelper::getInstance();
    mFilTools          = FilTools::getInstance();
    mAppInfo           = AppInfo::getInstance();
    mNetwork           = Network::getInstance();
    mAgentTestCallable = new NetworkCallable(this);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::init()
{
    // Set as a modal window
    setModal(true);
    // set onClick handle function
    connect(this->ui->checkBox_DbgEnable,       &QCheckBox::stateChanged, this, &SettingsDialog::onDbgEnableStateChanged);
    connect(this->ui->pushButton_TestCrash,     &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonTestCrash);
    connect(this->ui->pushButton_Restart,       &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonRestart);
    connect(this->ui->pushButton_OpenLogFolder, &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonOpenLoggerFolder);
    connect(this->ui->pushButton_OpenCfg,       &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonOpenConfiguration);
    connect(this->ui->spinBox_RefreshInterval,  QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::onValueChangedSpinBoxRefreshInterval);
    connect(this->ui->spinBox_DebounceDelay,    QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::onValueChangedSpinBox_DebounceDelay);
    connect(this->ui->pushButton_SelectDownloadLocation, &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonSelectDownloadLocation);
    connect(this->ui->comboBox_AgentMode,       QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onCurrentIndexChangedComboBoxAgentMode);
    connect(this->ui->pushButton_AgentTest,     &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonAgentTest);
    connect(this->ui->pushButton_AgentConfirm,  &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonAgentConfirm);
    // reload settings to UI
    reloadConfigurationsFromSettings();
    // init agent test
    connect(this->mAgentTestCallable, &NetworkCallable::start, this, [=] {
        this->mAgentTesting = true;
    });
    connect(this->mAgentTestCallable, &NetworkCallable::finish, this, [=] {
        this->mAgentTesting = false;
    });
    connect(this->mAgentTestCallable, &NetworkCallable::error, this, [=] (int status, QString errorString, QString result) {
        QString msg = QString("%1;%2;%3").arg(status).arg(errorString, result);
        QMessageBox::critical(this, tr("Agent Test"), msg);
    });
    connect(this->mAgentTestCallable, &NetworkCallable::success, this, [=] (QString result) {
        QMessageBox::information(this, tr("Agent Test"), tr("Test Succeed."));
    });

    // other
    this->ui->lineEdit_Port->setValidator(new QIntValidator(0, 65535, this->ui->lineEdit_Port));
    mAgentTesting = false;
    // About Page
    this->ui->label_Version->setText(mAppInfo->version());
    this->ui->label_QtVersion->setText(mAppInfo->buildQtVersion());
    this->ui->label_BuildTime->setText(QString("%1 %2").arg(mAppInfo->buildDate(), mAppInfo->buildTime()));
}

// void SettingsDialog::closeEvent(QCloseEvent *event)
// {
//     // If the minimize-to-tray function is enabled, the window will be hidden instead of being closed.
//     MainService *mMainService        = MainService::getInstance();
//     QSystemTrayIcon *mSystemTrayIcon = mMainService->getSystemTrayIcon();
//     if (mSystemTrayIcon->isVisible()) {
//         qDebug() << "SettingsDialog::closeEvent" << "hide window";
//         hide();
//         event->ignore();
//         // Display the prompt message
//         mSystemTrayIcon->showMessage("Friendly Reminder",
//                                      "SecAssistUp is hidden from the tray, click on the tray to activate the window again.",
//                                      QIcon(":/image/favicon_nbg.png"),
//                                      3000);
//     }
// }

void SettingsDialog::onDbgEnableStateChanged(int state)
{
    qDebug() << __func__ << "state: " << state;
    if (state == Qt::Unchecked) {
        mSettingsHelper->saveDebugEnable(false);
        mAppInfo->debugEnable(false);
    } else if (state == Qt::PartiallyChecked) {
    } else if (state == Qt::Checked) {
        mSettingsHelper->saveDebugEnable(true);
        mAppInfo->debugEnable(true);
    }
}

void SettingsDialog::onClickPushButtonTestCrash()
{
    mAppInfo->testCrash();
}

void SettingsDialog::onClickPushButtonRestart()
{
    qDebug() << __func__;
    QApplication::exit(931);
}

void SettingsDialog::onClickPushButtonOpenLoggerFolder()
{
    qDebug() << __func__;
    QString wl = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/log";
    QString wll = wl.replace("\\", "/");
    mFilTools->showDirInExplorer(wll);
}

void SettingsDialog::onClickPushButtonOpenConfiguration()
{
    qDebug() << __func__;
    mFilTools->showFileTextInNotepad(mSettingsHelper->getIniFilePath());
}

void SettingsDialog::onValueChangedSpinBoxRefreshInterval(int i)
{
    qDebug() << __func__ << " i:" << i;
    mSettingsHelper->saveHeartbeatInterval(i);
}

void SettingsDialog::onValueChangedSpinBox_DebounceDelay(int i)
{
    qDebug() << __func__ << " i:" << i;
    mSettingsHelper->saveAppDeboundDelay(i);
}

void SettingsDialog::onClickPushButtonSelectDownloadLocation()
{
    QString directory = QFileDialog::getExistingDirectory(
        this,
        tr("Open Folder"),
        mSettingsHelper->getAppDownloadDir(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );
    if (!directory.isEmpty()) {
        qDebug() << __func__ << " " << directory;
        mSettingsHelper->saveAppDownloadDir(directory);
    }
}

void SettingsDialog::onCurrentIndexChangedComboBoxAgentMode(int index)
{
    qDebug() << __func__ << " index:" << index;
    switch (index) {
    case 0:
        this->ui->lineEdit_Server->setEnabled(false);
        this->ui->lineEdit_Port->setEnabled(false);
        this->ui->lineEdit_UserName->setEnabled(false);
        this->ui->lineEdit_Password->setEnabled(false);
        break;
    case 1:
    case 2:
        this->ui->lineEdit_Server->setEnabled(true);
        this->ui->lineEdit_Port->setEnabled(true);
        this->ui->lineEdit_UserName->setEnabled(true);
        this->ui->lineEdit_Password->setEnabled(true);
        break;
    default:
        break;
    }
}

void SettingsDialog::onClickPushButtonAgentTest()
{
    qDebug() << __func__;
    if (mAgentTesting) {
        qDebug() << "Agent Testing ......";
        return ;
    }
    QString url = mSettingsHelper->getWebUrl();
    int agentMode = 0;
    QString agentServer   = this->ui->lineEdit_Server->text();
    uint16_t agentPort    = this->ui->lineEdit_Port->text().toUInt();
    QString agentUserName = this->ui->lineEdit_UserName->text();
    QString agentPassword = this->ui->lineEdit_Password->text();
    switch (this->ui->comboBox_AgentMode->currentIndex()) {
    case 1: agentMode = QNetworkProxy::ProxyType::HttpProxy;   break;
    case 2: agentMode = QNetworkProxy::ProxyType::Socks5Proxy; break;
    }
    qDebug() << "TestProxyUrl:" << url;
    QString msglog = QString("type:%1 server:%2 port:%3 username:%4 password:%5")
                         .arg(agentMode)
                         .arg(agentServer)
                         .arg(agentPort)
                         .arg(agentUserName, agentPassword);
    qDebug() << __func__ << " " << msglog;
    mNetwork->get(url)
    ->setProxy(agentMode, agentServer, agentPort, agentUserName, agentPassword)
    ->addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36 Edg/138.0.0.0")
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    ->addAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy)
#endif
    ->bind(this)
    ->go(mAgentTestCallable);
}

void SettingsDialog::onClickPushButtonAgentConfirm()
{
    qDebug() << __func__;
    uint8_t proxyType = 0;
    switch (this->ui->comboBox_AgentMode->currentIndex()) {
    case 1: proxyType = QNetworkProxy::ProxyType::HttpProxy;   break;
    case 2: proxyType = QNetworkProxy::ProxyType::Socks5Proxy; break;
    default:
            proxyType = 0;                                     break;
    }
    mSettingsHelper->saveAppAgentMode(proxyType);
    mSettingsHelper->saveAppAgentServer(this->ui->lineEdit_Server->text());
    mSettingsHelper->saveAppAgentPort(this->ui->lineEdit_Port->text().toUInt());
    mSettingsHelper->saveAppAgentUsername(this->ui->lineEdit_UserName->text());
    mSettingsHelper->saveAppAgentPassword(this->ui->lineEdit_Password->text());
    reloadAgentConfigurations();
}

void SettingsDialog::reloadConfigurationsFromSettings()
{
    qDebug() << __func__;
    // Agent
    // Common Page
    this->ui->spinBox_RefreshInterval->setValue(mSettingsHelper->getHeartbeatInterval());
    this->ui->lineEdit_DownloadLocation->setText(mSettingsHelper->getAppDownloadDir());
    // Agent
    uint8_t agentModeIndex  = 0;
    uint8_t agentMode     = mSettingsHelper->getAppAgentMode();
    QString agentServer   = mSettingsHelper->getAppAgentServer();
    uint16_t agentPort    = mSettingsHelper->getAppAgentPort();
    QString agentUserName = mSettingsHelper->getAppAgentUsername();
    QString agentPassword = mSettingsHelper->getAppAgentPassword();
    switch (agentMode) {
    case QNetworkProxy::ProxyType::HttpProxy:   agentModeIndex = 1; break;
    case QNetworkProxy::ProxyType::Socks5Proxy: agentModeIndex = 2; break;
    case QNetworkProxy::ProxyType::NoProxy:     agentModeIndex = 0; break;
    default: agentModeIndex = 0;
    }
    this->ui->comboBox_AgentMode->setCurrentIndex(agentModeIndex);
    onCurrentIndexChangedComboBoxAgentMode(agentModeIndex);
    this->ui->lineEdit_Server->setText(agentServer);
    this->ui->lineEdit_Port->setText(QString::number(agentPort));
    this->ui->lineEdit_UserName->setText(agentUserName);
    this->ui->lineEdit_Password->setText(agentPassword);
    // SpinBox
    {
        const QSignalBlocker blocker(this->ui->spinBox_RefreshInterval);
        this->ui->spinBox_RefreshInterval->setValue(mSettingsHelper->getHeartbeatInterval());
    }
    // SpinBox
    {
        const QSignalBlocker blocker(this->ui->spinBox_DebounceDelay);
        this->ui->spinBox_DebounceDelay->setValue(mSettingsHelper->getAppDeboundDelay());
    }

    // Debug Page
    {
        const QSignalBlocker blocker(this->ui->checkBox_DbgEnable);
        this->ui->checkBox_DbgEnable->setChecked(mSettingsHelper->getDebugEnable());
    }
}

void SettingsDialog::reloadAgentConfigurations()
{
    uint8_t agentMode     = mSettingsHelper->getAppAgentMode();
    QString agentServer   = mSettingsHelper->getAppAgentServer();
    uint16_t agentPort    = mSettingsHelper->getAppAgentPort();
    QString agentUserName = mSettingsHelper->getAppAgentUsername();
    QString agentPassword = mSettingsHelper->getAppAgentPassword();
    if (agentMode != QNetworkProxy::ProxyType::NoProxy) {
        mNetwork->setApplicationProxy((NetworkProxyType::ProxyType)agentMode, agentServer, agentPort, agentUserName, agentPassword);
    }
}
