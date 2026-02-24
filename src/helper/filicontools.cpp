#include "filicontools.h"

#include <QDebug>

static QFont mIconFont;
static bool mFontLoaded;

FilIconTools::FilIconTools(QObject *parent)
    : QObject{parent}
{
    mFontLoaded = false;
    init();
}

void FilIconTools::init()
{
    // Loader Font
    int fontfd = QFontDatabase::addApplicationFont(":/font/FluentIcons.ttf");
    if (fontfd != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontfd);
        if (!fontFamilies.isEmpty()) {
            mIconFont.setFamily(fontFamilies.first());
            mFontLoaded = true;
        } else {
            qDebug() << __func__ << "fontFamilies is empty.";
        }
    } else {
        qDebug() << __func__ << "Loading FluentIcons.ttf Failure.";
    }
    if (!mFontLoaded) {
        mIconFont = QApplication::font(); // set default font if load fail.
    }
}

QString FilIconTools::convert(FilIcons::Type source)
{
    return QString(QChar((int)source));
}

QFont FilIconTools::font()
{
    return mIconFont;
}
