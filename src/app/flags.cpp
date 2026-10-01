#include "flags.h"

#include <QFile>

namespace flags {

QIcon forLanguage(const QString &language)
{
    // the flags are PNG renderings of resources/flags/<cc>.svg (square, the 3:2 flag in the middle)
    const QString country = language.section(u'_', 1, 1).toLower();
    if (country.isEmpty()) return {};
    QIcon icon;
    for (const int size : {16, 24, 32, 48}) {
        const QString file = QStringLiteral(":/resources/flags/%1-%2.png").arg(country).arg(size);
        if (QFile::exists(file)) icon.addFile(file);
    }
    return icon;
}

}  // namespace flags
