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

#include "translation.h"
#include "log.h"

#include "language.h"

#include <QCoreApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

namespace translation
{

namespace
{

constexpr auto kResourceDir = ":/i18n";
constexpr auto kBaseName = "ChamberSimReloaded_";
constexpr auto kSettingKey = "ui/language";

}  // namespace

QStringList availableLanguages()
{
    QStringList out{csr::kDefaultLanguage};
    const QStringList files =
        QDir(QString::fromLatin1(kResourceDir)).entryList({QStringLiteral("%1*.qm").arg(QLatin1String(kBaseName))}, QDir::Files);
    for (const QString &f : files)
    {
        const QString lang = csr::normalizeLanguage(f.mid(int(qstrlen(kBaseName))).chopped(3));
        if (!lang.isEmpty() && !out.contains(lang)) out.append(lang);
    }
    out.sort();
    return out;
}

QString storedLanguage()
{
    return QSettings().value(QLatin1String(kSettingKey)).toString();
}

void storeLanguage(const QString &language)
{
    QSettings s;
    if (language.isEmpty()) s.remove(QLatin1String(kSettingKey));
    else s.setValue(QLatin1String(kSettingKey), language);
}

QString install(QCoreApplication &app, const QString &overrideLanguage)
{
    QStringList requested;
    if (!overrideLanguage.isEmpty()) requested << overrideLanguage;
    else if (const QString stored = storedLanguage(); !stored.isEmpty()) requested << stored;
    else requested = QLocale::system().uiLanguages();

    const QString language = csr::resolveLanguage(requested, availableLanguages());
    LOG_Info() << "Language:" << language << "(requested" << requested << ")";
    QLocale::setDefault(QLocale(language));  // number formats follow the UI language ('.' for en_US)

    // translators must outlive the application's event loop
    static QTranslator appTranslator;
    static QTranslator qtTranslator;
    if (appTranslator.load(QStringLiteral("%1%2").arg(QLatin1String(kBaseName), language), QString::fromLatin1(kResourceDir)))
        app.installTranslator(&appTranslator);
    if (qtTranslator.load(QLocale(language), QStringLiteral("qtbase"), QStringLiteral("_"),
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        app.installTranslator(&qtTranslator);
    return language;
}

QString displayName(const QString &language)
{
    const QLocale l(language);
    const QString name = l.nativeLanguageName();
    const QString territory = l.nativeTerritoryName();
    if (name.isEmpty()) return language;
    QString text = name.left(1).toUpper() + name.mid(1);
    if (!territory.isEmpty()) text += QStringLiteral(" (%1)").arg(territory);
    return text;
}

}  // namespace translation
