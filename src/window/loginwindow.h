#pragma once

#include <QMainWindow>
#include <QEvent>
#include <QCloseEvent>

#include "../helper/settingshelper.h"
#include "../common/singleton.h"
#include "../widgets/settingsdialog.h"
#include "../service/msevent.h"

class MainService;

namespace Ui {
class LoginWindow;
}

class LoginWindow : public QMainWindow
{
    Q_OBJECT

private:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

public:
    SINGLETON(LoginWindow)
    void init();
    bool getRememberMe();
    bool getAutoLogin();

private slots:
    void onClickPushButtonSignIn();
    void onClickPushButtonSettings();
    void onStateChangedRememberMe(int state);
    void onStateChangedAutoLogin(int state);

protected:
    bool event(QEvent *e) override;
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::LoginWindow *ui;
    SettingsHelper  *mSettings;
    SettingsDialog  *mSettingsDialog;
    QObject         *mMainService;
    bool             mRememberMe;
    bool             mAutoLogin;

    bool mSEventHandler(MSEvent *e);
};

