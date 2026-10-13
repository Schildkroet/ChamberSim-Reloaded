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

#include <QString>
#include <QStringList>

class QCoreApplication;

// Installs the translators for the language in use and exposes the choices for
// the language menu. American English (the source language) is the default and
// is used whenever no translation matches.
namespace translation
{

// Languages that ship a translation, always including "en_US". Normalized ("de_DE").
[[nodiscard]] QStringList availableLanguages();

// The user's stored choice; empty means "follow the system language".
[[nodiscard]] QString storedLanguage();
void storeLanguage(const QString &language);

// Resolves the language (override, else stored choice, else system UI languages),
// installs the application and Qt translators and sets the default QLocale.
// Returns the language that is now active.
QString install(QCoreApplication &app, const QString &overrideLanguage = {});

// "Deutsch (Deutschland)" style display name for menus, in the language's own words.
[[nodiscard]] QString displayName(const QString &language);

}  // namespace translation
