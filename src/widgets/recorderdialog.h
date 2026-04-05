#ifndef RECORDERDIALOG_H
#define RECORDERDIALOG_H

#include <QDialog>
#include <QTableWidget>

#include "../service/recorderservice.h"

namespace Ui {
class RecorderDialog;
}

class RecorderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecorderDialog(QWidget *parent = nullptr);
    ~RecorderDialog();

    void init();
    void reloadRecord();

public slots:
    void onAddNew();


private:
    Ui::RecorderDialog *ui;
    RecorderService    *mRecorder;
    QTableWidget       *mTableWidgetRecords;

    // func

};

#endif // RECORDERDIALOG_H
