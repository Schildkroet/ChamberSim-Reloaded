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

#include "loadbuilder.h"
#include "physicalpowders.h"
#include "log.h"

#include "loadspec.h"
#include "numeric.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <span>

using namespace Qt::StringLiterals;

namespace csr
{

namespace
{

double num(const QVariantMap &m, const char *key, double fallback = 0.0)
{
    const QVariant v = m.value(QLatin1String(key));
    bool ok = false;
    const double d = v.toDouble(&ok);
    return ok && std::isfinite(d) ? d : fallback;
}

QString str(const QVariantMap &m, const char *key)
{
    return m.value(QLatin1String(key)).toString();
}

QString fmt(double v)
{
    return QString::number(v, 'g', 15);
}

QString textOf(const QVariant &v)
{
    if (v.typeId() == QMetaType::Double || v.typeId() == QMetaType::Float) return fmt(v.toDouble());
    return v.toString();
}

void fill(EntryList &list, std::span<const spec::Field> fields, const QVariantMap &values)
{
    for (const spec::Field &f : fields)
    {
        list.entries().push_back({.name = QString::fromLatin1(f.name),
                                  .value = values.value(QString::fromLatin1(f.name)).toString(),
                                  .unit = QString::fromLatin1(f.unit),
                                  .type = QString::fromLatin1(f.type),
                                  .descr = QString::fromLatin1(f.descr)});
    }
}

}  // namespace

double caseVolume_cm3(const QVariantMap &caliber)
{
    return num(caliber, "V") * kGrainToGram;
}

double defaultBarrelLength_mm(const QVariantMap &caliber)
{
    const QString type = str(caliber, "ciptype").toLower();
    if (type.contains(u"pistol") || type.contains(u"revolver")) return 150.0;
    if (type.contains(u"rimfire")) return 400.0;
    return 600.0;
}

double defaultSeatingDepth_mm(const QVariantMap &caliber, const QVariantMap &projectile)
{
    const double glen = num(projectile, "glen");
    const double depth = num(caliber, "L3") + glen - num(caliber, "L6");
    return std::clamp(depth, 2.0, std::max(0.8 * glen, 2.0));
}

double defaultCharge_g(const QVariantMap &caliber, const QVariantMap &propellant)
{
    return 0.70 * num(propellant, "pcd") * 1e-3 * caseVolume_cm3(caliber);
}

namespace
{

QString caliberDisplayName(const QVariantMap &caliber)
{
    QString name = str(caliber, "cipname");
    if (const QString alt = str(caliber, "altname"); !alt.isEmpty()) name += u" ("_s + alt + u')';
    return name;
}

QString projectileDisplayName(const QVariantMap &projectile)
{
    QString bulletCaliber = str(projectile, "caliber");
    if (bulletCaliber.startsWith(u'.')) bulletCaliber.prepend(u'0');  // ".323" is shown as "0.323"
    return u"%1, %2, %3, %4 grain"_s.arg(str(projectile, "mname"), str(projectile, "pname"), bulletCaliber,
                                         QString::number(num(projectile, "gmass"), 'f', 2));
}

double startPressureOf(const QVariantMap &projectile)
{
    return num(projectile, "gpressure") > 0.0 ? num(projectile, "gpressure") : 250.0;
}

QVariantMap caliberFields(const QVariantMap &caliber)
{
    const QString method = str(caliber, "standard") == u"SAAMI" ? u"Piezo SAAMI"_s : u"Piezo CIP"_s;
    return {{u"CaliberName"_s, caliberDisplayName(caliber)},
            {u"caselen"_s, fmt(num(caliber, "L3"))},
            {u"casevol"_s, fmt(caseVolume_cm3(caliber))},
            {u"Dz"_s, fmt(num(caliber, "c_Z"))},
            {u"Aeff"_s, fmt(num(caliber, "c_Q"))},
            {u"pMaxZul"_s, fmt(num(caliber, "Pmax"))},
            {u"method"_s, method},
            // the project's records have no Sebert factor (the database then gives 0): 0 is never right, the original's records use 0.4-0.9
            {u"sebert"_s, fmt(num(caliber, "sebert") > 0.0 ? num(caliber, "sebert") : 0.5)},
            {u"shotshell"_s, u"false"_s}};
}

void fillCaliberFile(LoadFile &lf, const QVariantMap &caliber, const QStringList &caliberColumns)
{
    lf.caliberFile.entries().clear();
    const QStringList columns = !caliberColumns.isEmpty() ? caliberColumns : QStringList(caliber.keys());
    for (const QString &col : columns)
    {
        // the block is the original's record: columns this project added to the table (PmaxSAAMI) stay out
        if (col == u"id" || col == u"geloescht" || col == u"PmaxSAAMI" || !caliber.contains(col)) continue;
        lf.caliberFile.entries().push_back({.name = col, .value = textOf(caliber.value(col)), .unit = {}, .type = {}, .descr = {}});
    }
}

QVariantMap projectileFields(const QVariantMap &projectile)
{
    return {{u"ProjectileName"_s, projectileDisplayName(projectile)},
            {u"mname"_s, str(projectile, "mname")},
            {u"pname"_s, str(projectile, "pname")},
            {u"lotid"_s, str(projectile, "lotid")},
            {u"Dbul"_s, fmt(num(projectile, "gdia"))},
            {u"mp"_s, fmt(num(projectile, "gmass") * kGrainToGram)},
            {u"glen"_s, fmt(num(projectile, "glen"))},
            {u"gpressure"_s, fmt(startPressureOf(projectile))},
            {u"gtailh"_s, fmt(num(projectile, "gtailh"))},
            {u"gTailDiaA"_s, fmt(num(projectile, "gtaildiaA"))},
            {u"gTailDiaB"_s, fmt(num(projectile, "gtaildiaB"))},
            {u"gTailType"_s, QString::number(toIntClamped(num(projectile, "gtailtype"), 0, 2))},
            {u"g1bc"_s, fmt(num(projectile, "g1bc"))},
            {u"g7bc"_s, fmt(num(projectile, "g7bc"))},
            {u"gUBCS"_s, str(projectile, "gUBCS")},
            {u"gfriction"_s, fmt(num(projectile, "gfriction"))},
            {u"mode"_s, str(projectile, "mode")}};
}

QVariantMap propellantFields(const QVariantMap &propellant)
{
    return {{u"mname"_s, str(propellant, "mname")},
            {u"pname"_s, str(propellant, "pname")},
            {u"lotid"_s, str(propellant, "lotid")},
            {u"Ba"_s, fmt(num(propellant, "Ba"))},
            {u"Qex"_s, fmt(num(propellant, "Qex"))},
            {u"k"_s, fmt(num(propellant, "k"))},
            {u"a0"_s, fmt(num(propellant, "a0"))},
            {u"a1"_s, fmt(num(propellant, "a1"))},
            {u"z1"_s, fmt(num(propellant, "z1"))},
            {u"z2"_s, fmt(num(propellant, "z2"))},
            {u"eta"_s, fmt(num(propellant, "eta", 1.0))},
            {u"pc"_s, fmt(num(propellant, "pc"))},
            {u"pcd"_s, fmt(num(propellant, "pcd"))},
            {u"tcc"_s, fmt(num(propellant, "tcc"))},
            {u"tch"_s, fmt(num(propellant, "tch"))},
            {u"imageUuid"_s, str(propellant, "imageUuid")},
            {u"imageDetailUuid"_s, str(propellant, "imageDetailUuid")}};
}

void setFields(EntryList &list, const QVariantMap &values)
{
    for (auto it = values.cbegin(); it != values.cend(); ++it) list.set(it.key(), it.value().toString());
}

void refreshTitleAndLength(LoadFile &lf)
{
    lf.title = u"%1 %2 %3 %4"_s.arg(lf.caliber.text(u"CaliberName"), lf.projectile.text(u"ProjectileName"), lf.propellant.text(u"mname"),
                                    lf.propellant.text(u"pname"));
    lf.caliber.set(u"oal", lf.caliber.number(u"caselen") + lf.projectile.number(u"glen") - lf.projectile.number(u"gdepth"));
    lf.caliber.set(u"ps", lf.projectile.number(u"gpressure", 250.0));
}

}  // namespace

bool completeTail(EntryList &projectile, double previousDiameter)
{
    const int type = toIntClamped(projectile.number(u"gTailType"), 0, 2);
    const double D = projectile.number(u"Dbul");
    if (type == 0 || D <= 0.0) return false;
    struct Shape
    {
        double length, diaA, diaB;  // times the bullet diameter
    };
    const Shape s = type == 2 ? Shape{0.53, 1.0, 0.835} : Shape{0.055, 0.86, 0.29};
    bool changed = false;
    // still the estimates for the previous diameter: estimate them for the new one
    const auto isEstimate = [&](QStringView key, double factor)
    {
        const double v = projectile.number(key);
        return v > 0.0 && std::abs(v - factor * previousDiameter) <= 1e-6 * std::max(1.0, v);
    };
    if (previousDiameter > 0.0 && previousDiameter != D && isEstimate(u"gtailh", s.length) && isEstimate(u"gTailDiaA", s.diaA) &&
        isEstimate(u"gTailDiaB", s.diaB))
    {
        projectile.set(u"gtailh", fmt(s.length * D));
        projectile.set(u"gTailDiaA", fmt(s.diaA * D));
        projectile.set(u"gTailDiaB", fmt(s.diaB * D));
        changed = true;
    }
    const auto fillIfMissing = [&](QStringView key, double factor)
    {
        if (projectile.number(key) > 0.0) return;
        projectile.set(key, fmt(factor * D));
        changed = true;
    };
    if (projectile.number(u"gtailh") <= 0.0)
    {
        fillIfMissing(u"gtailh", s.length);
        fillIfMissing(u"gTailDiaA", s.diaA);
        fillIfMissing(u"gTailDiaB", s.diaB);
    }
    else if (type == 2)  // a boat tail with a length but no base diameter
    {
        fillIfMissing(u"gTailDiaA", s.diaA);
        fillIfMissing(u"gTailDiaB", s.diaB);
    }
    if (changed) LOG_Debug() << "Load: estimated tail dimensions for base type" << type;
    return changed;
}

bool followCaseLength(EntryList &caliber, EntryList &projectile, double previousLength)
{
    const double length = caliber.number(u"caselen");
    const double delta = length - previousLength;
    if (previousLength <= 0.0 || length <= 0.0 || delta == 0.0) return false;
    const double D = projectile.number(u"Dbul");
    const double area_mm2 = D > 0.0 ? std::numbers::pi / 4.0 * D * D : caliber.number(u"Aeff");
    const double volume = caliber.number(u"casevol");
    if (volume > 0.0) caliber.set(u"casevol", fmt(std::max(volume + area_mm2 * delta * 1e-3, 0.01)));  // mm3 -> cm3
    const double depth = std::max(projectile.number(u"gdepth") + delta, 0.0);
    projectile.set(u"gdepth", fmt(depth));
    caliber.set(u"oal", fmt(length + projectile.number(u"glen") - depth));
    LOG_Debug() << "Load: case length" << previousLength << "->" << length << "mm, seating depth and case volume follow";
    return true;
}

void applyCaliber(LoadFile &lf, const QVariantMap &caliber, const QStringList &caliberColumns, bool resetDependent,
                  const QVariantMap *projectile, const QVariantMap *propellant)
{
    LOG_Debug() << "Load: caliber replaced, reset dependent values" << resetDependent;
    setFields(lf.caliber, caliberFields(caliber));
    fillCaliberFile(lf, caliber, caliberColumns);
    if (resetDependent)
    {
        lf.gun.set(u"xe", defaultBarrelLength_mm(caliber));
        if (projectile) lf.projectile.set(u"gdepth", defaultSeatingDepth_mm(caliber, *projectile));
        if (propellant) lf.propellant.set(u"mc", defaultCharge_g(caliber, *propellant));
    }
    refreshTitleAndLength(lf);
}

void applyProjectile(LoadFile &lf, const QVariantMap &projectile, bool resetDependent, const QVariantMap *caliber)
{
    LOG_Debug() << "Load: projectile replaced, reset dependent values" << resetDependent;
    setFields(lf.projectile, projectileFields(projectile));
    completeTail(lf.projectile);
    if (resetDependent && caliber) lf.projectile.set(u"gdepth", defaultSeatingDepth_mm(*caliber, projectile));
    else if (lf.projectile.number(u"gdepth") <= 0.0 && caliber) lf.projectile.set(u"gdepth", defaultSeatingDepth_mm(*caliber, projectile));
    refreshTitleAndLength(lf);
}

QStringList propellantDifferences(const EntryList &propellant, const QVariantMap &record)
{
    QStringList differing;
    for (const QString &key :
         {u"Ba"_s, u"Qex"_s, u"k"_s, u"a0"_s, u"a1"_s, u"z1"_s, u"z2"_s, u"eta"_s, u"pc"_s, u"pcd"_s, u"tcc"_s, u"tch"_s})
    {
        const double fallback = key == u"eta" ? 1.0 : 0.0;
        const double a = propellant.number(key, fallback);
        const double b = num(record, key.toLatin1().constData(), fallback);
        if (std::abs(a - b) > 1e-6 * std::max({1.0, std::abs(a), std::abs(b)})) differing << key;
    }
    return differing;
}

void applyPropellant(LoadFile &lf, const QVariantMap &propellant, bool resetDependent, const QVariantMap *caliber)
{
    LOG_Debug() << "Load: propellant replaced, reset dependent values" << resetDependent;
    setFields(lf.propellant, propellantFields(propellant));
    writePhysicalBurn(lf.propellant, physicalBurnOf(propellant));
    if (resetDependent && caliber) lf.propellant.set(u"mc", defaultCharge_g(*caliber, propellant));
    refreshTitleAndLength(lf);
}

LoadFile buildLoad(const QVariantMap &caliber, const QVariantMap &projectile, const QVariantMap &propellant, LoadParams p,
                   const QStringList &caliberColumns)
{
    if (p.charge_g <= 0.0) p.charge_g = defaultCharge_g(caliber, propellant);
    if (p.barrelLength_mm <= 0.0) p.barrelLength_mm = defaultBarrelLength_mm(caliber);
    if (p.seatingDepth_mm <= 0.0) p.seatingDepth_mm = defaultSeatingDepth_mm(caliber, projectile);

    LoadFile lf;
    lf.appVersion = u"2021.2030-NIGHTLY"_s;
    lf.config.entries().push_back(
        {.name = u"CurveFocusIdx"_s, .value = u"0"_s, .unit = {}, .type = u"integer"_s, .descr = u"selected ladder curve for display"_s});

    QVariantMap cal = caliberFields(caliber);
    cal.insert(u"oal"_s, fmt(num(caliber, "L3") + num(projectile, "glen") - p.seatingDepth_mm));
    cal.insert(u"ps"_s, fmt(startPressureOf(projectile)));
    fill(lf.caliber, spec::kCaliber, cal);
    fillCaliberFile(lf, caliber, caliberColumns);

    fill(lf.gun, spec::kGun,
         {{u"GunName"_s, QString()},
          {u"xe"_s, fmt(p.barrelLength_mm)},
          {u"twistlen"_s, fmt(p.twistLength_mm)},
          {u"mgun"_s, fmt(p.gunWeight_kg)},
          {u"gjump"_s, u"0"_s},
          {u"gjumpUnlock"_s, u"false"_s}});

    QVariantMap prj = projectileFields(projectile);
    prj.insert(u"gdepth"_s, fmt(p.seatingDepth_mm));
    prj.insert(u"gfrictionUnlock"_s, u"false"_s);
    fill(lf.projectile, spec::kProjectile, prj);
    completeTail(lf.projectile);

    QVariantMap pro = propellantFields(propellant);
    pro.insert(u"pt"_s, fmt(p.powderTemperature_C));
    pro.insert(u"mc"_s, fmt(p.charge_g));
    pro.insert(u"laddercnt"_s, u"1"_s);
    pro.insert(u"laddermc"_s, fmt(kGrainToGram));
    fill(lf.propellant, spec::kPropellant, pro);
    writePhysicalBurn(lf.propellant, physicalBurnOf(propellant));

    refreshTitleAndLength(lf);
    return lf;
}

}  // namespace csr
