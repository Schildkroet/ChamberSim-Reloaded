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

#include <QSettings>
#include <QString>
#include <QStringList>

// The loads that were open when the program was closed, and the option to open them again at the next start (Settings > Start-up).
namespace session
{

[[nodiscard]] inline bool reopen()
{
    return QSettings().value(QStringLiteral("session/reopen"), false).toBool();
}

inline void setReopen(bool on)
{
    QSettings().setValue(QStringLiteral("session/reopen"), on);
}

[[nodiscard]] inline QStringList loads()
{
    return QSettings().value(QStringLiteral("session/loads")).toStringList();
}

// The files of the open loads in the order of their tabs, and the one that was shown (an index into `files`, -1: none of them).
[[nodiscard]] inline int current()
{
    return QSettings().value(QStringLiteral("session/current"), -1).toInt();
}

inline void store(const QStringList &files, int current)
{
    QSettings s;
    s.setValue(QStringLiteral("session/loads"), files);
    s.setValue(QStringLiteral("session/current"), current);
}

}  // namespace session
