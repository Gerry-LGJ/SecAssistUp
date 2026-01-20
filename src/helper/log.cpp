#include "log.h"
#include <QtGlobal>
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMessageLogContext>
#include <QMutex>
#include <QSysInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QDebug>
#include <QDir>
#include <QThread>
#include <iostream>

#include "../version.h"

using namespace std;

#ifdef WIN32
#  include <process.h>
#else
#  include <unistd.h>
#endif
// *************
#ifndef QT_ENDL
#  if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
#    define QT_ENDL Qt::endl
#  else
#    define QT_ENDL endl
#  endif
#endif

#define LOG_FILE_MAX_SIZE 10485760LL

// var
static int              g_logLevel     = 4;
static QString          g_app          = {};
static QString          g_filePath     = {};
static QString          g_fileDir      = {};
static bool             g_once         = false;
static bool             g_logError     = false;
static bool             g_overwrite    = true;
static std::unique_ptr<QFile>            g_logFile      = nullptr;
static std::unique_ptr<QTextStream>      g_logStream    = nullptr;

// roll log
static int              g_file_count        = 6;
static long long        g_file_maxsize      = 5 * 1024 * 1024LL; // 5M
static long long        g_file_writedsize   = 0;
static QFile           *g_file_roll         = nullptr;

// Log Level Map Define
std::map<QtMsgType, int> logLevelMap = {
    {QtFatalMsg,      0},
    {QtCriticalMsg,   1},
    {QtWarningMsg,    2},
    {QtInfoMsg,       3},
    {QtDebugMsg,      4}
};

QString Log::prettyProductInfoWrapper()
{
    auto productName = QSysInfo::prettyProductName();
#if defined(Q_OS_WIN)
    QSettings regKey{
                     QString::fromUtf8(R"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion)"),
                     QSettings::NativeFormat};
    if (regKey.contains(QString::fromUtf8("CurrentBuildNumber"))) {
        auto buildNumber = regKey.value(QString::fromUtf8("CurrentBuildNumber")).toInt();
        if (buildNumber > 0) {
            if (buildNumber < 9200) {
                productName = QString::fromUtf8("Windows 7 build %1").arg(buildNumber);
            } else if (buildNumber < 10240) {
                productName = QString::fromUtf8("Windows 8 build %1").arg(buildNumber);
            } else if (buildNumber < 22000) {
                productName = QString::fromUtf8("Windows 10 build %1").arg(buildNumber);
            } else {
                productName = QString::fromUtf8("Windows 11 build %1").arg(buildNumber);
            }
        }
    }
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
#  if defined(Q_OS_MACOS)
    auto macosVersionFile =
        QString::fromUtf8("/System/Library/CoreServices/.SystemVersionPlatform.plist");
    auto fi = QFileInfo(macosVersionFile);
    if (fi.exists() && fi.isReadable()) {
        auto plistFile = QFile(macosVersionFile);
        plistFile.open(QIODevice::ReadOnly);
        while (!plistFile.atEnd()) {
            auto line = plistFile.readLine();
            if (line.contains("ProductUserVisibleVersion")) {
                auto nextLine = plistFile.readLine();
                if (nextLine.contains("<string>")) {
                    QRegularExpression re(QString::fromUtf8("\\s*<string>(.*)</string>"));
                    auto matches = re.match(QString::fromUtf8(nextLine));
                    if (matches.hasMatch()) {
                        productName = QString::fromUtf8("macOS ") + matches.captured(1);
                        break;
                    }
                }
            }
        }
    }
#  endif
#endif

    return productName;
}

static QFile *getQFileHandle()
{
    QFile *handle = nullptr;
    int logIndex = 0;
    QString file_full_name;
    const QString logDir = g_fileDir;

    // 未写满文件
    if (g_file_roll != nullptr && g_file_writedsize < g_file_maxsize) {
        handle = g_file_roll;
        return handle;
    }
    // 写满文件
    if (g_file_roll != nullptr && g_file_writedsize >= g_file_maxsize) {
        qDebug() << __func__ << "file overflow, to close file";
        g_file_roll->close();
        delete g_file_roll;
        g_file_roll = nullptr;
        g_file_writedsize = 0;
    }

    // 滚存逻辑
    for (int i = 0; i < g_file_count; i++) {
        file_full_name = QString("%1/%2.%3.log").arg(logDir, g_app).arg(i);
        QFile file(file_full_name);
        if (!file.exists()) {
            logIndex = i;
            break;
        } else {
            qint64 size = file.size();
            if (size >= g_file_maxsize) {
                if (i == (g_file_count - 1)) {
                    // 轮询到最后一个文件，发现此文件也已经写满，则使用新的logindex创建一个新的文件继续写入
                    // 文件内容变化：1,2,3,4,5 => 0,1,2,3,4
                    QFile::remove(QString("%1/%2.%3.log").arg(logDir, g_app).arg(0));
                    for (int j = 0; j < g_file_count - 1; j++) {
                        QString _file_full_name;
                        file_full_name = QString("%1/%2.%3.log").arg(logDir, g_app).arg(j + 1);
                        _file_full_name = QString("%1/%2.%3.log").arg(logDir, g_app).arg(j);
                        qDebug() << __func__ << file_full_name << "=>" << _file_full_name;
                        QFile::rename(file_full_name, _file_full_name);
                    }
                    logIndex = g_file_count - 1;
                    break;
                }
            } else {
                // 如果文件没有满，则继续写入
                logIndex = i;
                g_file_writedsize = size;
                break;
            }
        }
    }
    handle = new QFile(QString("%1/%2.%3.log").arg(logDir, g_app).arg(logIndex));
    qDebug() << __func__ << handle->fileName();
    if (!handle->open(QFile::WriteOnly | QFile::Append)) {
        std::cerr << "Can't open file to write: " << qPrintable(handle->errorString()) << std::endl;
        handle->reset();
        delete handle;
        handle = nullptr;
    }
    g_file_roll = handle;
    return handle;
}

static inline void messageHandler_1(QtMsgType type, const QMessageLogContext &context, const QString& msg)
{
    static QMutex mutex;
    mutex.lock();
    QString time = QDateTime::currentDateTime().toString(QString("[ yyyy-MM-dd HH:mm:ss:zzz ]"));
    QString mmsg;
    switch(type)
    {
    case QtDebugMsg:
        mmsg = QString("%1: Debug:\t%2 (file:%3, line:%4, func: %5)").arg(time).arg(msg).arg(QString(context.file)).arg(context.line).arg(QString(context.function));
        break;
    case QtInfoMsg:
        mmsg = QString("%1: Info:\t%2 (file:%3, line:%4, func: %5)").arg(time).arg(msg).arg(QString(context.file)).arg(context.line).arg(QString(context.function));
        break;
    case QtWarningMsg:
        mmsg = QString("%1: Warning:\t%2 (file:%3, line:%4, func: %5)").arg(time).arg(msg).arg(QString(context.file)).arg(context.line).arg(QString(context.function));
        break;
    case QtCriticalMsg:
        mmsg = QString("%1: Critical:\t%2 (file:%3, line:%4, func: %5)").arg(time).arg(msg).arg(QString(context.file)).arg(context.line).arg(QString(context.function));
        break;
    case QtFatalMsg:
        mmsg = QString("%1: Fatal:\t%2 (file:%3, line:%4, func: %5)").arg(time).arg(msg).arg(QString(context.file)).arg(context.line).arg(QString(context.function));
        abort();
    }
    QFile file(g_filePath);
    file.open(QIODevice::ReadWrite | QIODevice::Append);
    if(file.size() > LOG_FILE_MAX_SIZE) {
        file.close();
        file.open(QFile::WriteOnly|QFile::Truncate);
    }
    QTextStream stream(&file);
    qDebug("%s", mmsg.toLatin1().data());
    stream << mmsg << "\r\n";
    stream.flush();
    file.flush();
    file.close();
    mutex.unlock();
}

static inline void messageHandler_2(QtMsgType type, const QMessageLogContext &context, const QString& msg)
{
    if (logLevelMap[type] > g_logLevel) {
        return ;
    }
    if (msg.isEmpty()) {
        return ;
    }

    QString levelName;
    switch (type) {
    case QtDebugMsg:
        levelName = QStringLiteral("Debug");
        break;
    case QtInfoMsg:
        levelName = QStringLiteral("Info");
        break;
    case QtWarningMsg:
        levelName = QStringLiteral("Warning");
        break;
    case QtCriticalMsg:
        levelName = QStringLiteral("Critical");
        break;
    case QtFatalMsg:
        levelName = QStringLiteral("Fatal");
        break;
    }
    // 从文件路径中截取文件名，简化Log内容输出
    QString fileAndLineStr; // log.cpp:173
    if (context.file) {
        char fn[256] = {0};
        const char *p = NULL;
        if ((p = strrchr(context.file, '/')) != NULL) {
            snprintf(fn, sizeof(fn), "%s", p + 1);
        } else if ((p = strrchr(context.file, '\\')) != NULL) {
            snprintf(fn, sizeof(fn), "%s", p + 1);
        }
        fileAndLineStr = QString("%1:%2").arg(QString::fromStdString(std::string(fn)), QString::number(context.line));
    }
    // 时间戳
    QString dateTimeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss:zzz");
    // 线程
    QString threadStr = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    // 构造最终输出字符串
    const QString finalMessage = QString("[ %1 ][%2][%3]\t%4")
                                     .arg(dateTimeStr, fileAndLineStr, levelName, msg);
    if (type == QtInfoMsg || type == QtDebugMsg) {
        std::cout << qPrintable(finalMessage) << std::endl;
    } else {
        std::cerr << qPrintable(finalMessage) << std::endl;
    }
    if (!g_logFile) {
        g_logFile = std::make_unique<QFile>(g_filePath);
        if (!g_logFile->open(QFile::WriteOnly | QFile::Text | QFile::Append)) {
            std::cerr << "Can't open file to write: " << qPrintable(g_logFile->errorString()) << std::endl;
            g_logFile->reset();
            g_logError = true;
            return ;
        }
    }
    if (!g_logStream) {
        g_logStream = std::make_unique<QTextStream>();
        g_logStream->setDevice(g_logFile.get());
    }
    (*g_logStream) << finalMessage << QT_ENDL;
    g_logStream->flush();
}

// 日志滚存
static inline void messageHandler_3(QtMsgType type, const QMessageLogContext &context, const QString& msg)
{
    if (logLevelMap[type] > g_logLevel) {
        return ;
    }
    if (msg.isEmpty()) {
        return ;
    }

    QString levelName;
    switch (type) {
    case QtDebugMsg:
        levelName = QStringLiteral("Debug");
        break;
    case QtInfoMsg:
        levelName = QStringLiteral("Info");
        break;
    case QtWarningMsg:
        levelName = QStringLiteral("Warning");
        break;
    case QtCriticalMsg:
        levelName = QStringLiteral("Critical");
        break;
    case QtFatalMsg:
        levelName = QStringLiteral("Fatal");
        break;
    }
    // 从文件路径中截取文件名，简化Log内容输出
    QString fileAndLineStr; // log.cpp:173
    if (context.file) {
        char fn[256] = {0};
        const char *p = NULL;
        if ((p = strrchr(context.file, '/')) != NULL) {
            snprintf(fn, sizeof(fn), "%s", p + 1);
        } else if ((p = strrchr(context.file, '\\')) != NULL) {
            snprintf(fn, sizeof(fn), "%s", p + 1);
        }
        fileAndLineStr = QString("%1:%2").arg(QString::fromStdString(std::string(fn)), QString::number(context.line));
    }
    // 时间戳
    QString dateTimeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss:zzz");
    // 线程
    QString threadStr = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    // 构造最终输出字符串
    const QString finalMessage = QString("%1 [%2][%3][%4]\t%5")
                                     .arg(dateTimeStr, fileAndLineStr, levelName, threadStr, msg);
    if (type == QtInfoMsg || type == QtDebugMsg) {
        std::cout << qPrintable(finalMessage) << std::endl;
    } else {
        std::cerr << qPrintable(finalMessage) << std::endl;
    }
    QFile *filehandle = getQFileHandle();
    if (filehandle != nullptr) {
        QTextStream stream(filehandle);
        stream << finalMessage << QT_ENDL;
        stream.flush();
        g_file_writedsize += finalMessage.length();
    }
}

void Log::setup(char *argv[], const QString &app, int level)
{
    Q_ASSERT(!app.isEmpty());
    if (app.isEmpty()) {
        return ;
    }
    if (g_once) {
        return ;
    }
    g_once = true;
    g_logLevel = level;
    g_app = app;
    const QString appPath = QString::fromStdString(argv[0]);
    // 格式化log文件格式
    QString logFileName;
    if (g_overwrite) {
        logFileName = QString("%2.log").arg(g_app);
    } else {
        logFileName = QString("%1_%2.log").arg(
            g_app,
            QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));
    }
    // 定位log输出目录
    const QString logDirPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/log";
    // const QString logDirPath = QCoreApplication::applicationDirPath() + "/log";

    qDebug() << "logDirPath: " << logDirPath;
    const QDir logDir(logDirPath);
    if (!logDir.exists()) {
        if (!logDir.mkpath(logDirPath)) {
            qWarning() << "logDir.mkpath() failure.";
        }
    }
    g_filePath = logDir.filePath(logFileName);
    g_fileDir  = logDirPath;

    // 装载日志系统
    // qInstallMessageHandler(messageHandler_1);
    // qInstallMessageHandler(messageHandler_2);
    qInstallMessageHandler(messageHandler_3);

    qInfo() << "===================================================";
    qInfo() << "[AppName]" << g_app;
    qInfo() << "[AppVersion]" << APPLICATION_VERSION;
    qInfo() << "[AppPath]" << appPath;
    qInfo() << "[QtVersion]" << QT_VERSION_STR;
#ifdef WIN32
    qInfo() << "[ProcessId]" << QString::number(_getpid());
#else
    qInfo() << "[ProcessId]" << QString::number(getpid());
#endif
    qInfo() << "[GitHashCode]" << COMMIT_HASH;
    qInfo() << "[DeviceInfo]";
    qInfo() << "  [DeviceId]" << QSysInfo::machineUniqueId();
    qInfo() << "  [Manufacturer]" << prettyProductInfoWrapper();
    qInfo() << "  [CPU_ABI]" << QSysInfo::currentCpuArchitecture();
    qInfo() << "[LOG_LEVEL]" << g_logLevel;
    qInfo() << "[LOG_OVERWRITE]" << g_overwrite;
    qInfo() << "[LOG_PATH]" << g_filePath;
    qInfo() << "===================================================";
}
