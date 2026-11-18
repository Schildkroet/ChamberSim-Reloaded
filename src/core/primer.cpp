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

#include "primer.h"

#include "lazyfile.h"

#include <QCoreApplication>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <map>

using namespace Qt::StringLiterals;

namespace csr
{

namespace
{

// the names for lupdate (kPrimerTypes holds the same English text)
[[maybe_unused]] constexpr const char *kNames[] = {
    QT_TRANSLATE_NOOP("csr::primer", "Small Pistol"), QT_TRANSLATE_NOOP("csr::primer", "Small Pistol Magnum"),
    QT_TRANSLATE_NOOP("csr::primer", "Large Pistol"), QT_TRANSLATE_NOOP("csr::primer", "Large Pistol Magnum"),
    QT_TRANSLATE_NOOP("csr::primer", "Small Rifle"),  QT_TRANSLATE_NOOP("csr::primer", "Small Rifle Magnum"),
    QT_TRANSLATE_NOOP("csr::primer", "Large Rifle"),  QT_TRANSLATE_NOOP("csr::primer", "Large Rifle Magnum"),
    QT_TRANSLATE_NOOP("csr::primer", ".50 BMG"),
};

struct Primers
{
    std::map<QString, double> start;  // bar, per type
    double volumeExponent{};          // start pressure x (V / referenceVolume)^-volumeExponent
    double referenceVolume_cm3{3.0};
};

Primers load(const QString &path)
{
    Primers p;
    const QJsonObject o = readJsonObject(path, "Primer start pressures");
    const QJsonObject types = o.value(u"primers"_s).toObject();
    for (auto it = types.begin(); it != types.end(); ++it)
        if (const double v = it.value().toDouble(); std::isfinite(v) && v > 0.0) p.start[it.key()] = v;
    p.volumeExponent = o.value(u"volumeExponent"_s).toDouble(0.0);
    p.referenceVolume_cm3 = o.value(u"referenceVolume_cm3"_s).toDouble(3.0);
    if (!std::isfinite(p.volumeExponent)) p.volumeExponent = 0.0;
    if (!(p.referenceVolume_cm3 > 0.0)) p.referenceVolume_cm3 = 3.0;
    return p;
}

LazyFile<Primers> &primers()
{
    static LazyFile<Primers> file(u":/data/physical/primers.json"_s, load);
    return file;
}

double number(const QVariantMap &m, const char *key)
{
    bool ok = false;
    const double v = m.value(QLatin1String(key)).toDouble(&ok);
    return ok && std::isfinite(v) ? v : 0.0;
}

}  // namespace

bool isPrimerCode(const QString &code)
{
    return std::ranges::any_of(kPrimerTypes, [&](const PrimerType &t) { return code == QLatin1String(t.code); });
}

QString primerName(const QString &code)
{
    for (const PrimerType &t : kPrimerTypes)
        if (code == QLatin1String(t.code)) return QCoreApplication::translate("csr::primer", t.name);
    return code;
}

QString primerOfCaliber(const QVariantMap &caliber)
{
    if (const QString own = caliber.value(u"primer"_s).toString().trimmed(); isPrimerCode(own)) return own;
    const QString type = caliber.value(u"ciptype"_s).toString();
    if (type.startsWith(u'5') || type.startsWith(u'7')) return {};  // rimfire, shotshell
    static const QRegularExpression bmg(uR"(\b(12[.,]7 x (99|108)|50 BMG|14[.,]5 x 114))"_s, QRegularExpression::CaseInsensitiveOption);
    if (bmg.match(caliber.value(u"cipname"_s).toString()).hasMatch()) return u"BMG"_s;
    const bool pistol = caliber.value(u"cippdf"_s).toString().contains(u"/tab-iv/"_s) || type.startsWith(u'4');
    const double p1 = number(caliber, "P1") > 0.0 ? number(caliber, "P1") : number(caliber, "R1");
    const bool small = p1 > 0.0 && p1 < (pistol ? 11.0 : 11.25);
    QString code = QString(small ? u'S' : u'L') + (pistol ? u'P' : u'R');
    if (type.startsWith(u'3')) code += u'M';
    return code;
}

double primerStartPressure_bar(const QString &code, double caseVolume_cm3)
{
    return primers().use(
        [&](const Primers &p)
        {
            const auto it = p.start.find(code);
            if (it == p.start.end()) return kReferenceStartPressure_bar;
            if (p.volumeExponent == 0.0 || !(caseVolume_cm3 > 0.0)) return it->second;
            return it->second * std::pow(caseVolume_cm3 / p.referenceVolume_cm3, -p.volumeExponent);
        });
}

void setPrimerFile(const QString &path)
{
    primers().setPath(path);
}

}  // namespace csr
