#include "recorderservice.h"

RecorderService::RecorderService(QObject *parent)
    : QObject{parent}
{
    mTools = FilTools::getInstance();
}

void RecorderService::init()
{
    mMaxSize = 50;
    mRecords.clear();
}

void RecorderService::push(QStringList files, bool isDownload)
{
    if (files.isEmpty()) {
        qDebug() << __func__ << "invalid files";
        return ;
    }

    QFileInfoList infoslist;

    for (int i = 0; i < files.size(); ++i) {
        infoslist.append(QFileInfo(files.at(i)));
    }

    push(infoslist, isDownload);
}

void RecorderService::push(QList<QFile> files, bool isDownload)
{
    if (files.isEmpty()) {
        qDebug() << __func__ << "invalid files";
        return ;
    }

    QFileInfoList infoslist;

    for (int i = 0; i < files.size(); ++i) {
        infoslist.append(QFileInfo(files.at(i)));
    }

    push(infoslist, isDownload);
}

void RecorderService::push(QFileInfoList infos, bool isDownload)
{
    if (infos.isEmpty()) {
        qDebug() << __func__ << "invalid infos";
        return ;
    }

    int infosCount = infos.size();

    if (infosCount >= mMaxSize) {
        mRecords.clear();
        int startIndex = infosCount - mMaxSize;
        for (int i = startIndex; i < infosCount; ++i) {
            record_info_t ri;
            ri.datetime   = QDateTime::currentDateTime();
            ri.isDownload = isDownload;
            ri.info       = infos.at(i);
            ri.md5        = mTools->md5CalculateFile(infos.at(i).absoluteFilePath());
            mRecords.append(ri);
        }

    } else {
        int keepCount = mMaxSize - infosCount;

        if (mRecords.size() > keepCount) {
            mRecords.erase(mRecords.begin() + keepCount, mRecords.end());
        }

        for (int i = infosCount - 1; i >=0; --i) {
            record_info_t ri;
            ri.datetime   = QDateTime::currentDateTime();
            ri.isDownload = isDownload;
            ri.info       = infos.at(i);
            ri.md5        = mTools->md5CalculateFile(infos.at(i).absoluteFilePath());
            mRecords.prepend(ri);
        }
    }
    emit onAddNew();
}
