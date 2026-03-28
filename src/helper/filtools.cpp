#include "filtools.h"

#include <QGuiApplication>
#include <QClipboard>
#include <QUuid>
#include <QCursor>
#include <QScreen>
#include <QColor>
#include <QFileInfo>
#include <QProcess>
#include <QDir>
#include <QOpenGLContext>
#include <QCryptographicHash>
#include <QTextDocument>
#include <QDateTime>
#include <QSettings>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QIcon>

#ifdef Q_OS_WIN
#  pragma comment(lib, "user32.lib")
#  include <windows.h>
#  include <windowsx.h>
#endif

FilTools::FilTools(QObject *parent)
    : QObject{parent}
{}

int FilTools::qtMajor()
{
    const QString qtVersion = QString::fromLatin1(qVersion());
    const QStringList versionParts = qtVersion.split('.');
    return versionParts[0].toInt();
}

int FilTools::qtMinor()
{
    const QString qtVersion = QString::fromLatin1(qVersion());
    const QStringList versionParts = qtVersion.split('.');
    return versionParts[1].toInt();
}

bool FilTools::isMacos()
{
#if defined(Q_OS_MACOS)
    return true;
#else
    return false;
#endif
}

bool FilTools::isLinux()
{
#if defined(Q_OS_LINUX)
    return true;
#else
    return false;
#endif
}

bool FilTools::isWin()
{
#if defined(Q_OS_WIN)
    return true;
#else
    return false;
#endif
}

void FilTools::clipText(const QString &text)
{
    QGuiApplication::clipboard()->setText(text);
}

QString FilTools::uuid()
{
    return QUuid::createUuid().toString().remove('-').remove('{').remove('}');
}

QString FilTools::readFile(const QString &fileName)
{
    QString content;
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        content = stream.readAll();
    }
    return content;
}

void FilTools::setQuitOnLastWindowClosed(bool val)
{
    QGuiApplication::setQuitOnLastWindowClosed(val);
}

void FilTools::setOverrideCursor(Qt::CursorShape shape)
{
    QGuiApplication::setOverrideCursor(QCursor(shape));
}

void FilTools::restoreOverrideCursor()
{
    QGuiApplication::restoreOverrideCursor();
}

QString FilTools::html2PlantText(const QString &html)
{
    QTextDocument textDocument;
    textDocument.setHtml(html);
    return textDocument.toPlainText();
}

QString FilTools::toLocalPath(const QUrl &url)
{
    return url.toLocalFile();
}

void FilTools::deleteLater(QObject *p)
{
    if (p) {
        p->deleteLater();
    }
}

QString FilTools::getFileNameByUrl(const QUrl &url)
{
    return QFileInfo(url.toLocalFile()).fileName();
}

QRect FilTools::getVirtualGeometry()
{
    return QGuiApplication::primaryScreen()->virtualGeometry();
}

QString FilTools::getApplicationDirPath()
{
    return QGuiApplication::applicationDirPath();
}

QString FilTools::getAppLocalDataLocation()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

QUrl FilTools::getUrlByFilePath(const QString &path)
{
    return QUrl::fromLocalFile(path);
}

QColor FilTools::withOpacity(const QColor &color, qreal opacity) {
    int alpha = qRound(opacity * 255) & 0xff;
    return QColor::fromRgba((alpha << 24) | (color.rgba() & 0xffffff));
}

QString FilTools::md5(const QString &text)
{
    return QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Md5).toHex();
}

/**
 * @brief 计算文件的MD5哈希值（支持大文件）
 * @param filePath 文件路径
 * @param progressCallback 进度回调函数（可选），参数为已读取的字节数
 * @return MD5哈希值的十六进制字符串，失败返回空字符串
 */
QString FilTools::md5CalculateFile(const QString &filePath, const std::function<void (qint64)> &progressCallback)
{
    QFile file(filePath);

    // 尝试以只读方式打开文件
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    // 创建MD5哈希计算器
    QCryptographicHash hash(QCryptographicHash::Md5);

    // 设置合适的缓冲区大小（可根据实际情况调整）
    const qint64 bufferSize = 64 * 1024; // 64KB
    char buffer[bufferSize];

    qint64 bytesRead;
    qint64 totalBytesRead = 0;

    // 分块读取文件并计算哈希
    while (!file.atEnd()) {
        bytesRead = file.read(buffer, bufferSize);

        if (bytesRead > 0) {
            hash.addData(buffer, bytesRead);
            totalBytesRead += bytesRead;

            // 调用进度回调（如果提供）
            if (progressCallback) {
                progressCallback(totalBytesRead);
            }
        }

        // 检查是否读取失败
        if (bytesRead < 0) {
            file.close();
            return QString();
        }
    }

    file.close();

    // 返回MD5的十六进制字符串
    return QString(hash.result().toHex());
}

QString FilTools::sha256(const QString &text)
{
    return QCryptographicHash::hash(text.toUtf8(), QCryptographicHash::Sha256).toHex();
}

QString FilTools::sha256CalculateFile(const QString &filePath, const std::function<void (qint64)> &progressCallback)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);

    const qint64 bufferSize = 64 * 1024; // 64KB
    char buffer[bufferSize];

    qint64 bytesRead;
    qint64 totalBytesRead = 0;

    while (!file.atEnd()) {
        bytesRead = file.read(buffer, bufferSize);

        if (bytesRead > 0) {
            hash.addData(buffer, bytesRead);
            totalBytesRead += bytesRead;

            if (progressCallback) {
                progressCallback(totalBytesRead);
            }
        }

        if (bytesRead < 0) {
            file.close();
            return QString();
        }
    }

    file.close();

    return QString(hash.result().toHex());
}

QString FilTools::toBase64(const QString &text)
{
    return text.toUtf8().toBase64();
}

QString FilTools::fromBase64(const QString &text)
{
    return QByteArray::fromBase64(text.toUtf8());
}

bool FilTools::removeDir(const QString &dirPath)
{
    QDir qDir(dirPath);
    return qDir.removeRecursively();
}

bool FilTools::removeFile(const QString &filePath)
{
    QFile file(filePath);
    return file.remove();
}

void FilTools::showFileInFolder(const QString &path)
{
    // 使用系统自带的资源管理器展示文件
#if defined(Q_OS_WIN)
    QProcess::startDetached("explorer.exe", {"/select,", QDir::toNativeSeparators(path)});
#endif
#if defined(Q_OS_LINUX)
    QFileInfo fileInfo(path);
    auto process = "xdg-open";
    auto arguments = {fileInfo.absoluteDir().absolutePath()};
    QProcess::startDetached(process, arguments);
#endif
#if defined(Q_OS_MACOS)
    QProcess::execute("/usr/bin/osascript",
                      {"-e", "tell application \"Finder\" to reveal POSIX file \"" + path + "\""});
    QProcess::execute("/usr/bin/osascript", {"-e", "tell application \"Finder\" to activate"});
#endif
}

void FilTools::showDirInExplorer(const QString &dir)
{
    // 使用系统自带的资源管理器展示目录
#if defined(Q_OS_WIN)
    QProcess::startDetached("explorer.exe", {"/root,", QDir::toNativeSeparators(dir)});
#endif
}

void FilTools::showFileTextInNotepad(const QString &path)
{
    // 使用系统自带的notepad展示文件
#if defined(Q_OS_WIN)
    QProcess::startDetached("notepad.exe", {QDir::toNativeSeparators(path)});
#endif
}

bool FilTools::checkDirExist(const QString &dirPath)
{
    return QFileInfo(dirPath).isDir();
}

bool FilTools::checkFileExist(const QString &filePath)
{
    return QFileInfo(filePath).isFile();
}

qint64 FilTools::currentTimestamp()
{
    return QDateTime::currentMSecsSinceEpoch();
}

QPoint FilTools::cursorPos()
{
    return QCursor::pos();
}

QIcon FilTools::windowIcon()
{
    return QGuiApplication::windowIcon();
}

int FilTools::cursorScreenIndex()
{
    int screenIndex = 0;
    int screenCount = QGuiApplication::screens().count();
    if (screenCount > 1) {
        QPoint pos = QCursor::pos();
        for (int i = 0; i <= screenCount - 1; ++i) {
            if (QGuiApplication::screens().at(i)->geometry().contains(pos)) {
                screenIndex = i;
                break;
            }
        }
    }
    return screenIndex;
}

int FilTools::windowBuildNumber() {
#if defined(Q_OS_WIN)
    QSettings regKey{
                     QString::fromUtf8(R"(HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion)"),
                     QSettings::NativeFormat};
    if (regKey.contains(QString::fromUtf8("CurrentBuildNumber"))) {
        auto buildNumber = regKey.value(QString::fromUtf8("CurrentBuildNumber")).toInt();
        return buildNumber;
    }
#endif
    return -1;
}

bool FilTools::isWindows11OrGreater() {
    static QVariant var;
    if (var.isNull()) {
#if defined(Q_OS_WIN)
        auto buildNumber = windowBuildNumber();
        if (buildNumber >= 22000) {
            var = QVariant::fromValue(true);
            return true;
        }
#endif
        var = QVariant::fromValue(false);
        return false;
    } else {
        return var.toBool();
    }
}

bool FilTools::isWindows10OrGreater() {
    static QVariant var;
    if (var.isNull()) {
#if defined(Q_OS_WIN)
        auto buildNumber = windowBuildNumber();
        if (buildNumber >= 10240) {
            var = QVariant::fromValue(true);
            return true;
        }
#endif
        var = QVariant::fromValue(false);
        return false;
    } else {
        return var.toBool();
    }
}

QString FilTools::getWallpaperFilePath()
{
#if defined(Q_OS_WIN)
    wchar_t path[MAX_PATH] = {};
    if (::SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, path, FALSE) == FALSE) {
        return {};
    }
    // qDebug() << __func__ << QString::fromWCharArray(path);
    return QString::fromWCharArray(path);
#elif defined(Q_OS_LINUX)
    auto type = QSysInfo::productType();
    if (type == "uos") {
        QProcess process;
        QStringList args;
        args << "--session";
        args << "--type=method_call";
        args << "--print-reply";
        args << "--dest=com.deepin.wm";
        args << "/com/deepin/wm";
        args << "com.deepin.wm.GetCurrentWorkspaceBackgroundForMonitor";
        args << QString("string:'%1'").arg(currentTimestamp());
        process.start("dbus-send", args);
        process.waitForFinished();
        QByteArray result = process.readAllStandardOutput().trimmed();
        int startIndex = result.indexOf("file:///");
        if (startIndex != -1) {
            auto path = result.mid(startIndex + 7, result.length() - startIndex - 8);
            return path;
        }
    } else if (type == "ubuntu") {
        QProcess process;
        QStringList args;
        args << "get";
        args << "org.gnome.desktop.background";
        args << "picture-uri";
        process.start("gsettings", args);
        process.waitForFinished();
        QByteArray result = process.readAllStandardOutput().trimmed();
        result = result.mid(1, result.length() - 2);
        if (result.startsWith("file:///")) {
            auto path = result.mid(7);
            return path;
        }
    }
    return {};
#elif defined(Q_OS_MACOS)
    QProcess process;
    QStringList args;
    args << "-e";
    args << R"(tell application "Finder" to get POSIX path of (desktop picture as alias))";
    process.start("osascript", args);
    process.waitForFinished();
    QByteArray result = process.readAllStandardOutput().trimmed();
    if (result.isEmpty()) {
        return "/System/Library/CoreServices/DefaultDesktop.heic";
    }
    return result;
#else
    return {};
#endif
}

QColor FilTools::imageMainColor(const QImage &image, double bright)
{
    int step = 20;
    int t = 0;
    int r = 0, g = 0, b = 0;
    for (int i = 0; i < image.width(); i += step) {
        for (int j = 0; j < image.height(); j += step) {
            if (image.valid(i, j)) {
                t++;
                QColor c = image.pixel(i, j);
                r += c.red();
                b += c.blue();
                g += c.green();
            }
        }
    }
    return QColor(int(bright * r / t) > 255 ? 255 : int(bright * r / t),
                  int(bright * g / t) > 255 ? 255 : int(bright * g / t),
                  int(bright * b / t) > 255 ? 255 : int(bright * b / t));
}

QString FilTools::getCurrentDateTimeToString(const QString &format)
{
    return QDateTime::currentDateTime().toString(format);
}

QString FilTools::getUniqueFileName(const QString &originalPath)
{
    QFileInfo fileInfo(originalPath);
    QString baseName    = fileInfo.completeBaseName();
    QString suffix      = fileInfo.suffix();
    QString path        = fileInfo.path();

    // mkdir
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // Check if the file is exists. If it does, generate a new name.
    QString newPath = originalPath;
    int counter = 1;

    while(QFile::exists(newPath)) {
        if (suffix.isEmpty()) {
            newPath = QString("%1/%2 (%3)").arg(path, baseName).arg(counter);
        } else {
            newPath = QString("%1/%2 (%3).%4").arg(path, baseName).arg(counter).arg(suffix);
        }
        counter++;
    }

    return newPath;
}

int FilTools::getFileSizeByPath(const QString &path)
{
    QFileInfo info = QFileInfo(path);
    return info.size();
}
