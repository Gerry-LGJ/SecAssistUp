#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QDebug>
#include <QStandardPaths>
#include <QFileDialog>
#include <QNetworkProxy>
#include <QMessageBox>

#include "../service/mainservice.h"
#include "../window/mainwindow.h"
#include "../widgets/notificationform.h"
#include "../widgets/notificationbubble.h"
#include "../helper/translatehelper.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    mSettingsHelper      = SettingsHelper::getInstance();
    mFilTools            = FilTools::getInstance();
    mAppInfo             = AppInfo::getInstance();
    mNetwork             = Network::getInstance();
    mUpdateService       = UpdateService::getInstance();
    mAgentTestCallable   = new NetworkCallable(this);
    mCMWButtonGroup      = new QButtonGroup(this);
    mLanguageButtonGroup = new QButtonGroup(this);
    mUpdateButtonGroup   = new QButtonGroup(this);
    init();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::initRunAtSystemStartup()
{
#ifdef Q_OS_WIN
    {
        QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                           QSettings::NativeFormat);
        QString appName = QCoreApplication::applicationName();
        QString appPath = QCoreApplication::applicationFilePath();
        appPath = QDir::toNativeSeparators(appPath);
        if (settings.value(appName).toString() != appPath) {
            if (!settings.value(appName).toString().isEmpty()) {
                qDebug() << __func__ << "There is an invalid registry path.";
                settings.remove(appName); // delete invalid path
            }
        }
    }
#endif
}

void SettingsDialog::init()
{
    // Set as a modal window
    // setModal(true);
    setWindowModality(Qt::WindowModal);
    // set onClick handle function
    connect(this->ui->checkBox_DbgEnable,            &QCheckBox::stateChanged, this, &SettingsDialog::onDbgEnableStateChanged);
    connect(this->ui->checkBox_NotifyBubble,         &QCheckBox::stateChanged, this, &SettingsDialog::onNotifyBubbleStateChanged);
    connect(this->ui->checkBox_SystemTrayNotify,     &QCheckBox::stateChanged, this, &SettingsDialog::onSystemTrayNotifyStateChanged);
    connect(this->ui->checkBox_RunAtSystemStartup,   &QCheckBox::stateChanged, this, &SettingsDialog::onRunAtSystemStartupStateChanged);
    connect(this->ui->checkBox_StartMinimizedToTray, &QCheckBox::stateChanged, this, &SettingsDialog::onStartMinimizedToTrayStateChanged);
    connect(this->ui->pushButton_TestCrash,          &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonTestCrash);
    connect(this->ui->pushButton_Restart,            &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonRestart);
    connect(this->ui->pushButton_SystemTray,         &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonSystemTray);
    connect(this->ui->pushButton_Notification,       &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonNotification);
    connect(this->ui->pushButton_OpenLogFolder,      &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonOpenLoggerFolder);
    connect(this->ui->pushButton_OpenCfg,            &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonOpenConfiguration);
    connect(this->ui->spinBox_RefreshInterval,  QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::onValueChangedSpinBoxRefreshInterval);
    connect(this->ui->spinBox_DebounceDelay,    QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::onValueChangedSpinBox_DebounceDelay);
    connect(this->ui->pushButton_SelectDownloadLocation, &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonSelectDownloadLocation);
    connect(this->ui->comboBox_AgentMode,       QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SettingsDialog::onCurrentIndexChangedComboBoxAgentMode);
    connect(this->ui->pushButton_AgentTest,          &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonAgentTest);
    connect(this->ui->pushButton_AgentConfirm,       &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonAgentConfirm);
    connect(this->ui->pushButton_CheckForUpdates,    &QPushButton::clicked, this, &SettingsDialog::onClickPushButtonCheckUpdate);

    // init Close Main Window Radio Button
    mCMWButtonGroup->addButton(this->ui->radioButton_cmw_ask);
    mCMWButtonGroup->addButton(this->ui->radioButton_cmw_tray);
    mCMWButtonGroup->addButton(this->ui->radioButton_cmw_exit);
    connect(mCMWButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &SettingsDialog::onClickRadioButtonCloseMainWindow);

    // init Language Radio Button
    mLanguageButtonGroup->addButton(this->ui->radioButton_lang_en_US);
    mLanguageButtonGroup->addButton(this->ui->radioButton_lang_zh_CN);
    connect(mLanguageButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &SettingsDialog::onClickRadioButtonLanguage);

    // init Update Mode Radio Button
    mUpdateButtonGroup->addButton(this->ui->radioButton_ud_auto);
    mUpdateButtonGroup->addButton(this->ui->radioButton_ud_notify);
    connect(mUpdateButtonGroup, QOverload<QAbstractButton *>::of(&QButtonGroup::buttonClicked), this, &SettingsDialog::onClickRadioButtonUpdate);

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
    mAgentTesting = false;

    // update service
    this->ui->label_CheckForUpdatesMsg->setTextInteractionFlags(Qt::TextSelectableByMouse);
    connect(mUpdateService, &UpdateService::updateMsgChanged, this, [=] (QString msg) {

        // When the text exceeds the preset length, use an ellipsis.
        QFontMetrics fontMetrics(this->ui->label_CheckForUpdatesMsg->font());
        QString elidedText = fontMetrics.elidedText(msg, Qt::ElideRight, this->ui->label_CheckForUpdatesMsg->width() - 10);

        this->ui->label_CheckForUpdatesMsg->setText(elidedText);
        this->ui->label_CheckForUpdatesMsg->setToolTip(msg);

    });
    this->ui->label_CheckForUpdatesMsg->setText(mUpdateService->getUpdateMsg());
    connect(mUpdateService, &UpdateService::updateProgressChanged, this, &SettingsDialog::updateProgressChangedSlot);
    updateProgressChangedSlot(mUpdateService->getUpdateProgress());

    // Common Page
    this->ui->lineEdit_Port->setValidator(new QIntValidator(0, 65535, this->ui->lineEdit_Port));

    // About Page
    this->ui->label_Version->setText(mAppInfo->version());
    this->ui->label_BuildType->setText(mAppInfo->buildType());
    this->ui->label_QtVersion->setText(mAppInfo->buildQtVersion());
    this->ui->label_BuildTime->setText(QString("%1 %2").arg(mAppInfo->buildDate(), mAppInfo->buildTime()));
}

// void SettingsDialog::closeEvent(QCloseEvent *event)
// {
// #if 0
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
// #endif
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
    MainService *service = MainService::getInstance();
    service->exit(931);
}

void SettingsDialog::onClickPushButtonSystemTray()
{
    static int count                 = 0;
    MainService *mMainService        = MainService::getInstance();
    QSystemTrayIcon *mSystemTrayIcon = mMainService->getSystemTrayIcon();
    if (mSystemTrayIcon->isVisible()) {
        qDebug() << __func__ << QString("onClick PushButton Test System Tray Count:%1.").arg(count);
        mSystemTrayIcon->showMessage(tr("Friendly Reminder"),
                                     QString(tr("SecAssistUp Test System Tray Count:%1.")).arg(count++),
                                     QIcon(":/image/favicon_nbg.png"),
                                     1000);
    }
}

void SettingsDialog::onClickPushButtonNotification()
{
    qDebug() << __func__;
#if 0
    NotificationBubble *form = new NotificationBubble("The content of your message.", 5000);
    form->showAtScreenCorner();
#else
    static int count = 0;
    QStringList files;
    files << "test 1" << "test 2" << "test 3";
    NotificationForm *form = new NotificationForm(QString(tr("Test Notification Bubble %1")).arg(++count), files);
    form->showAtScreenCorner();
#endif
    qDebug() << __func__ << "Return.";
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

void SettingsDialog::onNotifyBubbleStateChanged(int state)
{
    qDebug() << __func__ << "state: " << state;
    if (state == Qt::Unchecked) {
        mSettingsHelper->saveAppNotifyBubble(false);
    } else if (state == Qt::PartiallyChecked) {
    } else if (state == Qt::Checked) {
        mSettingsHelper->saveAppNotifyBubble(true);
    }
}

void SettingsDialog::onSystemTrayNotifyStateChanged(int state)
{
    qDebug() << __func__ << "i:" << state;
    if (state == Qt::Unchecked) {
        mSettingsHelper->saveAppSystemNotify(false);
    } else if (state == Qt::PartiallyChecked) {
    } else if (state == Qt::Checked) {
        mSettingsHelper->saveAppSystemNotify(true);
    }
}

void SettingsDialog::onClickRadioButtonLanguage(QAbstractButton *button)
{
    if (button == this->ui->radioButton_lang_en_US) {
        mSettingsHelper->saveLanguage("en_US");
        TranslateHelper::getInstance()->switchLanguage("en_US");
    } else if (button == this->ui->radioButton_lang_zh_CN) {
        mSettingsHelper->saveLanguage("zh_CN");
        TranslateHelper::getInstance()->switchLanguage("zh_CN");
    } else {
        mSettingsHelper->saveLanguage("en_US");
        TranslateHelper::getInstance()->switchLanguage("en_US");
    }
}

void SettingsDialog::onClickRadioButtonCloseMainWindow(QAbstractButton *button)
{
    if (button == this->ui->radioButton_cmw_ask) {
        mSettingsHelper->saveAppCloseMainWindow("ask");
    } else if (button == this->ui->radioButton_cmw_tray) {
        mSettingsHelper->saveAppCloseMainWindow("tray");
    } else if (button == this->ui->radioButton_cmw_exit) {
        mSettingsHelper->saveAppCloseMainWindow("exit");
    } else {
        mSettingsHelper->saveAppCloseMainWindow("ask");
    }
}

void SettingsDialog::onRunAtSystemStartupStateChanged(int state)
{
    qDebug() << __func__ << "state:" << state;
#ifdef Q_OS_WIN
    // 使用 NativeFormat 来操作 Windows 注册表
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                       QSettings::NativeFormat);

    QString appName = QCoreApplication::applicationName();
    QString appPath = QCoreApplication::applicationFilePath();

    // 将路径转换为 Windows 原生格式（反斜杠）
    appPath = QDir::toNativeSeparators(appPath);

    if (state == Qt::Checked) {
        settings.setValue(appName, appPath);
    } else {
        settings.remove(appName);
    }
#endif
}

void SettingsDialog::onStartMinimizedToTrayStateChanged(int state)
{
    qDebug() << __func__ << "i:" << state;
    if (state == Qt::Unchecked) {
        mSettingsHelper->saveAppStartMinimizedToTray(false);
    } else if (state == Qt::PartiallyChecked) {
    } else if (state == Qt::Checked) {
        mSettingsHelper->saveAppStartMinimizedToTray(true);
    }
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

void SettingsDialog::onClickRadioButtonUpdate(QAbstractButton *button)
{
    if (button == this->ui->radioButton_ud_auto) {
        mSettingsHelper->saveAppUpdateMode(0);
    } else if (button == this->ui->radioButton_ud_notify) {
        mSettingsHelper->saveAppUpdateMode(1);
    } else {
        mSettingsHelper->saveAppUpdateMode(1);
    }
}

void SettingsDialog::onClickPushButtonCheckUpdate()
{
    mUpdateService->checkForUpdate(false);
}

void SettingsDialog::updateProgressChangedSlot(int progress)
{
    QProgressBar *progressBar = this->ui->progressBar_UpdateDownload;

    if (0 <= progress && progress <= 100) {
        progressBar->setVisible(true);
        progressBar->setValue(progress);
    } else {
        progressBar->setVisible(false);
    }
}

bool SettingsDialog::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    return QDialog::event(e);
}

void SettingsDialog::reloadConfigurationsFromSettings()
{
    qDebug() << __func__;
    // // Common Page
    // LineEdit
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
    // CheckBox
    {
        const QSignalBlocker blocker(this->ui->checkBox_NotifyBubble);
        this->ui->checkBox_NotifyBubble->setChecked(mSettingsHelper->getAppNotifyBubble());
    }
    {
        const QSignalBlocker blocker(this->ui->checkBox_SystemTrayNotify);
        this->ui->checkBox_SystemTrayNotify->setChecked(mSettingsHelper->getAppSystemNotify());
    }
    {
        const QSignalBlocker blocker(this->ui->checkBox_StartMinimizedToTray);
        this->ui->checkBox_StartMinimizedToTray->setChecked(mSettingsHelper->getAppStartMinimizedToTray());
    }
#ifdef Q_OS_WIN
    {
        QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                           QSettings::NativeFormat);
        QString appName = QCoreApplication::applicationName();
        QString appPath = QCoreApplication::applicationFilePath();
        appPath = QDir::toNativeSeparators(appPath);
        if (settings.value(appName).toString() == appPath) {
            const QSignalBlocker blocker(this->ui->checkBox_RunAtSystemStartup);
            this->ui->checkBox_RunAtSystemStartup->setChecked(true);
        }
    }
#endif

    // Radio Button
    {
        QString mode = mSettingsHelper->getAppCloseMainWindow();
        const QSignalBlocker blocker(this->mCMWButtonGroup);
        if (mode == "ask") {
            this->ui->radioButton_cmw_ask->setChecked(true);
        } else if (mode == "tray") {
            this->ui->radioButton_cmw_tray->setChecked(true);
        } else if (mode == "exit") {
            this->ui->radioButton_cmw_exit->setChecked(true);
        } else { // default "ask"
            this->ui->radioButton_cmw_ask->setChecked(true);
        }
    }
    {
        QString lang = mSettingsHelper->getLanguage(TranslateHelper::getInstance()->getLocaleLanguage());
        const QSignalBlocker blocker(this->mLanguageButtonGroup);
        if (lang == "en_US") {
            this->ui->radioButton_lang_en_US->setChecked(true);
        } else if (lang == "zh_CN") {
            this->ui->radioButton_lang_zh_CN->setChecked(true);
        } else {
            this->ui->radioButton_lang_en_US->setChecked(true);
        }
    }
    {
        unsigned int mode = mSettingsHelper->getAppUpdateMode();
        const QSignalBlocker blocker(this->mUpdateButtonGroup);
        if (mode == 0) {
            this->ui->radioButton_ud_auto->setChecked(true);
        } else if (mode == 1) {
            this->ui->radioButton_ud_notify->setChecked(true);
        } else {
            this->ui->radioButton_ud_auto->setChecked(true);
        }
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
