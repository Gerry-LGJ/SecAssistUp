#ifndef TRANSLATEHELPER_H
#define TRANSLATEHELPER_H

#include <QObject>
#include <QTranslator>

#include "../common/singleton.h"
#include "../common/stdafx.h"

class TranslateHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY_AUTO(QString, current)
    Q_PROPERTY_READONLY_AUTO(QStringList, languages)
private:
    explicit TranslateHelper(QObject *parent = nullptr);

public:
    SINGLETON(TranslateHelper)
    ~TranslateHelper() override;
    void init();
    QString getLocaleLanguage();
    Q_INVOKABLE void switchLanguage(const QString &language);

private:
    QTranslator *_translator;
};

#endif // TRANSLATEHELPER_H
