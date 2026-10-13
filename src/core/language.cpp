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

#include "language.h"

#include <algorithm>

namespace csr
{

QString normalizeLanguage(const QString &tag)
{
    QString t = tag.section(u'.', 0, 0).section(u'@', 0, 0);  // strip ".UTF-8" and "@euro"
    t.replace(u'-', u'_');
    const qsizetype us = t.indexOf(u'_');
    if (us < 0) return t.toLower();
    return t.left(us).toLower() + u'_' + t.mid(us + 1).toUpper();
}

QString resolveLanguage(const QStringList &requested, const QStringList &available)
{
    const auto languageOf = [](const QString &l) { return l.section(u'_', 0, 0); };
    QStringList avail;
    avail.reserve(available.size());
    std::ranges::transform(available, std::back_inserter(avail), normalizeLanguage);

    for (const QString &raw : requested)
    {
        const QString want = normalizeLanguage(raw);
        if (want.isEmpty()) continue;
        if (avail.contains(want)) return want;
        if (want == kDefaultLanguage) return kDefaultLanguage;
        const auto sameLanguage = std::ranges::find_if(avail, [&](const QString &a) { return languageOf(a) == languageOf(want); });
        if (sameLanguage != avail.end()) return *sameLanguage;
        if (languageOf(want) == languageOf(kDefaultLanguage)) return kDefaultLanguage;  // any English -> default
    }
    return kDefaultLanguage;
}

}  // namespace csr
