#pragma once

#include <QString>

namespace Log {
    QString prettyProductInfoWrapper();
    void setup(char *argv[], const QString &app, int level = 4);
}
