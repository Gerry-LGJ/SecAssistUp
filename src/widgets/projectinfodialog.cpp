#include "projectinfodialog.h"
#include "ui_projectinfodialog.h"

#include <QFileDialog>

#include "../window/mainwindow.h"

ProjectInfoDialog::ProjectInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProjectInfoDialog)
{
    ui->setupUi(this);
    mSettingsHelper     = SettingsHelper::getInstance();
    mFilTools           = FilTools::getInstance();
    mProjectDbHelper    = ProjectDbHelper::getInstance();
}

ProjectInfoDialog::~ProjectInfoDialog()
{
    delete ui;
}

void ProjectInfoDialog::init()
{
    setModal(true);
    mReadRecordByPidCallable = new ProjectDbHelperCallable(this);
    connect(mReadRecordByPidCallable, &ProjectDbHelperCallable::readRecordByPid, this, [=] (
                                                                     const QString pid,
                                                                     const QString name,
                                                                     const QString wdir,
                                                                     const QString dspath,
                                                                     const bool rds,
                                                                     const QString uspath,
                                                                     const bool rus,
                                                                     const bool fsw,
                                                                     const QString lstime) {
        qDebug() << "ProjectInfoDialog::readRecordByPid pid:" << pid;
        mPid = pid;
        this->ui->lineEdit_Name->setText(name);
        this->ui->lineEdit_Wdir->setText(wdir);
        this->ui->lineEdit_dspath->setText(dspath);
        this->ui->checkBox_rds->setChecked(rds);
        this->ui->lineEdit_uspath->setText(uspath);
        this->ui->checkBox_rus->setChecked(rus);
        this->ui->checkBox_fsw->setChecked(fsw);
    });
    initPushButton();
}

void ProjectInfoDialog::openWithPid(QString pid)
{
    qDebug() << "ProjectInfoDialog::open pid:" << pid;
    mPid = pid;
    getInfoForUI(pid);
    this->show();
}

bool ProjectInfoDialog::event(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    return QDialog::event(e);
}

void ProjectInfoDialog::initPushButton()
{
    // Button Box
    connect(this->ui->buttonBox, &QDialogButtonBox::accepted, this, [=] {
        MainWindow *mMainWindow = MainWindow::getInstance();
        QTableWidget *tbwdg = mMainWindow->getTableWidgetProjects();
        project_info_t info = getInfoFromUI();
        if (mProjectDbHelper->modify(mPid,
                                 info.name,
                                 info.wdir,
                                 info.dspath,
                                 info.rds,
                                 info.uspath,
                                 info.rus,
                                     info.fsw, "")) {
            mMainWindow->setRestoreSelectPid(mPid);
            mMainWindow->updateProjectInfoToUI();
        } else {
            qWarning() << "Modify Project Info Failure.";
        }


    });
    connect(this->ui->buttonBox, &QDialogButtonBox::rejected, this, [=] {
        qDebug() << "User Cancel Modify Project Info.";
    });
    // Work Directory
    connect(this->ui->pushButton_Wdir, &QPushButton::clicked, this, [=] {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Open Folder"));
        if (dir.length() > 0) {
            this->ui->lineEdit_Wdir->setText(dir);
        }
    });
    // Run Downlaod Script
    connect(this->ui->pushButton_rds, &QPushButton::clicked, this, [=] {
        QString file = QFileDialog::getOpenFileName(this, tr("Open File"));
        if (file.length() > 0) {
            this->ui->lineEdit_dspath->setText(file);
        }
    });
    // Run Upload Script
    connect(this->ui->pushButton_rus, &QPushButton::clicked, this, [=] {
        QString file = QFileDialog::getOpenFileName(this, tr("Open File"));
        if (file.length() > 0) {
            this->ui->lineEdit_uspath->setText(file);
        }
    });
}

project_info_t ProjectInfoDialog::getInfoFromUI()
{
    project_info_t info;
    info.name = this->ui->lineEdit_Name->text();
    info.wdir = this->ui->lineEdit_Wdir->text();
    info.dspath = this->ui->lineEdit_dspath->text();
    info.rds    = this->ui->checkBox_rds->isChecked();
    info.uspath = this->ui->lineEdit_uspath->text();
    info.rus    = this->ui->checkBox_rus->isChecked();
    info.fsw    = this->ui->checkBox_fsw->isChecked();
    return info;
}

void ProjectInfoDialog::getInfoForUI(QString pid)
{
    qDebug() << __func__ << "pid:" << pid;
    mProjectDbHelper->readRecordByPid(pid, mReadRecordByPidCallable);
}
