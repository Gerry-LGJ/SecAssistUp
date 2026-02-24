#ifndef FILICONTOOLS_H
#define FILICONTOOLS_H

#include <QObject>
#include <QFont>
#include <QFontDatabase>
#include <QApplication>
#include "../common/filicondef.h"
#include "../common/singleton.h"

class FilIconTools : public QObject
{
    Q_OBJECT
private:
    explicit FilIconTools(QObject *parent = nullptr);
    void init(void);

public:
    SINGLETON(FilIconTools)
    static QString convert(FilIcons::Type source);
    static QFont   font();

};

#endif // FILICONTOOLS_H
