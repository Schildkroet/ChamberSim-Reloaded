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

#include "reference.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Qt::StringLiterals;

namespace csr
{

double ReferenceRun::number(const QString &key, double fallback) const
{
    const QString s = summary.value(key).toString().section(u' ', 0, 0);
    bool ok = false;
    const double v = s.toDouble(&ok);
    return ok ? v : fallback;
}

std::optional<ReferenceRun> readReferenceRun(const QString &jsonPath, QString *error)
{
    QFile f(jsonPath);
    if (!f.open(QIODevice::ReadOnly))
    {
        if (error) *error = f.errorString();
        return std::nullopt;
    }
    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (!doc.isObject())
    {
        if (error) *error = pe.errorString();
        return std::nullopt;
    }
    const QJsonObject o = doc.object();
    ReferenceRun run;
    run.job = o[u"job"_s].toString();
    run.stale = o[u"stale"_s].toBool(false);
    const QJsonArray pts = o[u"points"_s].toArray();
    run.points.reserve(static_cast<std::size_t>(pts.size()));
    for (const QJsonValue &v : pts)
    {
        const QJsonObject p = v.toObject();
        run.points.push_back({.x_mm = p[u"x"_s].toDouble(),
                              .z = p[u"z"_s].toDouble(),
                              .p_bar = p[u"p"_s].toDouble(),
                              .v_ms = p[u"v"_s].toDouble(),
                              .e_J = p[u"e"_s].toDouble(),
                              .t_ms = p[u"t"_s].toDouble()});
    }
    run.summary = o[u"summary"_s].toObject().toVariantMap();
    return run;
}

}  // namespace csr
