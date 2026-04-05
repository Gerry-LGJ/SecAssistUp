#ifndef RECORDERSERVICE_H
#define RECORDERSERVICE_H

#include <QObject>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>

#include "../helper/settingshelper.h"
#include "../helper/filtools.h"
#include "../common/singleton.h"


typedef struct {
    QDateTime datetime;
    QFileInfo info;
    bool      isDownload;
    QString   md5;
} record_info_t;

class RecorderService : public QObject
{
    Q_OBJECT
public:
    SINGLETON(RecorderService)
    void init();
    void push(QStringList   files, bool isDownload);
    void push(QList<QFile>  files, bool isDownload);
    void push(QFileInfoList infos, bool isDownload);
    QList<record_info_t> get() { return mRecords; }

    Q_SIGNAL void onAddNew();


private:
    QList<record_info_t> mRecords;
    int                  mCount;
    int                  mMaxSize;

    // inst
    SettingsHelper      *mSettings;
    FilTools            *mTools;

    // func
    explicit RecorderService(QObject *parent = nullptr);

signals:
};

#endif // RECORDERSERVICE_H
