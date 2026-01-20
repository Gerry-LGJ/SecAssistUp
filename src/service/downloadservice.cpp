#include "downloadservice.h"

#include <QDebug>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QDir>
#include <QThreadPool>

#include "mainservice.h"
#include "../helper/appinfo.h"
#include "../window/mainwindow.h"

DownloadService::DownloadService(QObject *parent)
    : QObject{parent}
{
    mNetwork             = Network::getInstance();
    mDownloadReqCallable = new NetworkCallable(this);
    mMainWindow          = MainWindow::getInstance();
    mSettingsHelper      = SettingsHelper::getInstance();
    mFilTools            = FilTools::getInstance();
}

void DownloadService::init()
{
    mUserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, "
                 "like Gecko) Chrome/138.0.0.0 Safari/537.36 Edg/138.0.0.0";
    mDownloadReqSender = nullptr;
    mState     = STATE_TYPE_IDLE;
    initDownloadReqCallable();
}

void DownloadService::initDownloadReqCallable()
{
    MainWindow *win = MainWindow::getInstance();
    connect(mDownloadReqCallable, &NetworkCallable::start, this, [=] {
        mDownloading = true;
    });
    connect(mDownloadReqCallable, &NetworkCallable::finish, this, [=] {
        mDownloading = false;
    });
    connect(mDownloadReqCallable, &NetworkCallable::error, this, [=] (int status, QString errorString, QString result) {
        qDebug() << "initDownloadReqCallable" << "error " <<
            "status:" << status <<
            "errorString:" << errorString <<
            "result:" << result;
        QVariantMap map;
        map["resultCode"]  = MSEvent::RESULT_CODE_NETWORK_ONERROR;
        map["status"]      = status;
        map["errorString"] = errorString;
        map["result"]      = result;
        sendMsgToObject(mDownloadReqSender, MSEvent::EVENT_TYPE_DOWNLOAD_CFM, map);

        mDownloading = false;
        win->setProgressBarDownload(0, "");
        transition(STATE_TYPE_IDLE);
    });
    connect(mDownloadReqCallable, &NetworkCallable::success, this, [=] (QString result) {
        bool isHtml = false, isSecError = false;

        QString contentType = mDownloadReqCallable->replyGetSetCookie("Content-Type");
        if (contentType.length() > 0) {
            qDebug() << "Content-Type:" << contentType;
            if (contentType.contains("text/html")) {
                qDebug() << "includes text/html, start check HTML format";
                QString fileData = mFilTools->readFile(result);
                isHtml = isHtmlContent(fileData);
                qDebug() << "isHtml:" << isHtml;
                if (isHtml && fileData.contains("文件防泄密网关") &&
                    fileData.contains("用户名") &&
                    fileData.contains("密码") &&
                    fileData.contains("登录")) {
                    qDebug() << "The server returned the login page information, but there might be an error in the network request.";
                    isSecError = true;
                    mFilTools->removeFile(result);
                }
            }
        }
        if (!isSecError) {
            if (mFiles.size() == 0) {
                qint64 takeTime = mTakeTimer.elapsed();
                qDebug() << "Download Succeed" << takeTime / 1000 << "s. md5:" << mFilTools->md5CalculateFile(result);
                win->setProgressBarDownload(0, "");
                // run script
                runDownloadScript();
                QVariantMap map;
                map["resultCode"] = MSEvent::RESULT_CODE_SUCCESS;
                sendMsgToObject(mDownloadReqSender, MSEvent::EVENT_TYPE_DOWNLOAD_CFM, map);
                // reset state
                mDownloading = false;
                transition(STATE_TYPE_IDLE);
            } else {
                downloadNext();
                return ;
            }
        } else {
            QVariantMap map;
            map["resultCode"]  = MSEvent::RESULT_CODE_FAIL;
            map["errorString"] = tr("Server has returned to the login page. Please try to log in again.");
            sendMsgToObject(mDownloadReqSender, MSEvent::EVENT_TYPE_DOWNLOAD_CFM, map);
            mDownloading = false;
            win->setProgressBarDownload(0, "");
            transition(STATE_TYPE_IDLE);
        }
    });
    connect(mDownloadReqCallable, &NetworkCallable::downloadProgress, this, [=] (qint64 recv, qint64 total) {
        int progress = (double)((double)recv / (double)total) * 100;
        win->setProgressBarDownload(progress, mActiveFile);
    });
}

void DownloadService::download()
{
    mTakeTimer.start();
    downloadNext();
}

void DownloadService::downloadNext()
{
    QString name    = mFiles.takeFirst();
    QString host    = mSettingsHelper->getWebUrl();
    QString webpath ;
    QString url     ;
    QString path    = mDir + "/" + name;

    if (mWebDir == "/") {
        webpath = "/" + name;
    } else {
        webpath = mWebDir + "/" + name;
    }
    url         = host + QString("/index.php?explorer/fileDownload&path=%1").arg(webpath);

    if (mSettingsHelper->getAppDownloadOverride() == false) {
        path = mFilTools->getUniqueFileName(path);
    }
    qDebug() << __func__ << url << path;

    if (AppInfo::getInstance()->debugEnable()) {
        qDebug() << "Debug Mode, not request download";
        return ;
    }

    mActiveFile = name;

    mNetwork->get(url)
    ->addHeader("User-Agent", mUserAgent)
    ->toDownload(path)
    ->bind(this)
    ->go(mDownloadReqCallable);
}

int DownloadService::runDownloadScript()
{
    if (mScript.length() <= 0) {
        qDebug() << __func__ << "Not found script file path";
        return -1;
    }
    if (mDir.length() <= 0) {
        qDebug() << __func__ << "Not found script work dir";
        return -2;
    }
    QFileInfo scriptInfo(mScript);
    QDir mmDir(mDir);
    if (!scriptInfo.exists() || !mmDir.exists()) {
        qDebug() << "scriptPath or scriptWorkDir not exists, stop execute script";
        return -3;
    }

#if defined(Q_OS_WIN)
    static QString cmdStr;
    cmdStr = "start \"" + scriptInfo.fileName() + "\" /D " + mDir.replace("/", "\\") + " " + mScript.replace("/", "\\");
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

void DownloadService::transition(uint16_t state)
{
    qDebug() << "DownloadService::transition" << "state :" << state;
    mState = state;
}

void DownloadService::sendMsgToObject(QObject *object, uint16_t code, QVariantMap map)
{
    map["resultSupplier"] = "DownloadService";
    if (object) {
        MSEvent *event = new MSEvent(this, code);
        event->setData(QVariant(map));
        QCoreApplication::postEvent(object, event);
    } else {
        qWarning() << __func__ << "Not Object to send.";
    }
}

bool DownloadService::downloadServiceEventHandler(MSEvent *e)
{
    bool result = true;
    uint16_t event = e->getMSEventType();
    qDebug("%s 0x%04x state:%d", __func__, event, mState);

    switch (event) {
    case MSEvent::EVENT_TYPE_DOWNLOAD_REQ: {
        if (mState == STATE_TYPE_IDLE) {
            QVariantMap map = e->getData().toMap();

            mFiles  = map["files"].toStringList();
            mDir    = map["dir"].toString();
            mWebDir = map["webdir"].toString();
            mScript = map["script"].toString();
            if (mFiles.size() == 0 || mDir.length() == 0 || mWebDir.length() == 0) {
                qWarning() << "Invalid mFiles or mDir or mWebDir";
                break;
            }

            download();
            mDownloadReqSender = e->getSender();
            transition(STATE_TYPE_DOWNLOADING);
        } else {
            qDebug() << "Error State";
        }
        result = true;
        break;
    }
    default:
        qWarning("%s Unknow Event Type 0x%04x", __func__, event);
    }

    return result;
}

bool DownloadService::event(QEvent *e)
{
    uint16_t eventType = 0x0000;

    if (e->type() == MSEvent::MSEVENT_BASE_TYPE) {
        MSEvent *event = static_cast<MSEvent *>(e);
        return downloadServiceEventHandler(event);
    }
    return QObject::event(e);
}


/***********************************************************************/
/************************** 判断文本内容是否为html ************************/
/***********************************************************************/

// 检查标签密度
bool DownloadService::checkTagDensity(const QString &content) {
    // 统计标签数量
    static QRegularExpression tagRegex("<[^>]+>");
    QRegularExpressionMatchIterator it = tagRegex.globalMatch(content);

    int tagCount = 0;
    while (it.hasNext()) {
        it.next();
        tagCount++;
    }

    if (tagCount < 3) {
        return false;
    }

    // 移除标签获取纯文本
    QString textContent = content;
    textContent.remove(tagRegex);

    // 计算标签密度
    double tagDensity = static_cast<double>(tagCount) / (tagCount + textContent.length());

    // 标签密度超过阈值且有一定数量的标签
    return tagDensity > 0.08 && tagCount >= 5;
}

// 检查内容开头特征
bool DownloadService::isLikelyHtmlByPrefix(const QString &content) {
    if (content.length() < 100) {
        QString lowerContent = content.toLower();
        return lowerContent.startsWith("<!doctype") ||
               lowerContent.startsWith("<html") ||
               lowerContent.startsWith("<!--") ||
               lowerContent.startsWith("<head") ||
               lowerContent.startsWith("<body");
    }

    QString first100 = content.left(100).toLower();
    return first100.startsWith("<!doctype") ||
           first100.startsWith("<html") ||
           first100.startsWith("<!--") ||
           first100.startsWith("<head") ||
           first100.startsWith("<body");
}

// 主要的HTML检测函数
bool DownloadService::isHtmlContent(const QString &content) {
    if (content.isEmpty() || content.length() < 20) {
        return false;
    }

    // 去除首尾空白字符
    QString trimmedContent = content.trimmed();

    // 快速检查：开头特征
    if (isLikelyHtmlByPrefix(trimmedContent)) {
        return true;
    }

    QString lowerContent = trimmedContent.toLower();

    // 定义HTML特征正则表达式
    static const QVector<QRegularExpression> patterns = {
        // 文档类型声明（最可靠的指标）
        QRegularExpression("<!DOCTYPE\\s+html\\s*(?:PUBLIC\\s+[^>]*)?>", QRegularExpression::CaseInsensitiveOption),

        // HTML标签结构
        QRegularExpression("<html[^>]*>[\\s\\S]*</html>", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("<head[^>]*>[\\s\\S]*</head>", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("<body[^>]*>[\\s\\S]*</body>", QRegularExpression::CaseInsensitiveOption),

        // 常见的HTML标签对
        QRegularExpression("<title[^>]*>[\\s\\S]*</title>", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("<div[^>]*>[\\s\\S]*</div>", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("<span[^>]*>[\\s\\S]*</span>", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("<p[^>]*>[\\s\\S]*</p>", QRegularExpression::CaseInsensitiveOption),

        // 自闭合标签
        QRegularExpression("<meta[^>]+>", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("<link[^>]+>", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("<img[^>]+>", QRegularExpression::CaseInsensitiveOption),

        // 脚本和样式
        QRegularExpression("<script[^>]*>[\\s\\S]*</script>", QRegularExpression::CaseInsensitiveOption),
        QRegularExpression("<style[^>]*>[\\s\\S]*</style>", QRegularExpression::CaseInsensitiveOption),

        // HTML注释
        QRegularExpression("<!--[\\s\\S]*?-->"),

        // 属性格式
        QRegularExpression("<[a-z]+[^>]*\\s+[a-z]+=([\"'])[^\"']*\\1[^>]*>", QRegularExpression::CaseInsensitiveOption),

        // 常见的HTML实体
        QRegularExpression("&(?:amp|lt|gt|quot|nbsp|copy|reg);", QRegularExpression::CaseInsensitiveOption),
    };

    // 计算匹配分数
    int score = 0;

    for (const auto &pattern : patterns) {
        if (pattern.match(lowerContent).hasMatch()) {
            score++;

            // 如果找到高度可信的模式，立即返回
            QString patternStr = pattern.pattern();
            if (patternStr.contains("DOCTYPE") || patternStr.contains("<html")) {
                return true;
            }
        }

        // 达到足够分数就认为是HTML
        if (score >= 3) {
            return true;
        }
    }

    // 检查标签密度
    return checkTagDensity(lowerContent);
}

// 简化版增强检测（合并原advancedHtmlDetection功能）
bool DownloadService::isHtmlContentEnhanced(const QString &content) {
    if (content.isEmpty()) return false;

    QString lowerContent = content.toLower();

    // 权重评分系统
    int score = 0;

    // 高权重特征（每个+3分）
    if (lowerContent.contains("<!doctype")) score += 3;
    if (lowerContent.contains("<html")) score += 3;
    if (lowerContent.contains("<head")) score += 3;
    if (lowerContent.contains("<body")) score += 3;

    // 中权重特征（每个+2分）
    if (lowerContent.contains("<title")) score += 2;
    if (lowerContent.contains("<meta")) score += 2;
    if (lowerContent.contains("<div")) score += 2;
    if (lowerContent.contains("<span")) score += 2;

    // 低权重特征（每个+1分）
    if (lowerContent.contains("<p")) score += 1;
    if (lowerContent.contains("<a")) score += 1;
    if (lowerContent.contains("<img")) score += 1;
    if (lowerContent.contains("<!--")) score += 1;

    // 快速判断
    if (score >= 4) return true;

    // 如果分数较低，检查标签密度
    if (score > 0) {
        return checkTagDensity(lowerContent);
    }

    return false;
}
