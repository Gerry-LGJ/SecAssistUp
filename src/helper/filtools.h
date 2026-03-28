#ifndef FILTOOLS_H
#define FILTOOLS_H

#include <QObject>
#include <QFile>
#include <QColor>
#include <QRect>
#include <QIcon>
#include "../common/singleton.h"

class FilTools : public QObject
{
    Q_OBJECT

private:
    explicit FilTools(QObject *parent = nullptr);

public:
    SINGLETON(FilTools)

    Q_INVOKABLE int qtMajor();

    Q_INVOKABLE int qtMinor();

    Q_INVOKABLE bool isMacos();

    Q_INVOKABLE bool isLinux();

    Q_INVOKABLE bool isWin();

    Q_INVOKABLE void clipText(const QString &text);

    Q_INVOKABLE QString uuid();

    Q_INVOKABLE QString readFile(const QString &fileName);

    Q_INVOKABLE void setQuitOnLastWindowClosed(bool val);

    Q_INVOKABLE void setOverrideCursor(Qt::CursorShape shape);

    Q_INVOKABLE void restoreOverrideCursor();

    Q_INVOKABLE QString html2PlantText(const QString &html);

    Q_INVOKABLE QString toLocalPath(const QUrl &url);

    Q_INVOKABLE void deleteLater(QObject *p);

    Q_INVOKABLE QString getFileNameByUrl(const QUrl &url);

    Q_INVOKABLE QRect getVirtualGeometry();

    Q_INVOKABLE QString getApplicationDirPath();

    Q_INVOKABLE QString getAppLocalDataLocation();

    Q_INVOKABLE QUrl getUrlByFilePath(const QString &path);

    Q_INVOKABLE QColor withOpacity(const QColor &, qreal alpha);

    Q_INVOKABLE QString md5(const QString &text);

    Q_INVOKABLE QString md5CalculateFile(const QString &filePath,
                                         const std::function<void(qint64)> &progressCallback = nullptr);

    Q_INVOKABLE QString sha256(const QString &text);

    Q_INVOKABLE QString sha256CalculateFile(const QString &filePath,
                                            const std::function<void(qint64)> &progressCallback = nullptr);

    Q_INVOKABLE QString toBase64(const QString &text);

    Q_INVOKABLE QString fromBase64(const QString &text);

    Q_INVOKABLE bool removeDir(const QString &dirPath);

    Q_INVOKABLE bool removeFile(const QString &filePath);

    Q_INVOKABLE void showFileInFolder(const QString &path);

    Q_INVOKABLE void showDirInExplorer(const QString &dir);

    Q_INVOKABLE void showFileTextInNotepad(const QString &path);

    Q_INVOKABLE bool checkDirExist(const QString &dirPath);

    Q_INVOKABLE bool checkFileExist(const QString &filePath);

    Q_INVOKABLE qint64 currentTimestamp();

    Q_INVOKABLE QPoint cursorPos();

    Q_INVOKABLE QIcon windowIcon();

    Q_INVOKABLE int cursorScreenIndex();

    Q_INVOKABLE int windowBuildNumber();

    Q_INVOKABLE bool isWindows11OrGreater();

    Q_INVOKABLE bool isWindows10OrGreater();

    Q_INVOKABLE QString getWallpaperFilePath();

    Q_INVOKABLE QColor imageMainColor(const QImage &image, double bright = 1);

    Q_INVOKABLE QString getCurrentDateTimeToString(const QString &format);

    Q_INVOKABLE QString getUniqueFileName(const QString &originalPath);

    Q_INVOKABLE int getFileSizeByPath(const QString &path);
};

#endif // FILTOOLS_H
