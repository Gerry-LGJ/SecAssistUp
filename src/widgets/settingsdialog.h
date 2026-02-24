#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QWidget>
#include <QEventLoop>
#include <QButtonGroup>

#include "../helper/settingshelper.h"
#include "../helper/appinfo.h"
#include "../helper/filtools.h"
#include "../helper/network.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

public:
    enum {
        AGENT_MODE_NO_PROXY,
        AGENT_MODE_HTTP_PROXY,
        AGENT_MODE_SOCKS5_PROXY
    };
    static void initRunAtSystemStartup(void);

// protected:
//     void closeEvent(QCloseEvent *event) override ;

private slots:
    // Debug Page
    void onDbgEnableStateChanged(int state);
    void onClickPushButtonTestCrash();
    void onClickPushButtonRestart();
    void onClickPushButtonSystemTray();
    void onClickPushButtonNotification();
    void onClickPushButtonOpenLoggerFolder();
    void onClickPushButtonOpenConfiguration();
    // Common Page
    void onValueChangedSpinBoxRefreshInterval(int i);
    void onClickPushButtonSelectDownloadLocation();
    void onNotifyBubbleStateChanged(int state);
    void onSystemTrayNotifyStateChanged(int state);
    void onClickRadioButtonCloseMainWindow(QAbstractButton *button);
    void onRunAtSystemStartupStateChanged(int state);
    void onStartMinimizedToTrayStateChanged(int state);
    void onValueChangedSpinBox_DebounceDelay(int i);
    void onCurrentIndexChangedComboBoxAgentMode(int index);
    void onClickPushButtonAgentTest();
    void onClickPushButtonAgentConfirm();

private:
    Ui::SettingsDialog *ui;
    SettingsHelper     *mSettingsHelper;
    FilTools           *mFilTools;
    AppInfo            *mAppInfo;
    bool                mAgentTesting;
    Network            *mNetwork;
    NetworkCallable    *mAgentTestCallable;
    QButtonGroup       *mCMWButtonGroup;

    void init();
    void reloadConfigurationsFromSettings();
    void reloadAgentConfigurations();
};

#endif // SETTINGSDIALOG_H
