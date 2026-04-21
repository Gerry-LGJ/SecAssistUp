#include "recorderdialog.h"
#include "ui_recorderdialog.h"

RecorderDialog::RecorderDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RecorderDialog)
{
    ui->setupUi(this);
    mTableWidgetRecords = this->ui->tableWidget_Record;
    mRecorder = RecorderService::getInstance();
    init();
}

RecorderDialog::~RecorderDialog()
{
    delete ui;
}

void RecorderDialog::init()
{
    connect(mRecorder, &RecorderService::onAddNew, this, &RecorderDialog::onAddNew);
    reloadRecord();
}

void RecorderDialog::reloadRecord()
{
    QTableWidget *twrs = mTableWidgetRecords;
    QList<record_info_t> list = mRecorder->get();

    // 清理表格数据
    twrs->clear();
    // 设置表头
    QStringList headers;
    headers << tr("time") << tr("filename") << "md5" << tr("location") << tr("direction");
    twrs->setRowCount(list.size());
    twrs->setColumnCount(5);
    twrs->setHorizontalHeaderLabels(headers);
    QHeaderView *horizontalHeader = twrs->horizontalHeader();
    horizontalHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    // 开始渲染数据
    for (int i = 0; i < list.size(); ++i) {
        record_info_t info = list.at(i);
        QTableWidgetItem *fpItem = new QTableWidgetItem(info.info.filePath());
        fpItem->setToolTip(info.info.filePath());

        twrs->setItem(i, 0, new QTableWidgetItem(info.datetime.toString("yyyy-MM-dd HH:mm:ss")));
        twrs->setItem(i, 1, new QTableWidgetItem(info.info.fileName()));
        twrs->setItem(i, 2, new QTableWidgetItem(info.md5));
        twrs->setItem(i, 3, fpItem);
        twrs->setItem(i, 4, new QTableWidgetItem(info.isDownload ? tr("Download") : tr("Upload")));
    }
}

void RecorderDialog::onAddNew()
{
    reloadRecord();
}
