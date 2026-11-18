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

#include "physicalpowders.h"

#include "lazyfile.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QStringList>

#include <algorithm>
#include <array>
#include <cmath>
#include <map>

using namespace Qt::StringLiterals;

namespace csr
{

namespace
{

struct PowderEntry
{
    std::array<double, 4> record;  // Ba, a0, z1, z2 of the record the values were fitted for
    PhysicalBurn burn;
};

using Powders = std::map<QString, PowderEntry>;  // key: keyOf()

struct CaliberScales
{
    std::map<QString, double> scales;  // key: cipname, lower case
    double autoScale{1.0}, rimmedScale{1.0};
    double rimmedFrom{1.045};  // rim / head diameter from which a cartridge counts as rimmed
};

QString keyOf(const QString &maker, const QString &name)
{
    // The original DB writes "RS 40", the project data "RS40": spaces do not count.
    QString key = (maker.trimmed() + u' ' + name.trimmed()).toLower();
    key.remove(u' ');
    return key;
}

[[nodiscard]] bool same(double a, double b) noexcept
{
    return std::abs(a - b) <= 1e-6 * std::max({1.0, std::abs(a), std::abs(b)});
}

Powders loadPowders(const QString &path)
{
    Powders entries;
    for (const auto &item : readJsonObject(path, "Physical burn values").value(u"propellants"_s).toArray())
    {
        const QJsonObject o = item.toObject();
        const QJsonObject r = o.value(u"record"_s).toObject();
        const QJsonObject grain = o.value(u"grain"_s).toObject();
        entries[keyOf(o.value(u"mname"_s).toString(), o.value(u"pname"_s).toString())] = {
            .record = {r.value(u"Ba"_s).toDouble(), r.value(u"a0"_s).toDouble(), r.value(u"z1"_s).toDouble(), r.value(u"z2"_s).toDouble()},
            .burn = {.Ba = o.value(u"Ba"_s).toDouble(),
                     .a0 = o.value(u"a0"_s).toDouble(),
                     .z1 = o.value(u"z1"_s).toDouble(),
                     .z2 = o.value(u"z2"_s).toDouble(),
                     .grain = grainShapeFromName(grain.value(u"shape"_s).toString()),
                     .grainLd = grain.value(u"ld"_s).toDouble(),
                     .grainHole = grain.value(u"hole"_s).toDouble(),
                     .grainAmplitude = grain.value(u"amplitude"_s).toDouble(1.0)}};
    }
    LOG_Debug() << "Physical burn values:" << entries.size() << "powders from" << path;
    return entries;
}

CaliberScales loadCaliberScales(const QString &path)
{
    CaliberScales c;
    const QJsonObject o = readJsonObject(path, "Caliber burn factors");
    c.autoScale = o.value(u"autoScale"_s).toDouble(1.0);
    c.rimmedScale = o.value(u"rimmedScale"_s).toDouble(1.0);
    c.rimmedFrom = o.value(u"rimmedFrom"_s).toDouble(c.rimmedFrom);
    for (const auto &item : o.value(u"calibers"_s).toArray())
        c.scales[item.toObject().value(u"cipname"_s).toString().toLower()] = item.toObject().value(u"burnScale"_s).toDouble(1.0);
    return c;
}

LazyFile<Powders> &powders()
{
    static LazyFile<Powders> file(u":/data/physical/propellants.json"_s, loadPowders);
    return file;
}

LazyFile<CaliberScales> &caliberScales()
{
    static LazyFile<CaliberScales> file(u":/data/physical/calibers.json"_s, loadCaliberScales);
    return file;
}

QString refText(double Ba, double a0, double z1, double z2)
{
    return QStringList{QString::number(Ba, 'g', 10), QString::number(a0, 'g', 10), QString::number(z1, 'g', 10),
                       QString::number(z2, 'g', 10)}
        .join(u' ');
}

}  // namespace

const char *grainShapeName(GrainShape shape) noexcept
{
    switch (shape)
    {
    case GrainShape::Ball: return "ball";
    case GrainShape::Cylinder: return "cylinder";
    case GrainShape::Fitted: break;
    }
    return "fitted";
}

GrainShape grainShapeFromName(QStringView name) noexcept
{
    if (name == u"ball") return GrainShape::Ball;
    if (name == u"cylinder") return GrainShape::Cylinder;
    return GrainShape::Fitted;
}

void setPhysicalBurnFile(const QString &path)
{
    powders().setPath(path);
    caliberScales().setPath(QFileInfo(path).path() + u"/calibers.json"_s);
}

double physicalCaliberScale(const QString &cipname, double rimRatio)
{
    return caliberScales().use(
        [&](const CaliberScales &c)
        {
            if (const auto it = c.scales.find(cipname.toLower()); it != c.scales.end()) return it->second;
            if (rimRatio <= 0.0) return 1.0;
            return rimRatio >= c.rimmedFrom ? c.rimmedScale : c.autoScale;
        });
}

std::optional<PhysicalBurn> physicalBurnOf(const QVariantMap &propellant)
{
    const std::array<double, 4> values{propellant.value(u"Ba"_s).toDouble(), propellant.value(u"a0"_s).toDouble(),
                                       propellant.value(u"z1"_s).toDouble(), propellant.value(u"z2"_s).toDouble()};
    return powders().use(
        [&](const Powders &p) -> std::optional<PhysicalBurn>
        {
            const auto it = p.find(keyOf(propellant.value(u"mname"_s).toString(), propellant.value(u"pname"_s).toString()));
            if (it == p.end()) return std::nullopt;
            for (std::size_t i = 0; i < values.size(); ++i)
                if (!same(it->second.record[i], values[i])) return std::nullopt;
            return it->second.burn;
        });
}

std::optional<PhysicalBurn> findPhysicalBurn(const QString &maker, const QString &name)
{
    return powders().use(
        [&](const Powders &p) -> std::optional<PhysicalBurn>
        {
            const auto it = p.find(keyOf(maker, name));
            return it == p.end() ? std::nullopt : std::optional{it->second.burn};
        });
}

void writePhysicalBurn(EntryList &propellant, const std::optional<PhysicalBurn> &burn)
{
    if (!burn)
    {
        std::erase_if(propellant.entries(), [](const csr::Entry &e) { return e.name.startsWith(u"phys"); });
        return;
    }
    // like the original's entries: a type and an English description (the property panel's label)
    const auto put = [&](QStringView name, const QString &value, const QString &type, const QString &descr)
    {
        propellant.set(name, value);
        for (csr::Entry &e : propellant.entries())
            if (e.name == name)
            {
                e.type = type;
                e.descr = descr;
            }
    };
    const auto number = [](double v) { return QString::number(v, 'g', 10); };
    put(u"physBa", number(burn->Ba), u"decimal"_s, u"physical model: vivacity coefficient"_s);
    put(u"physA0", number(burn->a0), u"decimal"_s, u"physical model: prog/deg coefficient"_s);
    put(u"physZ1", number(burn->z1), u"decimal"_s, u"physical model: burn-up limit z1"_s);
    put(u"physZ2", number(burn->z2), u"decimal"_s, u"physical model: burn-up limit z2"_s);
    put(u"physGrain", QString::fromLatin1(grainShapeName(burn->grain)), u"string"_s, u"physical model: grain surface shape"_s);
    put(u"physGrainLd", number(burn->grainLd), u"decimal"_s, u"physical model: grain length / diameter"_s);
    put(u"physGrainHole", number(burn->grainHole), u"decimal"_s, u"physical model: grain hole / outer radius"_s);
    put(u"physGrainAmp", number(burn->grainAmplitude), u"decimal"_s, u"physical model: geometric surface amplitude"_s);
    put(u"physRef", refText(propellant.number(u"Ba"), propellant.number(u"a0"), propellant.number(u"z1"), propellant.number(u"z2")),
        u"string"_s, u"burn values the physical ones belong to"_s);
}

std::optional<PhysicalBurn> readPhysicalBurn(const EntryList &propellant)
{
    const PhysicalBurn b{.Ba = propellant.number(u"physBa"),
                         .a0 = propellant.number(u"physA0"),
                         .z1 = propellant.number(u"physZ1"),
                         .z2 = propellant.number(u"physZ2"),
                         .grain = grainShapeFromName(propellant.text(u"physGrain")),
                         .grainLd = propellant.number(u"physGrainLd"),
                         .grainHole = propellant.number(u"physGrainHole"),
                         .grainAmplitude = propellant.number(u"physGrainAmp", 1.0)};
    if (b.Ba <= 0.0 || b.z2 <= b.z1) return std::nullopt;
    const QStringList ref = propellant.text(u"physRef").split(u' ', Qt::SkipEmptyParts);
    if (ref.size() != 4) return std::nullopt;
    const std::array<double, 4> now{propellant.number(u"Ba"), propellant.number(u"a0"), propellant.number(u"z1"), propellant.number(u"z2")};
    for (std::size_t i = 0; i < now.size(); ++i)
        if (!same(ref[static_cast<qsizetype>(i)].toDouble(), now[i])) return std::nullopt;
    return b;
}

bool attachPhysicalBurn(EntryList &propellant)
{
    if (readPhysicalBurn(propellant)) return false;
    QVariantMap record;
    for (const char16_t *key : {u"mname", u"pname"}) record.insert(QString::fromUtf16(key), propellant.text(key));
    for (const char16_t *key : {u"Ba", u"a0", u"z1", u"z2"}) record.insert(QString::fromUtf16(key), propellant.number(key));
    const auto burn = physicalBurnOf(record);
    if (!burn) return false;
    writePhysicalBurn(propellant, burn);
    return true;
}

}  // namespace csr
