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

#include "dates.h"

#include <QDate>
#include <QDateTime>
#include <QRegularExpression>

using namespace Qt::StringLiterals;

namespace csr
{

std::optional<QString> normalizedDate(const QVariant &value)
{
    if (value.typeId() == QMetaType::QDate)
    {
        const QDate d = value.toDate();
        return d.isValid() ? std::optional(d.toString(Qt::ISODate)) : std::nullopt;
    }
    if (value.typeId() == QMetaType::QDateTime)
    {
        const QDateTime d = value.toDateTime();
        return d.isValid() ? std::optional(d.date().toString(Qt::ISODate)) : std::nullopt;
    }
    const QString text = value.toString().trimmed();
    if (text.isEmpty() || text.startsWith(u"0000-00-00")) return QString();
    static const QRegularExpression iso(uR"(^(\d{4})[-/](\d{1,2})[-/](\d{1,2})(?:[ T].*)?$)"_s);
    static const QRegularExpression german(uR"(^(\d{1,2})\.(\d{1,2})\.(\d{4})(?: .*)?$)"_s);
    QDate d;
    if (const auto m = iso.match(text); m.hasMatch()) d = QDate(m.captured(1).toInt(), m.captured(2).toInt(), m.captured(3).toInt());
    else if (const auto g = german.match(text); g.hasMatch())
        d = QDate(g.captured(3).toInt(), g.captured(2).toInt(), g.captured(1).toInt());
    if (!d.isValid()) return std::nullopt;
    return d.toString(Qt::ISODate);
}

}  // namespace csr
