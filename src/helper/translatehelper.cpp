#include "translatehelper.h"

#include <QApplication>
#include <QLocale>

#include "settingshelper.h"

TranslateHelper::TranslateHelper(QObject *parent)
    : QObject{parent}
{
    _languages << "en_US" << "zh_CN";
    _current = SettingsHelper::getInstance()->getLanguage(getLocaleLanguage());
}

void TranslateHelper::init()
{
    _translator = new QTranslator(this);
    QApplication::installTranslator(_translator);
    QString translatorDir = QApplication::applicationDirPath() + "/i18n";
    QString translatorPath = QString::fromStdString("%1/%2_%3.qm").arg(translatorDir,
                                                                       QApplication::applicationName(),
                                                                       _current);
    qDebug() << __func__ << translatorPath;
    if (_translator->load(translatorPath)) {
    } else {
        qDebug() << __func__ << "failure.";
    }
}

QString TranslateHelper::getLocaleLanguage()
{
    QLocale locale;
    QString ret("en_US");
    if (locale.language() == QLocale::Language::Chinese) {
        ret = "zh_CN";
    } else if (locale.language() == QLocale::Language::English) {
        ret = "en_US";
    }
    qDebug() << __func__ << ret;
    return ret;
}

void TranslateHelper::switchLanguage(const QString &language)
{
    QTranslator *translator = new QTranslator(this);
    QString translatorDir = QApplication::applicationDirPath() + "/i18n";
    QString translatorPath = QString::fromStdString("%1/%2.qm").arg(translatorDir, language);
    qDebug() << __func__ << translatorPath;
    if (translator->load(translatorPath)) {
        QApplication::instance()->removeTranslator(_translator);
        delete _translator;
        _translator = translator;
        QApplication::installTranslator(translator);
    } else {
        delete translator;
        translator = nullptr;
        qDebug() << __func__ << "failure.";
    }
}

TranslateHelper::~TranslateHelper() = default;

