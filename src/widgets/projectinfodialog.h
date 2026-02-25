#ifndef PROJECTINFODIALOG_H
#define PROJECTINFODIALOG_H

#include <QDialog>

#include "../common/singleton.h"
#include "../common/filedata_t.h"
#include "../helper/settingshelper.h"
#include "../helper/projectdbhelper.h"
#include "../helper/filtools.h"

namespace Ui {
class ProjectInfoDialog;
}

class ProjectInfoDialog : public QDialog
{
    Q_OBJECT

private:
    explicit ProjectInfoDialog(QWidget *parent = nullptr);
    ~ProjectInfoDialog();

public:
    SINGLETON(ProjectInfoDialog)
    void init();
    void openWithPid(QString pid);

protected:
    bool event(QEvent *e) override;

private:
    Ui::ProjectInfoDialog *ui;
    SettingsHelper     *mSettingsHelper;
    FilTools           *mFilTools;
    ProjectDbHelper    *mProjectDbHelper;
    ProjectDbHelperCallable *mReadRecordByPidCallable;

    // Project Info
    QString             mPid;

    project_info_t getInfoFromUI();
    void           getInfoForUI(QString pid);
    void           initPushButton(void);
};

#endif // PROJECTINFODIALOG_H
