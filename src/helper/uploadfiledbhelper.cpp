#include "uploadfiledbhelper.h"

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

#include "projectdbhelper.h"

UploadFileDbHelperCallable::UploadFileDbHelperCallable(QObject *parent)
{ }

UploadFileDbHelper::UploadFileDbHelper(QObject *parent)
    : QObject{parent}
{
    this->db             = ProjectDbHelper::getInstance()->getSqlDatabase();
    this->defDbFilePath  = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/user.db";
    this->defDbTableName = "uploadfile";
    initialize();
}

void UploadFileDbHelper::initialize()
{
    qDebug() << "UploadFileDbHelper::initialize";

    if (!this->db.isOpen()) {
        if (this->db.open()) {
            qDebug() << "UploadFile Database opened successfully!";
        } else {
            qWarning() << "UploadFile Database opened failure:" << this->db.lastError().text();
            return ;
        }
    }

    QSqlQuery query(this->db);
    query.prepare("create table if not exists uploadfile("
                  "fid TEXT primary key, "
                  "pid TEXT not null, "
                  "name TEXT not null, "
                  "dir TEXT not null, "
                  "enable INTEGER);");
    if (!query.exec()) {
        qWarning() << "Create Query.exec failure, reason:" << query.lastError().text();
    }
}

bool UploadFileDbHelper::add(
    const QString &fid,
    const QString &pid,
    const QString &name,
    const QString &dir,
    const bool &enable)
{
    QSqlQuery query(this->db);
    query.prepare("insert into uploadfile (fid, pid, name, dir, enable) "
                                  "values (:fid, :pid, :name, :dir, :enable);");
    query.bindValue(":fid", fid);
    query.bindValue(":pid", pid);
    query.bindValue(":name", name);
    query.bindValue(":dir", dir);
    query.bindValue(":enable", enable);
    if (!query.exec()) {
        qWarning() << "add query.exec failure, reason:" << query.lastError().text();
        return false;
    }
    return true;
}

bool UploadFileDbHelper::del(const QString &fid)
{
    qDebug() << "del fid:" << fid;
    QSqlQuery query(this->db);
    query.prepare("delete from uploadfile where fid=:fid;");
    query.bindValue(":fid", fid);
    if (!query.exec()) {
        qWarning() << "del query.exec failure, reason:" << query.lastError().text();
        return false;
    }
    return true;
}

bool UploadFileDbHelper::modify(const QString &fid, const QString &pid, const QString &name, const QString &dir, const bool &enable)
{
    QSqlQuery query(this->db);
    query.prepare("UPDATE uploadfile SET "
                  "pid = :pid, "
                  "name = :name, "
                  "dir = :dir, "
                  "enable = :enable "
                  "WHERE fid = :fid;");
    query.bindValue(":fid", fid);
    query.bindValue(":pid", pid);
    query.bindValue(":name", name);
    query.bindValue(":dir", dir);
    query.bindValue(":enable", enable ? 1 : 0);
    if (!query.exec()) {
        qWarning() << "modify query.exec failure, reason:" << query.lastError().text();
        return false;
    }
    return true;
}

void UploadFileDbHelper::readAllByPid(const QString &pid, UploadFileDbHelperCallable *cb)
{
    QPointer<UploadFileDbHelperCallable> callable(cb);
    QThreadPool::globalInstance()->start([=]() {
        if (!callable.isNull()) {
            callable->start();
        }
        QSqlQuery query(this->db);
        query.prepare("select * from uploadfile WHERE pid = :pid;");
        query.bindValue(":pid", pid);
        if (!query.exec()) {
            callable->error(query.lastError().text());
            callable->finish();
            return ;
        }
        while (query.next()) {
            QString fid             = query.value("fid").toString();
            QString pid             = query.value("pid").toString();
            QString name            = query.value("name").toString();
            QString dir             = query.value("dir").toString();
            bool    enable          = query.value("enable").toBool();
            if (!callable.isNull()) {
                callable->readAllByPid(fid, pid, name, dir, enable);
            }
        }
        if (!callable.isNull()) {
            callable->finish();
        }
    });
}

void UploadFileDbHelper::readRecordByFid(const QString &fid, UploadFileDbHelperCallable *cb)
{
    QPointer<UploadFileDbHelperCallable> callable(cb);
    QThreadPool::globalInstance()->start([=]() {
        if (!callable.isNull()) {
            callable->start();
        }
        QSqlQuery query(this->db);
        query.prepare("select * from uploadfile WHERE fid =: fid;");
        query.bindValue(":fid", fid);
        if (!query.exec()) {
            callable->error(query.lastError().text());
            callable->finish();
            return ;
        }
        while (query.next()) {
            QString fid  = query.value("fid").toString();
            QString pid  = query.value("pid").toString();
            QString name = query.value("name").toString();
            QString dir  = query.value("dir").toString();
            bool enable  = query.value("enable").toBool();
            if (!callable.isNull()) {
                callable->readRecordByFid(fid, pid, name, dir, enable);
            }
            if (!callable.isNull()) {

            }
        }
        if (!callable.isNull()) {
            callable->finish();
        }
    });
}

QString UploadFileDbHelper::readRecordByFid(const QString &fid, int cit)
{
    UfColumnIndexType _cit = (UfColumnIndexType)cit;
    if (cit < 0 || UfColumnIndexType::CIT_MAX <= cit) {
        qWarning() << __func__ << "cit over range:" << cit;
        return "";
    }
    QString ret;
    QSqlQuery query(this->db);
    query.prepare("select * from uploadfile WHERE fid=:fid;");
    query.bindValue(":fid", fid);
    if (!query.exec()) {
        qWarning() << __func__ << "query.exec failure, reason:" << query.lastError().text();
        return "";
    }
    while (query.next()) {
        ret = query.value(getColumnDisplayName(_cit)).toString();
    }
    qDebug() << __func__ << "ret:" << ret;
    return ret;
}

bool UploadFileDbHelper::updateRecordByFid(const QString &fid, int cit, const QString &text)
{
    UfColumnIndexType _cit = (UfColumnIndexType)cit;
    if (cit < 0 || UfColumnIndexType::CIT_MAX <= cit) {
        qWarning() << __func__ << "cit over range:" << cit;
        return false;
    }
    QSqlQuery query(this->db);
    QString citStr = getColumnDisplayName(_cit);
    qDebug() << __func__ << "fid:" << fid  << "cit:" << cit << "text:" << text << "citStr:" << citStr;
    query.prepare("UPDATE uploadfile SET " + citStr + " = :" + citStr + "WHERE fid=:fid");
    query.bindValue(":" + citStr, text);
    query.bindValue(":fid", fid);
    if (!query.exec()) {
        qWarning() << __func__ << "query.exec failure, reason:" << query.lastError().text();
        return false;
    }
    return true;
}

























