#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTableWidget>
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#include <QWinThumbnailToolBar>
#include <QWinThumbnailToolButton>

#include "../service/msevent.h"
#include "../common/singleton.h"
#include "../common/filedata_t.h"
#include "../helper/projectdbhelper.h"
#include "../helper/uploadfiledbhelper.h"
#include "../helper/filtools.h"
#include "../helper/settingshelper.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    SINGLETON(MainWindow)
    void init();
    bool updateProjectInfoToUI();
    bool updateUploadFilesInfoToUI();
    QTableWidget *getTableWidgetProjects()                   { return mTableWidgetProjects; }
    QListWidget  *getListWidgetWebFiles()                    { return mListWidgetWebFiles; }
    QList<project_info_t> getProjectsInfo()                  { return mProjectsInfo; }
    void setRestoreSelectPid(QString pid);
    project_info_t getActiveProjectInfo()                    { return mActiveProjectInfo; }
    void setProgressBarDownload(int value, QString text);
    void setProgressBarUpload(int value, QString text);
    void setCountdownLabelText(QString text);
    void setLabelLoginUser(const QString &text);


protected:
    bool event(QEvent *e) override ;
    void closeEvent(QCloseEvent *event) override ;
    void changeEvent(QEvent *event) override ;
    bool eventFilter(QObject *watched, QEvent *event) override ;

private:
    Ui::MainWindow *ui;
    QListWidget                 *mListWidgetWebFiles;
    QTableWidget                *mTableWidgetProjects;
    QListWidget                 *mListWidgetUploadFiles;
    ProjectDbHelper             *mProjectDbHelper;
    ProjectDbHelperCallable     *mPDBHCbReadAll;
    ProjectDbHelperCallable     *mPDBHCbSearch;
    UploadFileDbHelper          *mUploadFileDbHelper;
    bool                         mPDBHCbReadingAll;
    bool                         mPDBHCbSearching;
    UploadFileDbHelperCallable  *mUFDBHCbReadAllByPid;
    bool                         mUFDBHCbReadingAllByPid;
    project_info_t               mActiveProjectInfo;
    QString                      mRestoreProjectsSelectPid;
    QString                      mRestoreUploadFilesSelectPid;
    QList<project_info_t>        mProjectsInfo;
    QList<uf_info_t>             mUploadFilesInfo;
    FilTools                    *mFilTools;
    SettingsHelper              *mSettingsHelper;
    QWinTaskbarButton           *mWinTaskbarButton;
    QWinTaskbarProgress         *mWinTaskbarProgress;
    QWinThumbnailToolBar        *mWinThumbnailToolBar;

    // func
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void initPushButton(void);
    void initListWidgetWebFiles(void);
    void initTableWidgetProjects(void);
    void initListWidgetUploadFiles(void);
    void initProgressBar(void);
    void initThumbnailToolBar(void);
    void initPDBHCbReadAll(void);
    void initPDBHCbSearch(void);
    void initUFDBHCbReadAllByPid(void);
    void onClickPushButtonDownload(void);
    void onClickPushButtonUpload_l(void);
    void onClickPushButtonUpload_r(void);
    void updateTableWidgetDataFromProjectsInfo();
    void updateListWidgetDataFromUploadFilesInfo();
    bool updateWebFilesToUI();
    void retranslate();
    void setSelectProjectName(QString text);
    void setSelectDownloadMode(int index);
    // Notification Bubble
    void notifyBubble(const QString &title, const QStringList &files);
    void pushRecorder(const QStringList &files, bool isDownlaod);

    bool mSEventHandler(MSEvent *e);
};
#endif // MAINWINDOW_H
