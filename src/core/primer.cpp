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

#include "log.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <map>
#include <mutex>

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

struct Store
{
    std::mutex mutex;
    QString path{u":/data/physical/primers.json"_s};
    bool loaded{};
    std::map<QString, double> start;  // bar, per type
    double volumeExponent{};          // start pressure ~ (V / referenceVolume)^-volumeExponent (0: independent of the case)
    double referenceVolume_cm3{3.0};
};

Store &store()
{
    static Store s;
    return s;
}

void loadLocked(Store &s)
{
    s.loaded = true;
    s.start.clear();
    s.volumeExponent = 0.0;
    QFile f(s.path);
    if (!f.open(QIODevice::ReadOnly))
    {
        LOG_Warn() << "Primer start pressures: cannot read" << s.path << "- every primer starts at" << kReferenceStartPressure_bar << "bar";
        return;
    }
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    const QJsonObject types = o.value(u"primers"_s).toObject();
    for (auto it = types.begin(); it != types.end(); ++it)
        if (const double v = it.value().toDouble(); std::isfinite(v) && v > 0.0) s.start[it.key()] = v;
    s.volumeExponent = o.value(u"volumeExponent"_s).toDouble(0.0);
    s.referenceVolume_cm3 = o.value(u"referenceVolume_cm3"_s).toDouble(3.0);
    if (!std::isfinite(s.volumeExponent)) s.volumeExponent = 0.0;
    if (!(s.referenceVolume_cm3 > 0.0)) s.referenceVolume_cm3 = 3.0;
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
    Store &s = store();
    const std::scoped_lock lock(s.mutex);
    if (!s.loaded) loadLocked(s);
    const auto it = s.start.find(code);
    if (it == s.start.end()) return kReferenceStartPressure_bar;
    if (s.volumeExponent == 0.0 || !(caseVolume_cm3 > 0.0)) return it->second;
    return it->second * std::pow(caseVolume_cm3 / s.referenceVolume_cm3, -s.volumeExponent);
}

void setPrimerFile(const QString &path)
{
    Store &s = store();
    const std::scoped_lock lock(s.mutex);
    s.path = path;
    s.loaded = false;
}

}  // namespace csr
