#ifndef PROJECTDBHELPER_H
#define PROJECTDBHELPER_H

#include <QObject>
#include <QSqlDatabase>

// #include "sqlite3/sqlite3.h"
#include "../common/singleton.h"

namespace ProjectDbHelperColumnIndexType {
    Q_NAMESPACE
    enum ColumnIndexType {
        CIT_PID = 0,        /* pid */
        CIT_NAME,           /* name */
        CIT_WDIR,           /* wdir */
        CIT_DSPATH,         /* dspath */
        CIT_RDS,            /* rds */
        CIT_USPATH,         /* uspath */
        CIT_RUS,            /* rus */
        CIT_FSW,            /* fsw */
        CIT_LSTIME,         /* lstime */
        CIT_MAX             /* max */
    };
    Q_ENUM_NS(ColumnIndexType)
}

using namespace ProjectDbHelperColumnIndexType;
class ProjectDbHelperCallable : public QObject
{
    Q_OBJECT
public:
    explicit ProjectDbHelperCallable(QObject *parent = nullptr);

    Q_SIGNAL void start();
    Q_SIGNAL void finish();
    Q_SIGNAL void error(QString errorString);
    Q_SIGNAL void readAll(
        const QString pid,
        const QString name,
        const QString wdir,
        const QString dspath,
        const bool rds,
        const QString uspath,
        const bool rus,
        const bool fsw,
        const QString lstime);
    Q_SIGNAL void search(
        const QString pid,
        const QString name,
        const QString wdir,
        const QString dspath,
        const bool rds,
        const QString uspath,
        const bool rus,
        const bool fsw,
        const QString lstime);
    Q_SIGNAL void readRecordByPid(
        const QString pid,
        const QString name,
        const QString wdir,
        const QString dspath,
        const bool rds,
        const QString uspath,
        const bool rus,
        const bool fsw,
        const QString lstime);
};

class ProjectDbHelper : public QObject
{
    Q_OBJECT
private:
    explicit ProjectDbHelper(QObject *parent = nullptr);

public:
    static inline Q_INVOKABLE QString getColumnDisplayName(ColumnIndexType type) {
        char name[ColumnIndexType::CIT_MAX][64] = {
            "pid", "name", "wdir", "dspath", "rds", "uspath", "rus", "fsw", "lstime"
        };
        return QString(name[type]);
    }

    SINGLETON(ProjectDbHelper);
    // ~ProjectDbHelper() override;
    void initialize();
    QSqlDatabase getSqlDatabase() { return db; };

    Q_INVOKABLE QString getPidByName(const QString &name);
    Q_INVOKABLE bool add(
        const QString &pid,
        const QString &name,
        const QString &wdir,
        const QString &dspath,
        const bool &rds,
        const QString &uspath,
        const bool &rus,
        const bool &fsw,
        const QString &lstime);
    Q_INVOKABLE bool del(const QString &pid);
    Q_INVOKABLE bool modify(
        const QString &pid,
        const QString &name,
        const QString &wdir,
        const QString &dspath,
        const bool &rds,
        const QString &uspath,
        const bool &rus,
        const bool &fsw,
        const QString &lstime);
    Q_INVOKABLE void readAll(ProjectDbHelperCallable *cb);
    Q_INVOKABLE void search(const int columnIndexType, const QString &text, ProjectDbHelperCallable *cb);
    Q_INVOKABLE void readRecordByPid(const QString &pid, ProjectDbHelperCallable *cb);
    Q_INVOKABLE QString readRecordByPid(const QString &pid, int cit);
    Q_INVOKABLE bool updateRecordByPid(const QString &pid, int cit, const QString &text);

    // runSystemScriptFile
    Q_INVOKABLE int runDownloadScript(const QString &pid);
    Q_INVOKABLE int runUploadScript(const QString &pid, int msecs = 30000);

signals:

private:
    QString defDbFilePath;            /* 默认数据库文件路径 */
    QString defDbTableName;           /* 默认数据库中的表名 */
    QSqlDatabase db;                  /* 数据库实例 */

};

#endif // PROJECTDBHELPER_H
