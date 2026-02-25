#include <QApplication>

#include "helper/settingshelper.h"
#include "helper/log.h"
#include "helper/translatehelper.h"
#include "service/mainservice.h"
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
    const char *uri = "SecAssistUp";

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

    // Start Main Service
    MainService::getInstance()->init();

    // return app.exec();
    const int exec = QApplication::exec();
    if (exec == 931) {
        QProcess::startDetached(qApp->applicationFilePath(), qApp->arguments());
    }
    return exec;
}
