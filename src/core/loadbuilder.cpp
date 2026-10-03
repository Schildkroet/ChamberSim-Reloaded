#include "loadbuilder.h"
#include "physicalpowders.h"
#include "log.h"

#include "loadspec.h"
#include "numeric.h"

#include <algorithm>
#include <cmath>
#include <span>

using namespace Qt::StringLiterals;

namespace grt
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
            {u"sebert"_s, fmt(num(caliber, "sebert", 0.5))},
            {u"shotshell"_s, u"false"_s}};
}

void fillCaliberFile(LoadFile &lf, const QVariantMap &caliber, const QStringList &caliberColumns)
{
    lf.caliberFile.entries().clear();
    const QStringList columns = !caliberColumns.isEmpty() ? caliberColumns : QStringList(caliber.keys());
    for (const QString &col : columns)
    {
        if (col == u"id" || col == u"geloescht" || !caliber.contains(col)) continue;
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
    if (resetDependent && caliber) lf.projectile.set(u"gdepth", defaultSeatingDepth_mm(*caliber, projectile));
    else if (lf.projectile.number(u"gdepth") <= 0.0 && caliber) lf.projectile.set(u"gdepth", defaultSeatingDepth_mm(*caliber, projectile));
    refreshTitleAndLength(lf);
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

}  // namespace grt
