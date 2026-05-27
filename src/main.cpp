#include <QApplication>
#include <QEventLoop>
#include <QTimer>

#include "helper/settingshelper.h"
#include "helper/log.h"
#include "helper/translatehelper.h"
#include "service/mainservice.h"
#include "service/updateservice.h"
#include "version.h"
#include "window/mainwindow.h"
#include "window/loginwindow.h"
#ifdef WIN32
#   include "common/app_dmp.h"
#endif


int main(int argc, char *argv[])
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QApplication app(argc, argv);
    QStringList arguments = app.arguments();
    const char *uri       = "SecAssistUp";

#ifdef WIN32
    ::SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)MyUnhandledExceptionFilter);
#endif
#ifdef Q_OS_LINUX
    // fix bug UOSv20 does not print logs
    qputenv("QT_LOGGING_RULES", "");
    // fix bug UOSv20 v-sync does not work
    qputenv("QSG_RENDER_LOOP", "basic");
#endif

    QApplication::setOrganizationName("TimeChicken");
    QApplication::setOrganizationDomain("https://timechicken.cc");
    QApplication::setApplicationName("SecAssistUp");
    QApplication::setApplicationDisplayName("SecAssistUp");
    QApplication::setApplicationVersion(APPLICATION_VERSION);


    SettingsHelper::getInstance()->init(argv);
    TranslateHelper::getInstance()->init();

    // When restarting the application, we need to wait for a moment to ensure that the previous
    // application has sufficient time to completely exit.
    {
        QEventLoop loop;
        QTimer::singleShot(200, &loop, &QEventLoop::quit);
        loop.exec();
    }

    // once only
    if (MainService::runOnceOnly() != 0) {
        return 0;
    }


    Log::setup(argv, uri);

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#  if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#  endif
#endif

    // The window display will be controlled by the MainService for the display logic.
#if 0
    MainWindow w;
    w.show();
#endif
#if 0
    LoginWindow w;
    w.show();
#endif

    // check for update
    if (UpdateService::checkForAutoUpdate()) {
        return 932;
    }
    if (arguments.size() >= 2 && arguments.at(1).startsWith("-update")) {
        if (UpdateService::checkForAutoUpdate(true)) {
            return 932;
        }
    }

    // Start Main Service
    MainService::getInstance()->init();

    // return app.exec();
    const int exec = QApplication::exec();

    qDebug() << __func__ << "exec exit with code" << exec << ".";

    if (exec == 931) {
        QProcess::startDetached(qApp->applicationFilePath(), qApp->arguments());
    } else if (exec == 932) {
        QStringList arg;
        arg << "-update";
        QProcess::startDetached(qApp->applicationFilePath(), arg);
    }
    return exec;
}
