#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDebug>
#include <QIcon>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMenu>

#include "../common/filedata_t.h"
#include "../service/refreshservice.h"
#include "../service/mainservice.h"
#include "../service/recorderservice.h"
#include "../widgets/settingsdialog.h"
#include "../widgets/projectinfodialog.h"
#include "../widgets/notificationform.h"
#include "../widgets/recorderdialog.h"
#include "../helper/filicontools.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mListWidgetWebFiles    = this->ui->listWidget_WebFiles;
    mTableWidgetProjects   = this->ui->tableWidget_Project;
    mListWidgetUploadFiles = this->ui->listWidget_UploadFiles;
    mProjectDbHelper       = ProjectDbHelper::getInstance();
    mUploadFileDbHelper    = UploadFileDbHelper::getInstance();
    mFilTools              = FilTools::getInstance();
    mSettingsHelper        = SettingsHelper::getInstance();
    mWinTaskbarButton      = new QWinTaskbarButton(this);
    mWinThumbnailToolBar   = new QWinThumbnailToolBar(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init()
{
    // The database callback must be initialized properly before executing the initListWidgetWebFiles() function.
    initPDBHCbReadAll();
    initPDBHCbSearch();
    initUFDBHCbReadAllByPid();
    initPushButton();
    initListWidgetWebFiles();
    initTableWidgetProjects();
    initListWidgetUploadFiles();
    initProgressBar();
    initThumbnailToolBar();
    updateProjectInfoToUI();

    int index     = mSettingsHelper->getAppDownloadMode();
    { // not send QComboBox::currentIndexChanged Signal
        const QSignalBlocker blocker(this->ui->comboBox_SelectMode);
        this->ui->comboBox_SelectMode->setCurrentIndex(index);
        if (index == 0) {
            setSelectProjectName(mSettingsHelper->getAppDownloadDir());
        }
    }
    bool override = mSettingsHelper->getAppDownloadOverride();
    this->ui->radioButton_Override->setChecked(override);
}

void MainWindow::initPushButton()
{
    qDebug() << __func__;
    MainService *mMainService = MainService::getInstance();

    // Refresh Button
    connect(this->ui->pushButton_Refresh, &QPushButton::clicked, this, [=] {
        MSEvent *event = new MSEvent(this, MSEvent::EVENT_TYPE_REFRESH_REQ);
        QCoreApplication::postEvent(mMainService, event);
    });

    // Return Button
    connect(this->ui->pushButton_Return, &QPushButton::clicked, this, [=] {
        MSEvent *event = new MSEvent(this, MSEvent::EVENT_TYPE_RETURN_PARENT_DIR_REQ);
        QCoreApplication::postEvent(mMainService, event);
    });

    // Recorder Button
    connect(this->ui->pushButton_Recorder, &QPushButton::clicked, this, [=] {
        RecorderDialog recorder(this);
        recorder.exec();
    });
    this->ui->pushButton_Recorder->setFont(FilIconTools::font());
    this->ui->pushButton_Recorder->setText(FilIconTools::convert(FilIcons::Type::History));

    // Settings Button
    connect(this->ui->pushButton_Settings, &QPushButton::clicked, this, [=] {
        SettingsDialog settings(this);
        settings.exec();
    });
    this->ui->pushButton_Settings->setFont(FilIconTools::font());
    this->ui->pushButton_Settings->setText(FilIconTools::convert(FilIcons::Type::Settings));

    // Logout Button
    connect(this->ui->pushButton_Logout, &QPushButton::clicked, this, [=] {
        FileWatcherService *mFWService  = FileWatcherService::getInstance();
        mFWService->cleanup();
        RefreshService *mRefreshService = RefreshService::getInstance();
        mRefreshService->stopCountdown();
        QVariantMap map;
        map["resultCode"]               = MSEvent::RESULT_CODE_SUCCESS;
        MSEvent *event                  = new MSEvent(this, MSEvent::EVENT_TYPE_LOGOUT_REQ);
        event->setData(map);
        QCoreApplication::postEvent(mMainService, event);
    });

    // Select Mode ComBoBox
    connect(this->ui->comboBox_SelectMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=] (int index) {
        setSelectDownloadMode(index);
    });

    // Add Project Button
    connect(this->ui->pushButton_AddProject, &QPushButton::clicked, this, [=] {
        ProjectInfoDialog *mProjectInfoDialog = ProjectInfoDialog::getInstance();
        QString inputRet = QInputDialog::getText(this, tr("New Project"), tr("Please input the project name:"));

        if (inputRet.length() <= 0) {
            qDebug() << "Cancel Add Project";

        } else {
            QString dateTimeStr = mFilTools->getCurrentDateTimeToString("yyyyMMddHHmmsszzz");
            if (mProjectDbHelper->add(dateTimeStr, inputRet.left(64), "", "", false, "", false, false, "")) {
                qDebug() << "Add New Project Succeed. pid:" << dateTimeStr;
                // updateProjectInfoToUI();
                mProjectInfoDialog->openWithPid(dateTimeStr);
                qDebug() << "Modify Project Info Done.";
            } else {
                qWarning() << "Add New Project Failure.";
            }
        }
    });

    // Remove Project Button
    connect(this->ui->pushButton_RemoveProject, &QPushButton::clicked, this, [=] {
        int index = mTableWidgetProjects->currentRow();
        if (index >= 0) {
            project_info_t info = mProjectsInfo.at(index);
            QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Reminder"),
                                                                      QString(tr("Are you sure you want to delete:\n%1(%2)\n?")).arg(info.name, info.pid),
                                                                      QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                qDebug() << "User Delete name:" << info.name << "pid:" << info.pid;
                if (mRestoreProjectsSelectPid == info.pid) {
                    qDebug() << "clean up mRestoreSelectPid";
                }
                if (mProjectDbHelper->del(info.pid)) {
                    updateProjectInfoToUI();
                }
            } else {
                qDebug() << "User cancel delete project";
            }
        } else {
            qDebug() << "Not Select CurrentRow()";
        }
    });

    // Override Radio Button
    connect(this->ui->radioButton_Override, &QRadioButton::clicked, this, [=] (bool checked) {
        qDebug() << "on Override Radio Button State Changed:" << checked;
        mSettingsHelper->saveAppDownloadOverride(checked);
    });

    // Add File Button
    connect(this->ui->pushButton_AddFile, &QPushButton::clicked, this, [=] {
        QStringList files = QFileDialog::getOpenFileNames(this, tr("Open Files"));

        // if user cancel select
        if (files.isEmpty()) {
            qDebug() << "User Cancel Select";
        }
        qDebug() << "Add UploadFiles::Open files:" << files;

        // get every file info
        int okcount = 0;
        for (int i  = 0; i < files.size(); ++i) {
            QFileInfo fileInfo(files.at(i));
            uf_info_t info;
            info.fid    = mFilTools->getCurrentDateTimeToString("yyyyMMddHHmmsszzz") + QString("%1").arg(i);
            info.pid    = mActiveProjectInfo.pid;
            info.name   = fileInfo.fileName();
            info.dir    = fileInfo.path();
            info.enable = false;
            if (!mUploadFileDbHelper->add(info.fid, info.pid, info.name, info.dir, info.enable)) {
                qWarning() << "Add Upload Files Failure.";
            }
        }
        // update UI data
        updateUploadFilesInfoToUI();
    });

    // Remove File Button
    connect(this->ui->pushButton_RemoveFile, &QPushButton::clicked, this, [=] {
        qDebug() << "Remove UploadFiles";
        UploadFileDbHelper *helper     = mUploadFileDbHelper;
        QList<uf_info_t> files         = mUploadFilesInfo;
        QListWidget *lwufs             = mListWidgetUploadFiles;
        QList<QListWidgetItem *> items = lwufs->selectedItems();
        if (items.size() > 0) {
            QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Reminder"), tr("Are you sure you want to delete the selected files?"));
            if (reply == QMessageBox::Yes) {
                for (int i = 0; i < items.size(); ++i) {
                    uf_info_t info = files.at(lwufs->row(items.at(i)));
                    qDebug() << "Delete Upload File(fid):" << info.fid << "name:" << info.name;
                    if (!helper->del(info.fid)) {
                        qWarning() << "Delete Upload File(fid):" << info.fid << "Failure.";
                    }
                }
                updateUploadFilesInfoToUI();
            } else {
                qDebug() << "User cancel delete upload file";
            }
        } else {
            qDebug() << "Not Select items";
        }
    });

    // Enable File System Watcher Radio Button
    connect(this->ui->radioButton_EnableFsw, &QRadioButton::clicked, this, [=] (bool checked) {
        ProjectDbHelper    *helper     = mProjectDbHelper;
        FileWatcherService *mFWService = FileWatcherService::getInstance();
        QString            pid         = mActiveProjectInfo.pid;
        int                ret         = checked ? 1 : 0;
        qDebug() << "on Enable File System Watcher Radio Button State Changed:" << checked;
        if (checked) {
            mFWService->restart();
        } else {
            mFWService->stop();
        }
        if (pid.length() > 0) {
            if (!helper->updateRecordByPid(pid, ProjectDbHelperColumnIndexType::CIT_FSW, QString("%1").arg(ret))) {
                qWarning() << "updateRecordByPid fsw Failure.";
            } else {
                // 修改完数据库之后直接修改缓存变量的值即可
                // updateProjectInfoToUI();
                mActiveProjectInfo.fsw = checked;
                for (int i = 0; i < mProjectsInfo.size(); ++i) {
                    project_info_t info = mProjectsInfo.at(i);
                    if (info.pid == pid) {
                        info.fsw = checked;
                        mProjectsInfo[i] = info;
                    }
                }
            }
        } else {
            qDebug() << "Invalid mActiveProjectInfo";
        }
    });

    /* Buttom Bar */
    // Downlaod Button
    connect(this->ui->pushButton_Download, &QPushButton::clicked, this, &MainWindow::onClickPushButtonDownload);

    // Upload_l Button
    connect(this->ui->pushButton_Upload_l, &QPushButton::clicked, this, &MainWindow::onClickPushButtonUpload_l);

    // Upload_r Button
    connect(this->ui->pushButton_Upload_r, &QPushButton::clicked, this, &MainWindow::onClickPushButtonUpload_r);
}

void MainWindow::initListWidgetWebFiles()
{
    qDebug() << __func__;
    mListWidgetWebFiles->installEventFilter(this);
    mListWidgetWebFiles->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(mListWidgetWebFiles, &QListWidget::itemDoubleClicked, this, [=] (QListWidgetItem *item) {
        int index                       = mListWidgetWebFiles->row(item);
        RefreshService *mRefreshService = RefreshService::getInstance();
        QList<file_t> files             = mRefreshService->getFileList();
        qDebug() << "MainWindow::initListWidgetWebFiles onDoubleClicked" << index;
        file_t fd = files.at(index);
        if (!fd.isFile) {
            QVariantMap map;
            MSEvent *event = new MSEvent(this, MSEvent::EVENT_TYPE_ENTRY_FOLDER_REQ);
            map["name"]    = fd.name;
            QVariant var   = map;
            event->setData(var);
            QCoreApplication::postEvent(MainService::getInstance(), event);
        }
    });

    // 右键菜单
    QAction *copyAction     = new QAction(tr("Copy")    , this);
    connect(copyAction,     &QAction::triggered, this, [=] {
        RefreshService *mRefreshService = RefreshService::getInstance();
        OtherService   *mOtherService   = OtherService::getInstance();
        QList<file_t> files             = mRefreshService->getFileList();
        QString webdir                  = mRefreshService->getPathByPathList(mRefreshService->getPathList());
        QList<copy_info_t> copylist        ;
        QListWidget *lwwfs              = mListWidgetWebFiles;
        QList<QListWidgetItem *> items  = lwwfs->selectedItems();

        if (items.size() > 0) {
            for (int i = 0; i < items.size(); ++i) {
                copy_info_t cpinfo;
                file_t info  = files.at(lwwfs->row(items.at(i)));
                cpinfo.type = info.type;
                cpinfo.path = webdir + "/" + info.name;
                copylist.append(cpinfo);
            }
            QVariantMap map;
            MSEvent *event = new MSEvent(this, MSEvent::EVENT_TYPE_COPY_REQ);
            map["list"]    = QVariant::fromValue(copylist);
            event->setData(map);
            QCoreApplication::postEvent(mOtherService, event);
        }
    });

    QAction *cutAction      = new QAction(tr("Cut")     , this);
    connect(cutAction,      &QAction::triggered, this, [=] {
        RefreshService *mRefreshService = RefreshService::getInstance();
        OtherService   *mOtherService   = OtherService::getInstance();
        QList<file_t> files             = mRefreshService->getFileList();
        QString webdir                  = mRefreshService->getPathByPathList(mRefreshService->getPathList());
        QList<copy_info_t> cutlist        ;
        QListWidget *lwwfs              = mListWidgetWebFiles;
        QList<QListWidgetItem *> items  = lwwfs->selectedItems();

        if (items.size() > 0) {
            for (int i = 0; i < items.size(); ++i) {
                cut_info_t cutinfo;
                file_t info  = files.at(lwwfs->row(items.at(i)));
                cutinfo.type = info.type;
                cutinfo.path = webdir + "/" + info.name;
                cutlist.append(cutinfo);
            }
            QVariantMap map;
            MSEvent *event = new MSEvent(this, MSEvent::EVENT_TYPE_CUT_REQ);
            map["list"]    = QVariant::fromValue(cutlist);
            event->setData(map);
            QCoreApplication::postEvent(mOtherService, event);
        }
    });

    QAction *pasteAction    = new QAction(tr("Paste")   , this);
    connect(pasteAction,   &QAction::triggered, this, [=] {
        RefreshService *mRefreshService = RefreshService::getInstance();
        OtherService   *mOtherService   = OtherService::getInstance();
        QString         webdir          = mRefreshService->getPathByPathList(mRefreshService->getPathList());

        QVariantMap map;
        MSEvent *event = new MSEvent(this, MSEvent::EVENT_TYPE_PASTE_REQ);
        map["path"] = webdir;
        event->setData(map);
        QCoreApplication::postEvent(mOtherService, event);

    });

    QAction *renameAction   = new QAction(tr("Rename")  , this);
    connect(renameAction,   &QAction::triggered, this, [=] {
        RefreshService *mRefreshService = RefreshService::getInstance();
        OtherService   *mOtherService   = OtherService::getInstance();
        QList<file_t> files             = mRefreshService->getFileList();
        QString webdir                  = mRefreshService->getPathByPathList(mRefreshService->getPathList());
        QListWidget *lwwfs              = mListWidgetWebFiles;
        QList<QListWidgetItem *> items  = lwwfs->selectedItems();

        if (items.size() == 1) {
            file_t info      = files.at(lwwfs->row(items.at(0)));
            QString inputRet = QInputDialog::getText(this, tr("Rename"), QString(tr("Rename %1 as:")).arg(info.name),
                                                     QLineEdit::Normal, info.name);
            QString path     = webdir + "/" + info.name;
            QString rname_to   ;

            if (inputRet.length() > 0) {
                rname_to = webdir + "/" + inputRet.left(64);

                QVariantMap map;
                MSEvent *event  = new MSEvent(this, MSEvent::EVENT_TYPE_RENAME_REQ);
                map["path"]     = path;
                map["rname_to"] = rname_to;
                event->setData(map);
                QCoreApplication::postEvent(mOtherService, event);

            } else {
                qDebug() << "Cancel Rename File";
            }

        } else {
            qDebug() << "renameAction more select, disallow rename";
        }
    });

    QAction *deleteAction   = new QAction(tr("Delete")  , this);
    connect(deleteAction,   &QAction::triggered, this, [=] {
        RefreshService *mRefreshService = RefreshService::getInstance();
        OtherService   *mOtherService   = OtherService::getInstance();
        QList<file_t> files             = mRefreshService->getFileList();
        QString webdir                  = mRefreshService->getPathByPathList(mRefreshService->getPathList());
        QList<del_info_t> dellist         ;
        QListWidget *lwwfs              = mListWidgetWebFiles;
        QList<QListWidgetItem *> items  = lwwfs->selectedItems();

        QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Reminder"),
                                                                  tr("Are you sure you want to delete the selected content?"));
        if (reply == QMessageBox::Yes) {
            if (items.size() > 0) {
                for (int i = 0; i < items.size(); ++i) {
                    del_info_t delinfo;
                    file_t info  = files.at(lwwfs->row(items.at(i)));
                    delinfo.type = info.type;
                    delinfo.path = webdir + "/" + info.name;
                    dellist.append(delinfo);
                }
                QVariantMap map;
                MSEvent *event = new MSEvent(this, MSEvent::EVENT_TYPE_DELETE_REQ);
                map["list"]    = QVariant::fromValue(dellist);
                event->setData(map);
                QCoreApplication::postEvent(mOtherService, event);
            }
        } else {
            qDebug() << "User cancel delete web file";
        }
    });

    QAction *mkdirAction    = new QAction(tr("New Folder"),this);
    connect(mkdirAction,    &QAction::triggered, this, [=] {
        RefreshService *mRefreshService = RefreshService::getInstance();
        OtherService   *mOtherService   = OtherService::getInstance();
        QString webdir                  = mRefreshService->getPathByPathList(mRefreshService->getPathList());

        QString inputRet = QInputDialog::getText(this, tr("New Folder"), tr("Please enter the folder name:"),
                                                 QLineEdit::Normal, tr("New Folder"));
        if (inputRet.length() > 0) {

            QVariantMap map;
            QString path    = webdir + "/" + inputRet.left(64);
            MSEvent *event  = new MSEvent(this, MSEvent::EVENT_TYPE_MKDIR_REQ);
            map["path"]     = path;
            event->setData(map);
            QCoreApplication::postEvent(mOtherService, event);

        } else {
            qDebug() << "Cancel Make Directory";
        }

    });

    QAction *uploadAction   = new QAction(tr("Upload")  , this);
    connect(uploadAction,   &QAction::triggered, this, &MainWindow::onClickPushButtonUpload_l);

    QAction *downloadAction = new QAction(tr("Download"), this);
    connect(downloadAction, &QAction::triggered, this, &MainWindow::onClickPushButtonDownload);

    connect(mListWidgetWebFiles, &QListWidget::customContextMenuRequested, this, [=] (const QPoint &pos) {
        QListWidgetItem *item = mListWidgetWebFiles->itemAt(pos);
        int selects           = mListWidgetWebFiles->selectedItems().count();
        QMenu menu;

        if (item && selects == 1) {
            menu.addAction(renameAction);
        }
        if (selects > 0) {
            menu.addAction(copyAction);
            menu.addAction(cutAction);
            menu.addAction(deleteAction);
            menu.addAction(downloadAction);
        }
        menu.addAction(pasteAction);
        menu.addAction(uploadAction);
        menu.addAction(mkdirAction);

        menu.exec(mListWidgetWebFiles->mapToGlobal(pos));
    });
}

void MainWindow::initTableWidgetProjects()
{
    qDebug() << __func__;
    mProjectsInfo.clear();
    mTableWidgetProjects->setColumnCount(5);
    connect(mTableWidgetProjects, &QTableWidget::itemChanged, this, [=] (QTableWidgetItem *item) {
        ProjectDbHelper *helper     = mProjectDbHelper;
        QList<project_info_t> &list = mProjectsInfo;
        QTableWidget *twps          = mTableWidgetProjects;
        int row                     = item->row();
        int column                  = item->column();
        if (item->flags() & Qt::ItemIsUserCheckable) {
            Qt::CheckState state = item->checkState();
            if (mPDBHCbReadingAll == false) {
                qDebug() << "TableWidgetProjects onCheckStateChanged Row:" << row << "column:" << column << "state:" << state;
                project_info_t info = list.at(row);
                bool ret = false;
                if (column == 2) {
                    info.rds = state == Qt::Checked ? true : false;
                    ret = true;
                } else if (column == 3) {
                    info.rus = state == Qt::Checked ? true : false;
                    ret = true;
                }
                if (ret) {
                    twps->selectRow(row);
                    if (!helper->modify(info.pid,
                                   info.name,
                                   info.wdir,
                                   info.dspath,
                                   info.rds,
                                   info.uspath,
                                   info.rus,
                                   info.fsw,
                                        info.lstime)) {
                        qWarning() << "Modify Projects Info failure.";
                    }
                    // 此处因为是在槽函数当中，更新UI会导致item被清空，所以直接修改mProjectsInfo中缓存的值
                    list[row] = info;
                }
            }
        }
    });
    connect(mTableWidgetProjects, &QTableWidget::itemClicked, this, [=] (QTableWidgetItem *item) {
        project_info_t info = mProjectsInfo.at(item->row());
        mActiveProjectInfo  = info;
        qDebug() << "onItemClicked Row:" << item->row() << "column:" << item->column() << "pid:" << info.pid << "name:" << info.name;
        // 更新当前选择Project的Name
        setSelectProjectName(info.name);
        // 设置需要恢复选择的pid，以便刷新后重新自动选择
        setRestoreSelectPid(info.pid);
        // 更新主窗口右侧的上传文件信息列表
        this->ui->radioButton_EnableFsw->setChecked(info.fsw);
        updateUploadFilesInfoToUI();
        // 更新文件下载模式
        {
            const QSignalBlocker blocker(this->ui->comboBox_SelectMode);
            this->ui->comboBox_SelectMode->setCurrentIndex(1);
            setSelectDownloadMode(1);
        }
    });
}

void MainWindow::initListWidgetUploadFiles()
{
    qDebug() << __func__;
    mListWidgetUploadFiles->installEventFilter(this);

    connect(mListWidgetUploadFiles, &QListWidget::itemChanged, this, [=] (QListWidgetItem *item) {
        FileWatcherService *mFWService = FileWatcherService::getInstance();
        UploadFileDbHelper *helper     = mUploadFileDbHelper;
        QList<uf_info_t>  &list        = mUploadFilesInfo;
        int row                        = mListWidgetUploadFiles->row(item);
        if (item->flags() & Qt::ItemIsUserCheckable) {
            Qt::CheckState state = item->checkState();
            if (mUFDBHCbReadingAllByPid == false) {
                qDebug() << "ListWidgetUploadFiles onCheckStateChanged Row:" << row << "state:" << state;
                uf_info_t info = mUploadFilesInfo.at(row);
                info.enable    = state == Qt::Checked ? true : false;
                if (!helper->modify(info.fid, info.pid, info.name, info.dir, info.enable)) {
                    qWarning() << "Modify Upload File 'enable' state failure.";
                }
                mUploadFilesInfo[row] = info;
                if (info.enable) {
                    mFWService->addPath(info.dir + "/" + info.name);
                } else {
                    mFWService->removePath(info.dir + "/" + info.name);
                }
            }
        }
    });
    connect(mListWidgetUploadFiles, &QListWidget::itemDoubleClicked, this, [=] (QListWidgetItem *item) {
        UploadFileDbHelper *helper = mUploadFileDbHelper;
        int index = mListWidgetUploadFiles->row(item);
        qDebug() << "MainWindow::initListWidgetUploadFiles onDoubleClicked" << index;
        QString file = QFileDialog::getOpenFileName(this, tr("Open File"));
        if (file.length() > 0) {
            QFileInfo fileInfo = QFileInfo(file);
            uf_info_t ufInfo = mUploadFilesInfo.at(index);
            ufInfo.name = fileInfo.fileName();
            ufInfo.dir  = fileInfo.path();
            if (!helper->modify(ufInfo.fid, ufInfo.pid, ufInfo.name, ufInfo.dir, ufInfo.enable)) {
                qWarning() << "Modify Upload File Failure";
            }
            updateUploadFilesInfoToUI();
        }
    });
}

void MainWindow::initProgressBar()
{
    QProgressBar *download = this->ui->progressBar_Download;
    QProgressBar *upload   = this->ui->progressBar_Upload;
    QLabel *downloadFile   = this->ui->label_DownloadFile;
    QLabel *uploadFile     = this->ui->label_UploadFile;
    mWinTaskbarProgress    = nullptr;
    download->setVisible(false);
    upload->setVisible(false);
    downloadFile->setText("");
    uploadFile->setText("");
}

void MainWindow::initThumbnailToolBar()
{
    static bool initialized = false;
    QWindow *win            = windowHandle();
    if (initialized == false && win != nullptr) {
        initialized = true;
        qDebug() << __func__ << "MainWindow::initThumbnailToolBar" << win;
        mWinThumbnailToolBar->setWindow(win);
        QWinThumbnailToolButton *downBtn = new QWinThumbnailToolButton(mWinThumbnailToolBar);
        downBtn->setIcon(QIcon(":/image/download.png"));
        downBtn->setToolTip(tr("Downlaod"));
        downBtn->setDismissOnClick(true);
        connect(downBtn, &QWinThumbnailToolButton::clicked, this, [=] {
            // qDebug() << "MainWindow::initThumbnailToolBar onClick Downlaod Button.";
            onClickPushButtonDownload();
        });
        QWinThumbnailToolButton *upBtn   = new QWinThumbnailToolButton(mWinThumbnailToolBar);
        upBtn->setIcon(QIcon(":/image/upload.png"));
        upBtn->setToolTip(tr("Upload"));
        upBtn->setDismissOnClick(true);
        connect(upBtn, &QWinThumbnailToolButton::clicked, this, [=] {
            // qDebug() << "MainWindow::initThumbnailToolBar onClick Upload Button.";
            onClickPushButtonUpload_r();
        });

        mWinThumbnailToolBar->addButton(downBtn);
        mWinThumbnailToolBar->addButton(upBtn);
    }
}

void MainWindow::initPDBHCbReadAll()
{
    mPDBHCbReadAll              = new ProjectDbHelperCallable(this);
    ProjectDbHelperCallable *cb = mPDBHCbReadAll;
    mPDBHCbReadingAll           = false;

    connect(cb, &ProjectDbHelperCallable::start, this, [=] {
        qDebug() << "mPDBHCbReadAll" << "clean up mProjectsInfo";
        mProjectsInfo.clear();
        mPDBHCbReadingAll           = true;
    });
    connect(cb, &ProjectDbHelperCallable::finish, this, [=] {
        updateTableWidgetDataFromProjectsInfo();
        mPDBHCbReadingAll           = false;
    });
    connect(cb, &ProjectDbHelperCallable::error, this, [=] (QString errorString) {
        qWarning() << "MainWindow::initPDBHCbReadAll" << errorString;
    });
    connect(cb, &ProjectDbHelperCallable::readAll, this, [=] (
                                                             const QString pid,
                                                             const QString name,
                                                             const QString wdir,
                                                             const QString dspath,
                                                             const bool rds,
                                                             const QString uspath,
                                                             const bool rus,
                                                             const bool fsw,
                                                             const QString lstime) {
        project_info_t info = { pid, name, wdir, dspath, rds, uspath, rus, fsw, lstime };
        mProjectsInfo.append(info);
    });
}

void MainWindow::initUFDBHCbReadAllByPid()
{
    mUFDBHCbReadAllByPid           = new UploadFileDbHelperCallable(this);
    UploadFileDbHelperCallable *cb = mUFDBHCbReadAllByPid;
    mUFDBHCbReadingAllByPid        = false;

    connect(cb, &UploadFileDbHelperCallable::start, this, [=] {
        qDebug() << "mUFDBHCbReadAllByPid" << "clean up mUploadFilesInfo";
        mUploadFilesInfo.clear();
        mUFDBHCbReadingAllByPid = true;
    });
    connect(cb, &UploadFileDbHelperCallable::finish, this, [=] {
        updateListWidgetDataFromUploadFilesInfo();
        mUFDBHCbReadingAllByPid = false;
    });
    connect(cb, &UploadFileDbHelperCallable::error, this, [=] (QString errorString) {
        qWarning() << "MainWindow::initUFDBHCbReadAllByPid" << errorString;
    });
    connect(cb, &UploadFileDbHelperCallable::readAllByPid, this, [=] (
                                                                     const QString fid,
                                                                     const QString pid,
                                                                     const QString name,
                                                                     const QString dir,
                                                                     const bool    enable) {
        uf_info_t info = { fid, pid, name, dir, enable };
        mUploadFilesInfo.append(info);
    });
}

void MainWindow::onClickPushButtonDownload()
{
    qDebug() << __func__;
    MainService *mMainService       = MainService::getInstance();
    RefreshService *mRefreshService = RefreshService::getInstance();
    QList<file_t> list              = mRefreshService->getFileList();
    QListWidget *lwwfs              = mListWidgetWebFiles;
    QList<QListWidgetItem *> items  = lwwfs->selectedItems();
    QStringList downFiles;
    project_info_t pinfo            = mActiveProjectInfo;
    QString dir    = mSettingsHelper->getAppDownloadMode() == 0 ? mSettingsHelper->getAppDownloadDir() : pinfo.wdir;
    QString webdir = mRefreshService->getPathByPathList(mRefreshService->getPathList());
    if (!QFileInfo().exists(dir)) {
        QMessageBox::information(this, tr("Reminder"), tr("Download Directory Not Exists.") + "\n" + dir);
        return ;
    }
    qDebug() << "Append Download Files items.size():" << items.size();
    if (items.size() > 0) {
        for (int i = 0; i < items.size(); ++i) {
            file_t info = list.at(lwwfs->row(items.at(i)));
            if (info.isFile) {
                downFiles.append(info.name);
            }
        }
        qDebug() << "Downlaod files:" << downFiles;
        QVariantMap map;
        MSEvent *event = new MSEvent(this, MSEvent::EVENT_TYPE_DOWNLOAD_REQ);
        map["files"]   = downFiles;
        map["dir"]     = dir;
        map["webdir"]  = webdir;
        map["script"]  = (mSettingsHelper->getAppDownloadMode() == 1) ? (pinfo.rds ? pinfo.dspath : "") : "";
        event->setData(QVariant(map));
        QCoreApplication::postEvent(mMainService, event);
    }
}

void MainWindow::onClickPushButtonUpload_l()
{
    qDebug() << "onClick Upload_l Button";
    MainService *mMainService       = MainService::getInstance();
    RefreshService *mRefreshService = RefreshService::getInstance();
    QStringList files               = QFileDialog::getOpenFileNames(this, tr("Open Files"));
    project_info_t pinfo            = mActiveProjectInfo;
    QString webdir                  = mRefreshService->getPathByPathList(mRefreshService->getPathList());
    if (!files.isEmpty()) {
        qDebug() << "pushButton_Upload_l:" << files;
        QVariantMap map;
        MSEvent *event = new MSEvent(this, MSEvent::EVENT_TYPE_UPLOAD_REQ);
        map["files"]   = files;
        map["dir"]     = pinfo.wdir;
        map["webdir"]  = webdir;
        map["script"]  = "";
        event->setData(QVariant(map));
        QCoreApplication::postEvent(mMainService, event);
    } else {
        qDebug() << "pushButton_Upload_l User Cancel Select Files";
    }
}

void MainWindow::onClickPushButtonUpload_r()
{
    qDebug() << __func__;
    MainService *mMainService       = MainService::getInstance();
    RefreshService *mRefreshService = RefreshService::getInstance();
    QList<uf_info_t> files          = mUploadFilesInfo;
    QStringList uploadfiles;
    project_info_t pinfo            = mActiveProjectInfo;
    QString webdir                  = mRefreshService->getPathByPathList(mRefreshService->getPathList());

    if (files.size() > 0) {
        for (int i = 0; i < files.size(); ++i) {
            uf_info_t info = files.at(i);
            qDebug() << __func__ << "fid:" << info.fid << "pid:" << info.pid << "name:" << info.name << "dir:" << info.dir << "enable:" << info.enable;
            if (info.enable) {
                QString filepath = info.dir + "/" + info.name;
                uploadfiles.append(filepath);
            }
        }
        qDebug() << "pushButton_Upload_r:" << uploadfiles;
        QVariantMap map;
        MSEvent *event = new MSEvent(this, MSEvent::EVENT_TYPE_UPLOAD_REQ);
        map["files"]   = uploadfiles;
        map["dir"]     = pinfo.wdir;
        map["webdir"]  = webdir;
        map["script"]  = pinfo.rus ? pinfo.uspath : "";
        event->setData(QVariant(map));
        QCoreApplication::postEvent(mMainService, event);
    }
}

void MainWindow::initPDBHCbSearch()
{
    mPDBHCbSearch  = new ProjectDbHelperCallable(this);
}

/**
 * @brief MainWindow::updateTableWidgetDataFromProjectsInfo
 * 将mProjectsInfo数据渲染到TableWidget
 */
void MainWindow::updateTableWidgetDataFromProjectsInfo()
{
    qDebug() << __func__;
    int restoreSelectRow = -1;
    // apply mProjectsInfo to UI
    QList<project_info_t> &list = mProjectsInfo;
    for (int i = 0; i < list.size(); ++i) {
        project_info_t info = list.at(i);
        qDebug() << "pid:" << info.pid <<
            "name:" << info.name <<
            "wdir:" << info.wdir <<
            "dspath:" << info.dspath <<
            "rds:" << info.rds <<
            "uspath:" << info.uspath <<
            "rus:" << info.rus;
    }
    QTableWidget *twps = mTableWidgetProjects;
    QSignalBlocker blocker(twps);

    // 清理表格数据
    twps->clear();

    // 设置表头
    QStringList headers;
    headers << tr("Name") << tr("Work Dir") << tr("RDS") << tr("RUS") << tr("Options");
    mTableWidgetProjects->setHorizontalHeaderLabels(headers);
    twps->setRowCount(list.size());
    QHeaderView *horizontalHeader = twps->horizontalHeader();
    horizontalHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    // 渲染数据
    for (int i = 0; i < list.size(); ++i) {
        project_info_t info = list.at(i);
        if (mRestoreProjectsSelectPid == info.pid) restoreSelectRow = i;

        twps->setItem(i, 0, new QTableWidgetItem(info.name));

        QTableWidgetItem *wdir = new QTableWidgetItem(info.wdir);
        wdir->setToolTip(info.wdir);
        twps->setItem(i, 1, wdir);

        QTableWidgetItem *rds = new QTableWidgetItem();
        rds->setCheckState(info.rds ? Qt::Checked : Qt::Unchecked);
        rds->setFlags(rds->flags() & ~Qt::ItemIsEditable);
        rds->setFlags(rds->flags() | Qt::ItemIsUserCheckable);
        twps->setItem(i, 2, rds);

        QTableWidgetItem *rus = new QTableWidgetItem();
        rus->setCheckState(info.rus ? Qt::Checked : Qt::Unchecked);
        rus->setFlags(rus->flags() & ~Qt::ItemIsEditable);
        rus->setFlags(rus->flags() | Qt::ItemIsUserCheckable);
        twps->setItem(i, 3, rus);

        // QPushButton *btn = new QPushButton(tr("Options"));
        QPushButton *btn = new QPushButton(FilIconTools::convert(FilIcons::Type::More));
        btn->setFont(FilIconTools::font());
        btn->setProperty("row", i);
        connect(btn, &QPushButton::clicked, this, [=] {
            int row = btn->property("row").toInt();
            qDebug() << "onClick TableWidgetProjects row:" << row;
            ProjectInfoDialog *dialog = ProjectInfoDialog::getInstance();
            dialog->openWithPid(list.at(row).pid);
        });
        twps->setCellWidget(i, 4, btn);
    }

    blocker.unblock();

    // 渲染完表格数据后尝试从mRestoreSelectPid中找到上一次选择的project对象，并选择它
    if (restoreSelectRow != -1) {
        project_info_t info = list.at(restoreSelectRow);
        qDebug() << "Found need to select" <<
            "row:" << restoreSelectRow <<
            "pid:" << info.pid <<
            "name:" << info.name;
        twps->selectRow(restoreSelectRow);
        this->ui->radioButton_EnableFsw->setChecked(info.fsw);
    }
}

/**
 * @brief MainWindow::updateListWidgetDataFromUploadFilesInfo
 * 将mUploadFilesInfo数据渲染到ListWidget
 */
void MainWindow::updateListWidgetDataFromUploadFilesInfo()
{
    qDebug() << __func__;
    int restoreSelectRow = -1;
    // apply mUploadFilesInfo to UI
    QList<uf_info_t> &list = mUploadFilesInfo;
    for (int i = 0; i < list.size(); ++i) {
        uf_info_t info = list.at(i);
        qDebug() << "fid:" << info.fid <<
            "pid:" << info.pid <<
            "name:" << info.name <<
            "dir:" << info.dir <<
            "enable:" << info.enable;
    }
    QListWidget *lwufs = mListWidgetUploadFiles;
    // clean up list data
    lwufs->clear();
    // prepare watcher service
    FileWatcherService *mFWService = FileWatcherService::getInstance();
    mFWService->cleanup();

    for (int i = 0; i < list.size(); ++i) {
        uf_info_t info = list.at(i);
        if (mRestoreUploadFilesSelectPid == info.fid) restoreSelectRow = i;
        QListWidgetItem *item = new QListWidgetItem(info.name, lwufs);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(info.enable ? Qt::Checked : Qt::Unchecked);
        item->setToolTip(info.dir);

        QString filepath = info.dir + "/" + info.name;
        if (info.enable) {
            mFWService->addPath(filepath);
        }
    }

    if (restoreSelectRow != -1) {
        qDebug() << "Found need to select" <<
            "row:" << restoreSelectRow <<
            "fid:" << list.at(restoreSelectRow).fid <<
            "pid:" << list.at(restoreSelectRow).pid <<
            "name:" << list.at(restoreSelectRow).name;
        // lwufs->setCurrentItem(lwufs->item(restoreSelectRow));
    }

    // start file watcher listen
    if (mActiveProjectInfo.fsw) {
        mFWService->restart();
    }
}

bool MainWindow::updateWebFilesToUI()
{
    qDebug() << __func__;
    RefreshService *mRefreshService = RefreshService::getInstance();
    QList<file_t> list              = mRefreshService->getFileList();
    mListWidgetWebFiles->clear();

    for (int i = 0; i < list.size(); ++i) {
        file_t fd             = list.at(i);
        QIcon icon            = QIcon(fd.isFile ? ":/image/file.png" : ":/image/folder.png");
        QListWidgetItem *item = new QListWidgetItem(icon, fd.name);
        mListWidgetWebFiles->addItem(item);
    }

    return true;
}

void MainWindow::retranslate()
{
    updateTableWidgetDataFromProjectsInfo();
}

void MainWindow::setSelectProjectName(QString text)
{
    this->ui->label_SelectProjectName->setText(tr("Selected: ") + text);
}

void MainWindow::setSelectDownloadMode(int index)
{
    qDebug() << __func__ << index;
    mSettingsHelper->saveAppDownloadMode(index);
    if (index == 0) {
        setSelectProjectName(mSettingsHelper->getAppDownloadDir());
    } else if (index == 1) {
        project_info_t info = mActiveProjectInfo;
        QString ret = "N/A";
        if (info.pid.length() > 0) {
            if (info.name.length() > 0) {
                ret = info.name;
            }
        }
        setSelectProjectName(ret);
    } else {
        qWarning() << "Invalid index.";
    }

}

void MainWindow::notifyBubble(const QString &title, const QStringList &files)
{
    int count              = files.length();
    if (count == 0) {
        qDebug() << __func__ << "files.length() == 0.";
        return ;
    }

    if (isActiveWindow()) {
        qDebug() << __func__ << "Current Window is actived.";
        return ;
    }

    QFileInfoList fileinfos;
    for (int i = 0; i < files.length(); ++i) {
        fileinfos.append(QFileInfo(files.at(i)));
    }
    NotificationForm *form = new NotificationForm(title, fileinfos);
    form->showAtScreenCorner();
}

void MainWindow::pushRecorder(const QStringList &files, bool isDownlaod)
{
    RecorderService *mRecorder = RecorderService::getInstance();
    mRecorder->push(files, isDownlaod);
}

bool MainWindow::updateProjectInfoToUI()
{
    qDebug() << __func__;
    mProjectDbHelper->readAll(mPDBHCbReadAll);
    return true;
}

bool MainWindow::updateUploadFilesInfoToUI()
{
    qDebug() << __func__;
    if (mActiveProjectInfo.pid.length() > 0) {
        mUploadFileDbHelper->readAllByPid(mActiveProjectInfo.pid, mUFDBHCbReadAllByPid);
        return true;
    }
    qDebug() << __func__ << "Invalid mActiveProjectInfo";
    return false;
}

void MainWindow::setRestoreSelectPid(QString pid)
{
    qDebug() << __func__ << "pid:" << pid;
    mRestoreProjectsSelectPid = pid;
}

void MainWindow::setProgressBarDownload(int value, QString text)
{
    // Set from DownloadService
    QProgressBar *mProgressBar = this->ui->progressBar_Download;
    QLabel *label              = this->ui->label_DownloadFile;
    if (value <= 0) {
        if (mProgressBar->isVisible()) {
            mProgressBar->setVisible(false);
        }
        label->setText(text);
    } else {
        if (!mProgressBar->isVisible()) {
            mProgressBar->setVisible(true);
        }
        mProgressBar->setValue(value);
        label->setText(text);
    }
}

void MainWindow::setProgressBarUpload(int value, QString text)
{
    // Set from UploadService
    QProgressBar *mProgressBar = this->ui->progressBar_Upload;
    QLabel *label              = this->ui->label_UploadFile;
    if (value <= 0) {
        if (mProgressBar->isVisible()) {
            mProgressBar->setVisible(false);
        }
        label->setText(text);
        if (mWinTaskbarProgress && mWinTaskbarProgress->isVisible()) {
            mWinTaskbarProgress->setVisible(false);
        }
    } else {
        if (!mProgressBar->isVisible()) {
            mProgressBar->setVisible(true);
        }
        mProgressBar->setValue(value);
        label->setText(text);
        if (mWinTaskbarProgress) {
            if (!mWinTaskbarProgress->isVisible()) {
                mWinTaskbarProgress->setVisible(true);
            }
            mWinTaskbarProgress->setValue(value);
        }
    }
}

void MainWindow::setCountdownLabelText(QString text)
{
    // set from RefreshService
    this->ui->label_Countdown->setText(text);
}

void MainWindow::setLabelLoginUser(const QString &text)
{
    QString s = QString("Hi, %1").arg(text);
    this->ui->label_LoginUser->setText(s);
}

bool MainWindow::mSEventHandler(MSEvent *e)
{
    uint16_t event  = e->getMSEventType();
    QVariantMap map = e->getData().toMap();
    qDebug("MainWindow::mSEventHandler event:0x%04x", event);
    switch (event) {
    case MSEvent::EVENT_TYPE_REFRESH_IND: {

        uint16_t resultCode = map["resultCode"].toUInt();
        qDebug() << "resultCode:" << resultCode;

        if (resultCode == MSEvent::RESULT_CODE_SUCCESS) {
            RefreshService *mRefreshService = RefreshService::getInstance();
            QString webdir = mRefreshService->getPathByPathList(mRefreshService->getPathList());
            qDebug() << "Refresh Succeed.";
            this->ui->label_webPath->setText(webdir);
            updateWebFilesToUI();
            mRefreshService->recoverSelects();

        } else if (resultCode == MSEvent::RESULT_CODE_FAIL) {
            QString msg = map["msg"].toString();
            if (msg.length() > 0) {
                qDebug() << "msg:" << msg;
                QMessageBox::critical(this, tr("Reminder"), msg);
            } else {
                QMessageBox::critical(this, tr("Reminder"), "Refresh Failure.");
            }

        } else if (resultCode == MSEvent::RESULT_CODE_NETWORK_ONERROR) {
            QString msg = QString("%1;%2;%3")
                .arg(map["status"].toString(), map["errorString"].toString(), map["result"].toString());
            qDebug() << "msg:" << msg;
            QMessageBox::critical(this, tr("Reminder"), msg);

        } else {
            qWarning() << "Unknow Result Code." << resultCode;
        }

        break;
    }
    case MSEvent::EVENT_TYPE_REFRESH_CFM: {

        uint16_t resultCode = map["resultCode"].toUInt();
        qDebug() << "resultCode:" << resultCode;
        if (resultCode == MSEvent::RESULT_CODE_SUCCESS) {

        } else if (resultCode == MSEvent::RESULT_CODE_FAIL) {


        } else if (resultCode == MSEvent::RESULT_CODE_NETWORK_ONERROR) {


        } else {
            qWarning() << "Unknow Result Code." << resultCode;
        }
        break;
    }
    case MSEvent::EVENT_TYPE_DOWNLOAD_CFM: {
        uint16_t resultCode = map["resultCode"].toUInt();
        qDebug() << "resultCode:" << resultCode;
        if (resultCode == MSEvent::RESULT_CODE_SUCCESS) {
            QStringList okFiles    = map["okFiles"].toStringList();
            // int count              = okFiles.length();
            // QString title          = QString(tr("%1 file(s) downloaded")).arg(count);

            pushRecorder(okFiles, true);
            qDebug() << "Download Succeed.";

        } else if (resultCode == MSEvent::RESULT_CODE_FAIL) {
            QString errorString = map["errorString"].toString();
            if (errorString.length() > 0) {
                qDebug() << "errorString:" << errorString;
                QMessageBox::critical(this, tr("Reminder"), errorString);
            } else {
                QMessageBox::critical(this, tr("Reminder"), "Download Failure.");
            }

        } else if (resultCode == MSEvent::RESULT_CODE_NETWORK_ONERROR) {
            QString msg = QString("%1;%2;%3")
            .arg(map["status"].toString(), map["errorString"].toString(), map["result"].toString());
            qDebug() << "msg:" << msg;
            QMessageBox::critical(this, tr("Reminder"), msg);

        } else {
            qWarning() << "Unknow Result Code." << resultCode;
        }
        break;
    }
    case MSEvent::EVENT_TYPE_UPLOAD_CFM: {
        uint16_t resultCode = map["resultCode"].toUInt();
        qDebug() << "resultCode:" << resultCode;
        MainService *mMainService  = MainService::getInstance();
        if (resultCode == MSEvent::RESULT_CODE_SUCCESS) {
            QStringList okFiles    = map["okFiles"].toStringList();
            int count              = okFiles.length();
            QString title          = QString(tr("%1 file(s) uploaded")).arg(count);

            pushRecorder(okFiles, false);
            notifyBubble(title, okFiles);
            qDebug() << "Upload Succeed.";
            // After uploading, you need to refresh the file list.
            MSEvent *event = new MSEvent(this, MSEvent::EVENT_TYPE_REFRESH_REQ);
            QCoreApplication::postEvent(mMainService, event);

        } else if (resultCode == MSEvent::RESULT_CODE_FAIL) {
            QString errorString = map["errorString"].toString();
            if (errorString.length() > 0) {
                qDebug() << "errorString:" << errorString;
                QMessageBox::critical(this, tr("Reminder"), errorString);
            } else {
                QMessageBox::critical(this, tr("Reminder"), "Download Failure.");
            }

        } else if (resultCode == MSEvent::RESULT_CODE_NETWORK_ONERROR) {
            QString msg = QString("%1;%2;%3")
            .arg(map["status"].toString(), map["errorString"].toString(), map["result"].toString());
            qDebug() << "msg:" << msg;
            QMessageBox::critical(this, tr("Reminder"), msg);

        } else {
            qWarning() << "Unknow Result Code." << resultCode;
        }
        break;
    }
    case MSEvent::EVENT_TYPE_DELETE_CFM:
    case MSEvent::EVENT_TYPE_RENAME_CFM:
    case MSEvent::EVENT_TYPE_CUT_CFM:
    case MSEvent::EVENT_TYPE_COPY_CFM:
    case MSEvent::EVENT_TYPE_PASTE_CFM:
    case MSEvent::EVENT_TYPE_MKDIR_CFM: {

        uint16_t resultCode = map["resultCode"].toUInt();
        qDebug() << "resultCode:" << resultCode;
        MainService *mMainService  = MainService::getInstance();
        if (resultCode == MSEvent::RESULT_CODE_SUCCESS) {

            // After other operation, you need to refresh the file list.
            MSEvent *event = new MSEvent(this, MSEvent::EVENT_TYPE_REFRESH_REQ);
            QCoreApplication::postEvent(mMainService, event);

        } else if (resultCode == MSEvent::RESULT_CODE_FAIL) {
            QString errorString = map["errorString"].toString();
            if (errorString.length() > 0) {
                qDebug() << "errorString:" << errorString;
                QMessageBox::critical(this, tr("Reminder"), errorString);
            } else {
                QMessageBox::critical(this, tr("Reminder"), "Other Operation Failure.");
            }

        } else if (resultCode == MSEvent::RESULT_CODE_NETWORK_ONERROR) {
            QString msg = QString("%1;%2;%3")
            .arg(map["status"].toString(), map["errorString"].toString(), map["result"].toString());
            qDebug() << "msg:" << msg;
            QMessageBox::critical(this, tr("Reminder"), msg);

        } else {
            qWarning() << "Unknow Result Code." << resultCode;
        }

        break;
    }
    case MSEvent::EVENT_TYPE_FILE_CHANGED_IND: {
        MainService *mMainService       = MainService::getInstance();
        RefreshService *mRefreshService = RefreshService::getInstance();
        QStringList files               = map["files"].toStringList();
        project_info_t pinfo            = mActiveProjectInfo;
        QString webdir                  = mRefreshService->getPathByPathList(mRefreshService->getPathList());
        if (files.isEmpty()) {
            break;
        }
        qDebug() << "onFileChangedInd:" << files;
        MSEvent *event                  = new MSEvent(this, MSEvent::EVENT_TYPE_UPLOAD_REQ);
        map["files"]                    = files;
        map["dir"]                      = pinfo.wdir;
        map["webdir"]                   = webdir;
        map["script"]                   = pinfo.rus ? pinfo.uspath : "";
        event->setData(map);
        QCoreApplication::postEvent(mMainService, event);
        break;
    }
    default:
        break;
    }
    return true;
}

bool MainWindow::event(QEvent *e)
{
    uint16_t eventType = 0x0000;

    if (e->type() == MSEvent::MSEVENT_BASE_TYPE) {
        MSEvent *event = static_cast<MSEvent *>(e);
        mSEventHandler(event);
    } else if (e->type() == QEvent::Show) {
        static bool initialized = false;
        if (initialized == false) {
            initialized             = true;
            QWindow *win            = windowHandle();
            qDebug() << "MainWindow::event QEvent::Show windowHandle:" << win;
            if (win) {
                mWinTaskbarButton->setWindow(win);
                mWinTaskbarProgress = mWinTaskbarButton->progress();
                initThumbnailToolBar();
            }
        }
    } else if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    return QMainWindow::event(e);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // If the minimize-to-tray function is enabled, the window will be hidden instead of being closed.
    MainService::getInstance()->closeEvent(this, event);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QMainWindow::changeEvent(event);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    MainService *mMainService = MainService::getInstance();

    // 拖拽文件上传功能
    if (watched == this->ui->listWidget_WebFiles) {
        if (event->type() == QEvent::DragEnter) {
            QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent *>(event);
            if (dragEvent->mimeData()->hasUrls()) {
                dragEvent->acceptProposedAction();
                return true;
            }

        } else if (event->type() == QEvent::Drop) {
            QStringList files               ;
            RefreshService *mRefreshService = RefreshService::getInstance();
            QDropEvent *dropEvent           = static_cast<QDropEvent *>(event);
            const QList<QUrl> urls          = dropEvent->mimeData()->urls();
            project_info_t pinfo            = mActiveProjectInfo;
            QString webdir                  = mRefreshService->getPathByPathList(mRefreshService->getPathList());

            for (const QUrl &url : urls) {
                if (url.isLocalFile()) {
                    files.append(url.toLocalFile());
                }
            }

            if (!files.isEmpty()) {
                QVariantMap map;
                MSEvent *event = new MSEvent(this, MSEvent::EVENT_TYPE_UPLOAD_REQ);
                map["files"]   = files;
                map["dir"]     = pinfo.wdir;
                map["webdir"]  = webdir;
                map["script"]  = "";
                event->setData(QVariant(map));
                QCoreApplication::postEvent(mMainService, event);
            }

            dropEvent->acceptProposedAction();
            return true;

        }

    // 拖拽文件添加至Upload Files列表
    } else if (watched == this->ui->listWidget_UploadFiles) {
        if (event->type() == QEvent::DragEnter) {
            QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent *>(event);
            if (dragEvent->mimeData()->hasUrls()) {
                dragEvent->acceptProposedAction();
                return true;
            }

        } else if (event->type() == QEvent::Drop) {
            QStringList files;
            QDropEvent *dropEvent = static_cast<QDropEvent *>(event);
            const QList<QUrl> urls = dropEvent->mimeData()->urls();

            for (const QUrl &url : urls) {
                if (url.isLocalFile()) {
                    files.append(url.toLocalFile());
                }
            }

            if (!files.isEmpty() && !mActiveProjectInfo.pid.isEmpty()) {
                // get every file info
                int okcount = 0;
                for (int i  = 0; i < files.size(); ++i) {
                    QFileInfo fileInfo(files.at(i));
                    uf_info_t info;
                    info.fid    = mFilTools->getCurrentDateTimeToString("yyyyMMddHHmmsszzz") + QString("%1").arg(i);
                    info.pid    = mActiveProjectInfo.pid;
                    info.name   = fileInfo.fileName();
                    info.dir    = fileInfo.path();
                    info.enable = false;
                    if (!mUploadFileDbHelper->add(info.fid, info.pid, info.name, info.dir, info.enable)) {
                        qWarning() << "Add Upload Files Failure.";
                    }
                }

                // update UI data
                updateUploadFilesInfoToUI();
            }
        }

    }

    return QMainWindow::eventFilter(watched, event);
}
