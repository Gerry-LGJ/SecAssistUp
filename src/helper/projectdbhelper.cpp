#include "projectdbhelper.h"

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QThreadPool>
#include <QEventLoop>
#include <QStandardPaths>
#include <QPointer>

ProjectDbHelperCallable::ProjectDbHelperCallable(QObject *parent) : QObject{parent}
{ }

ProjectDbHelper::ProjectDbHelper(QObject *parent)
    : QObject{parent}
{
    this->db = QSqlDatabase::addDatabase("QSQLITE");
    this->defDbFilePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/user.db";
    this->db.setDatabaseName(this->defDbFilePath);
    this->defDbTableName = "project";
    qDebug() << "database:" << this->defDbFilePath;
    initialize();
}

void ProjectDbHelper::initialize()
{
    qDebug() << "ProjectDbHelper::initialize";
    if (this->db.open()) {
        qDebug() << "Project Database opened successfully!";
    } else {
        qWarning() << "Project Database opened failure:" << this->db.lastError().text();
        return ;
    }
    QSqlQuery query(this->db);
    query.prepare("create table if not exists project ("
                                  "pid TEXT primary key, "
                                  "name TEXT unique not null, "
                                  "wdir TEXT, "
                                  "dspath TEXT, "
                                  "rds INTEGER, "
                                  "uspath TEXT, "
                                  "rus INTEGER, "
                                  "fsw INTEGER, "
                                  "lstime TEXT);");
    if (!query.exec()) {
        qWarning() << "create query.exec failure, reason:" << query.lastError().text();
    }
}

QString ProjectDbHelper::getPidByName(const QString &name)
{
    QString ret;
    QSqlQuery query(this->db);
    query.prepare("select * from project where name=:name;");
    query.bindValue(":name", name);
    if (!query.exec()) {
        qWarning() << __func__ << "query.exec failure, reason:" << query.lastError().text();
        return "";
    }
    while (query.next()) {
        ret = query.value(getColumnDisplayName(ProjectDbHelperColumnIndexType::CIT_PID)).toString();
    }
    qDebug() << __func__ << "ret:" << ret;
    return ret;
}

bool ProjectDbHelper::add(
    const QString &pid,
    const QString &name,
    const QString &wdir,
    const QString &dspath,
    const bool &rds,
    const QString &uspath,
    const bool &rus,
    const bool &fsw,
    const QString &lstime)
{
    QSqlQuery query(this->db);
    query.prepare("insert into project (pid, name, wdir, dspath, rds, uspath, rus, fsw, lstime) "
                               "values (:pid, :name, :wdir, :dspath, :rds, :uspath, :rus, :fsw, :lstime);");
    query.bindValue(":pid", pid);
    query.bindValue(":name", name);
    query.bindValue(":wdir", wdir);
    query.bindValue(":dspath", dspath);
    query.bindValue(":rds", rds);
    query.bindValue(":uspath", uspath);
    query.bindValue(":rus", rus);
    query.bindValue(":fsw", fsw);
    query.bindValue(":lstime", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    if (!query.exec()) {
        qWarning() << "add query.exec failure, reason:" << query.lastError().text();
        return false;
    }
    return true;
}

bool ProjectDbHelper::del(const QString &pid)
{
    qDebug() << "del pid:" << pid;
    QSqlQuery query(this->db);
    query.prepare("delete from project where pid=:pid;");
    query.bindValue(":pid", pid);
    if (!query.exec()) {
        qWarning() << "del query.exec failure, reason:" << query.lastError().text();
        return false;
    }
    return true;
}

bool ProjectDbHelper::modify(
    const QString &pid,
    const QString &name,
    const QString &wdir,
    const QString &dspath,
    const bool &rds,
    const QString &uspath,
    const bool &rus,
    const bool &fsw,
    const QString &lstime)
{
    QSqlQuery query(this->db);
    query.prepare(QString("UPDATE %1 SET "
                          "name = :name, "
                          "wdir = :wdir, "
                          "dspath = :dspath, "
                          "rds = :rds, "
                          "uspath = :uspath, "
                          "rus = :rus, "
                          "fsw = :fsw, "
                          "lstime = :lstime "
                          "WHERE pid = :pid;")
                      .arg(this->defDbTableName));
    query.bindValue(":pid", pid);
    query.bindValue(":name", name);
    query.bindValue(":wdir", wdir);
    query.bindValue(":dspath", dspath);
    query.bindValue(":rds", rds ? 1 : 0);
    query.bindValue(":uspath", uspath);
    query.bindValue(":rus", rus ? 1 : 0);
    query.bindValue(":fsw", fsw ? 1 : 0);
    query.bindValue(":lstime", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    if (!query.exec()) {
        qWarning() << "modify query.exec failure, reason:" << query.lastError().text();
        return false;
    }
    return true;
}

void ProjectDbHelper::readAll(ProjectDbHelperCallable *cb)
{
    QPointer<ProjectDbHelperCallable> callable(cb);
    QThreadPool::globalInstance()->start([=]() {
        if (!callable.isNull()) {
            callable->start();
        }
        QSqlQuery query(this->db);
        query.prepare("select * from project;");
        if (!query.exec()) {
            callable->error(query.lastError().text());
            callable->finish();
            return ;
        }
        while (query.next()) {
            QString pid             = query.value("pid").toString();
            QString name            = query.value("name").toString();
            QString wdir            = query.value("wdir").toString();
            QString dspath          = query.value("dspath").toString();
            bool rds                = query.value("rds").toBool();
            QString uspath          = query.value("uspath").toString();
            bool rus                = query.value("rus").toBool();
            bool fsw                = query.value("fsw").toBool();
            QString lstime          = query.value("lstime").toString();
            if (!callable.isNull()) {
                callable->readAll(pid, name, wdir, dspath, rds, uspath, rus, fsw, lstime);
            }
        }
        if (!callable.isNull()) {
            callable->finish();
        }
    });
}

void ProjectDbHelper::search(const int columnIndexType, const QString &text, ProjectDbHelperCallable *cb)
{
    QPointer<ProjectDbHelperCallable> callable(cb);
    QThreadPool::globalInstance()->start([=]() {
        if (!callable.isNull()) {
            callable->start();
        }
        QString key = getColumnDisplayName((ColumnIndexType)columnIndexType);
        /* 给每个字符前后都添加通配符 % */
        QString textExtended = "%";
        for (int i = 0; i < text.length(); i++) {
            textExtended += QString(text.at(i)) + "%";
        }
        /* 生成sql查询语句 */
        QSqlQuery query(this->db);
        query.prepare("select * from project where :key like \":text\";");
        query.bindValue(":key", key);
        query.bindValue(":text", textExtended);
        if (!query.exec()) {
            callable->error(query.lastError().text());
            callable->finish();
            return ;
        }
        while (query.next()) {
            QString pid             = query.value("pid").toString();
            QString name            = query.value("name").toString();
            QString wdir            = query.value("wdir").toString();
            QString dspath          = query.value("dspath").toString();
            bool rds                = query.value("rds").toBool();
            QString uspath          = query.value("uspath").toString();
            bool rus                = query.value("rus").toBool();
            bool fsw                = query.value("fsw").toBool();
            QString lstime          = query.value("lstime").toString();
            if (!callable.isNull()) {
                callable->search(pid, name, wdir, dspath, rds, uspath, rus, fsw, lstime);
            }
        }
        if (!callable.isNull()) {
            callable->finish();
        }
    });
}

void ProjectDbHelper::readRecordByPid(const QString &pid, ProjectDbHelperCallable *cb)
{
    QPointer<ProjectDbHelperCallable> callable(cb);
    QThreadPool::globalInstance()->start([=]() {
        if (!callable.isNull()) {
            callable->start();
        }
        QSqlQuery query(this->db);
        query.prepare("select * from project where pid=:pid;");
        query.bindValue(":pid", pid);
        if (!query.exec()) {
            callable->error(query.lastError().text());
            callable->finish();
            return ;
        }
        while (query.next()) {
            QString pid             = query.value("pid").toString();
            QString name            = query.value("name").toString();
            QString wdir            = query.value("wdir").toString();
            QString dspath          = query.value("dspath").toString();
            bool rds                = query.value("rds").toBool();
            QString uspath          = query.value("uspath").toString();
            bool rus                = query.value("rus").toBool();
            bool fsw                = query.value("fsw").toBool();
            QString lstime          = query.value("lstime").toString();
            if (!callable.isNull()) {
                callable->readRecordByPid(pid, name, wdir, dspath, rds, uspath, rus, fsw, lstime);
            }
        }
        if (!callable.isNull()) {
            callable->finish();
        }
    });
}

QString ProjectDbHelper::readRecordByPid(const QString &pid, int cit)
{
    ColumnIndexType _cit = (ColumnIndexType)cit;
    if (cit < 0 || CIT_MAX <= cit) {
        qWarning() << __func__ << "cit over range:" << cit;
        return "";
    }
    QString ret;
    QSqlQuery query(this->db);
    query.prepare("select * from project where pid=:pid;");
    query.bindValue(":pid", pid);
    if (!query.exec()) {
        qWarning() << __func__ << "query.exec failure, reason:" << query.lastError().text();
        return "";
    }
    while (query.next()) {
        ret = query.value(getColumnDisplayName(_cit)).toString();
    }
    qDebug() << __func__ << "ret: " << ret;
    return ret;
}

bool ProjectDbHelper::updateRecordByPid(const QString &pid, int cit, const QString &text)
{
    ColumnIndexType _cit = (ColumnIndexType)cit;
    if (cit < 0 || CIT_MAX <= cit) {
        qWarning() << __func__ << "cit over range:" << cit;
        return false;
    }
    QSqlQuery query(this->db);
    QString citStr = getColumnDisplayName(_cit);
    // qDebug() << __func__ << "pid:" << pid << "cit:" << cit << "text:" << text << "citStr:" << citStr;
    query.prepare("UPDATE project SET " + citStr + " = :" + citStr + " WHERE pid=:pid");
    query.bindValue(":" + citStr, text);
    query.bindValue(":pid", pid);
    if (!query.exec()) {
        qWarning() << __func__ << "query.exec failure, reason:" << query.lastError().text();
        return false;
    }
    return true;
}

int ProjectDbHelper::runDownloadScript(const QString &pid)
{
    qDebug() << __func__ << "pid:" << pid;
    QString fileStr = readRecordByPid(pid, CIT_DSPATH);
    if (fileStr.length() <= 0) {
        qDebug() << __func__ << "Not found script file path";
        return -1;
    }
    QString wdirStr = readRecordByPid(pid, CIT_WDIR);
    if (wdirStr.length() <= 0) {
        qDebug() << __func__ << "Not found script work dir";
        return -2;
    }
    QFileInfo fileinfo(fileStr);
    QDir dir(wdirStr);
    if (!fileinfo.exists() || !dir.exists()) {
        qWarning() << "scriptPath or scriptWorkDir not exists, stop execute script";
        return -3;
    }
#if defined(Q_OS_WIN)
    static QString cmdStr;
    cmdStr = "start \"" + fileinfo.fileName() + "\" /D " + wdirStr.replace("/", "\\") + " " + fileStr.replace("/", "\\");
    qDebug() << __func__ << "cmdStr:" << cmdStr;
    // system(cmdStr.toUtf8().data());
    QThreadPool::globalInstance()->start([=]() {
        system(cmdStr.toUtf8().data());
    });
 #endif
#if defined(Q_OS_LINUX)
    qint64 processPid = 0;
    if (QProcess::startDetached(fileStr, {}, wdirStr, &processPid)) {
        qDebug() << __func__ << "Succees";
    } else {
        qDebug() << __func__ << "Failure";
    }
#endif
#if defined(Q_OS_MACOS)
    qint64 processPid = 0;
    if (QProcess::startDetached(fileStr, {}, wdirStr, &processPid)) {
        qDebug() << __func__ << "Succees";
    } else {
        qDebug() << __func__ << "Failure";
    }
#endif
    return 0;
}

int ProjectDbHelper::runUploadScript(const QString &pid, int msecs)
{
    qDebug() << __func__ << "pid:" << pid;
    QString fileStr = readRecordByPid(pid, CIT_USPATH);
    if (fileStr.length() <= 0) {
        qDebug() << __func__ << "Not found script file path";
        return -1;
    }
    QString wdirStr = readRecordByPid(pid, CIT_WDIR);
    if (wdirStr.length() <= 0) {
        qDebug() << __func__ << "Not found script work dir";
        return -2;
    }
    QFile file(fileStr);
    QDir dir(wdirStr);
    if (!file.exists() || !dir.exists()) {
        qWarning() << "scriptPath or scriptWorkDir not exists, stop execute script";
        return -3;
    }
    QString cmdStr = "cmd.exe /c \"" + fileStr + "\"";
    return system(cmdStr.toUtf8().data());
}
