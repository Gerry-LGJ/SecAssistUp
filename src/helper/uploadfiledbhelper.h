#ifndef UPLOADFILEDBHELPER_H
#define UPLOADFILEDBHELPER_H

#include <QObject>
#include <QSqlDatabase>

// #include "sqlite3/sqlite3.h"
#include "../common/singleton.h"

namespace UploadFileDbHelperColumnIndexType {
    Q_NAMESPACE
    enum UfColumnIndexType {
        CIT_FID = 0,        /* fid */
        CIT_PID,            /* pid */
        CIT_NAME,           /* name */
        CIT_DIR,            /* dir */
        CIT_ENABLE,         /* enable */
        CIT_MAX             /* max */
    };
    Q_ENUM_NS(UfColumnIndexType)
}

class UploadFileDbHelperCallable : public QObject
{
    Q_OBJECT
public:
    explicit UploadFileDbHelperCallable(QObject *parent = nullptr);

    Q_SIGNAL void start();
    Q_SIGNAL void finish();
    Q_SIGNAL void error(QString errorString);
    Q_SIGNAL void readAllByPid(
        const QString fid,
        const QString pid,
        const QString name,
        const QString dir,
        const bool    enable);
    Q_SIGNAL void readRecordByFid(
        const QString fid,
        const QString pid,
        const QString name,
        const QString dir,
        const bool    enable);
};

using namespace UploadFileDbHelperColumnIndexType;

class UploadFileDbHelper : public QObject
{
    Q_OBJECT
private:
    explicit UploadFileDbHelper(QObject *parent = nullptr);

public:
    static inline Q_INVOKABLE QString getColumnDisplayName(UfColumnIndexType type) {
        char name[UfColumnIndexType::CIT_MAX][64] = {
            "fid", "pid", "name", "dir", "enable"
        };
        return QString(name[type]);
    }

    SINGLETON(UploadFileDbHelper)
    void initialize(void);

    // operater
    Q_INVOKABLE bool add(
        const QString &fid,
        const QString &pid,
        const QString &name,
        const QString &dir,
        const bool    &enable);
    Q_INVOKABLE bool del(const QString &fid);
    Q_INVOKABLE bool modify(
        const QString &fid,
        const QString &pid,
        const QString &name,
        const QString &dir,
        const bool    &enable);
    Q_INVOKABLE void readAllByPid(const QString &pid, UploadFileDbHelperCallable *cb);
    Q_INVOKABLE void readRecordByFid(const QString &fid, UploadFileDbHelperCallable *cb);
    Q_INVOKABLE QString readRecordByFid(const QString &fid, int cit);
    Q_INVOKABLE bool updateRecordByFid(const QString &fid, int cit, const QString &text);

private:
    QString defDbFilePath;            /* 默认数据库文件路径 */
    QString defDbTableName;           /* 默认数据库中的表名 */
    QSqlDatabase db;                  /* 数据库实例 */
};

#endif // UPLOADFILEDBHELPER_H
