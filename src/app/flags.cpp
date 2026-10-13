/*
 * This file is part of ChamberSim Reloaded.
 *
 * Copyright (C) 2026 Schildkroet
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "flags.h"

#include <QFile>

namespace flags
{

QIcon forLanguage(const QString &language)
{
    // the flags are PNG renderings of resources/flags/<cc>.svg (square, the 3:2 flag in the middle)
    const QString country = language.section(u'_', 1, 1).toLower();
    if (country.isEmpty()) return {};
    QIcon icon;
    for (const int size : {16, 24, 32, 48})
    {
        const QString file = QStringLiteral(":/resources/flags/%1-%2.png").arg(country).arg(size);
        if (QFile::exists(file)) icon.addFile(file);
    }
    return icon;
}

}  // namespace flags
