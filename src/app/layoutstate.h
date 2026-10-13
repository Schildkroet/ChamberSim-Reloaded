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

#pragma once

#include <QByteArray>
#include <QSettings>
#include <QSplitter>
#include <QString>

// Remembers the sizes of a splitter across sessions (saved whenever a handle is moved).
inline void rememberSplitter(QSplitter *splitter, const QString &key)
{
    const QString path = QStringLiteral("layout/") + key;
    splitter->restoreState(QSettings().value(path).toByteArray());
    QObject::connect(splitter, &QSplitter::splitterMoved, splitter,
                     [splitter, path] { QSettings().setValue(path, splitter->saveState()); });
}
