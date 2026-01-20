#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

#include "../common/singleton.h"
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

private:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

public:
    enum {
        AGENT_MODE_NO_PROXY,
        AGENT_MODE_HTTP_PROXY,
        AGENT_MODE_SOCKS5_PROXY
    };
    SINGLETON(SettingsDialog)
    void init();

// protected:
//     void closeEvent(QCloseEvent *event) override ;

private slots:
    // Debug Page
    void onDbgEnableStateChanged(int state);
    void onClickPushButtonTestCrash();
    void onClickPushButtonRestart();
    void onClickPushButtonOpenLoggerFolder();
    void onClickPushButtonOpenConfiguration();
    // Common Page
    void onValueChangedSpinBoxRefreshInterval(int i);
    void onValueChangedSpinBox_DebounceDelay(int i);
    void onClickPushButtonSelectDownloadLocation();
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

    void reloadConfigurationsFromSettings();
    void reloadAgentConfigurations();
};

#endif // SETTINGSDIALOG_H
