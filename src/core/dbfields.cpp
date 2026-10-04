#include "dbfields.h"

#include <QCoreApplication>
#include <QLocale>

#include <algorithm>
#include <array>

using namespace Qt::StringLiterals;

namespace grt::dbfields
{

namespace
{

using units::Quantity;
constexpr double kGrain = 0.06479891;  // g per grain (also cm3 of water per grain H2O)

struct Spec
{
    const char *column;
    const char *label;  // translated in context "grt::dbfields"
    const char *group;
    Quantity quantity;
    double toBase;
    Kind kind;
    const char *unitText;
};

// groups
constexpr const char *kName = QT_TRANSLATE_NOOP("grt::dbfields", "Name");
constexpr const char *kCase = QT_TRANSLATE_NOOP("grt::dbfields", "Case dimensions (CIP)");
constexpr const char *kPressure = QT_TRANSLATE_NOOP("grt::dbfields", "Pressures");
constexpr const char *kChamber = QT_TRANSLATE_NOOP("grt::dbfields", "Chamber and barrel (CIP)");
constexpr const char *kBullet = QT_TRANSLATE_NOOP("grt::dbfields", "Projectile");
constexpr const char *kModel = QT_TRANSLATE_NOOP("grt::dbfields", "Propellant model");
constexpr const char *kRecord = QT_TRANSLATE_NOOP("grt::dbfields", "Record");

constexpr Spec kCommon[] = {
    {"descr", QT_TRANSLATE_NOOP("grt::dbfields", "Description"), kRecord, Quantity::None, 1, Kind::LongText, ""},
    {"cdate", QT_TRANSLATE_NOOP("grt::dbfields", "Created"), kRecord, Quantity::None, 1, Kind::Date, ""},
    {"cby", QT_TRANSLATE_NOOP("grt::dbfields", "Created by"), kRecord, Quantity::None, 1, Kind::Text, ""},
    {"mdate", QT_TRANSLATE_NOOP("grt::dbfields", "Changed"), kRecord, Quantity::None, 1, Kind::Date, ""},
    {"mby", QT_TRANSLATE_NOOP("grt::dbfields", "Changed by"), kRecord, Quantity::None, 1, Kind::Text, ""},
    {"origin", QT_TRANSLATE_NOOP("grt::dbfields", "Source"), kRecord, Quantity::None, 1, Kind::Text, ""},
    {"type", QT_TRANSLATE_NOOP("grt::dbfields", "Type"), kRecord, Quantity::None, 1, Kind::Text, ""},
    {"mode", QT_TRANSLATE_NOOP("grt::dbfields", "Mode"), kRecord, Quantity::None, 1, Kind::Text, ""},
    {"status", QT_TRANSLATE_NOOP("grt::dbfields", "Status"), kRecord, Quantity::None, 1, Kind::Text, ""},
};

constexpr Spec kCaliber[] = {
    {"cipname", QT_TRANSLATE_NOOP("grt::dbfields", "Caliber name"), kName, Quantity::None, 1, Kind::Text, ""},
    {"altname", QT_TRANSLATE_NOOP("grt::dbfields", "Other names"), kName, Quantity::None, 1, Kind::Text, ""},
    {"standard", QT_TRANSLATE_NOOP("grt::dbfields", "Standard"), kName, Quantity::None, 1, Kind::Choice, ""},
    {"ciporigin", QT_TRANSLATE_NOOP("grt::dbfields", "Country of origin"), kName, Quantity::None, 1, Kind::Choice, ""},
    {"ciptype", QT_TRANSLATE_NOOP("grt::dbfields", "Case type"), kName, Quantity::None, 1, Kind::Choice, ""},
    {"cipdate", QT_TRANSLATE_NOOP("grt::dbfields", "Standard date"), kName, Quantity::None, 1, Kind::Date, ""},
    {"ciprevdate", QT_TRANSLATE_NOOP("grt::dbfields", "Revision date"), kName, Quantity::None, 1, Kind::Date, ""},
    {"cippdf", QT_TRANSLATE_NOOP("grt::dbfields", "Data sheet (link)"), kName, Quantity::None, 1, Kind::Text, ""},
    {"L3", QT_TRANSLATE_NOOP("grt::dbfields", "Case length (L3)"), kCase, Quantity::Length, 1, Kind::Number, ""},
    {"L6", QT_TRANSLATE_NOOP("grt::dbfields", "Max. cartridge length (L6)"), kCase, Quantity::Length, 1, Kind::Number, ""},
    {"V", QT_TRANSLATE_NOOP("grt::dbfields", "Case capacity (V)"), kCase, Quantity::Volume, kGrain, Kind::Number, ""},
    {"Pmax", QT_TRANSLATE_NOOP("grt::dbfields", "Max. pressure C.I.P. (Pmax)"), kPressure, Quantity::Pressure, 1, Kind::Number, ""},
    {"PmaxSAAMI", QT_TRANSLATE_NOOP("grt::dbfields", "Max. pressure SAAMI (MAP)"), kPressure, Quantity::Pressure, 1, Kind::Number, ""},
    {"PK", "PK", kPressure, Quantity::Pressure, 1, Kind::Number, ""},
    {"PE", "PE", kPressure, Quantity::Pressure, 1, Kind::Number, ""},
    {"EE", "EE", kPressure, Quantity::Pressure, 1, Kind::Number, ""},
    {"M", QT_TRANSLATE_NOOP("grt::dbfields", "Measuring method (M)"), kPressure, Quantity::None, 1, Kind::Text, ""},
    {"c_Z", QT_TRANSLATE_NOOP("grt::dbfields", "Groove diameter (Z)"), kChamber, Quantity::Length, 1, Kind::Number, ""},
    {"c_F", QT_TRANSLATE_NOOP("grt::dbfields", "Bore diameter (F)"), kChamber, Quantity::Length, 1, Kind::Number, ""},
    {"c_Q", QT_TRANSLATE_NOOP("grt::dbfields", "Bore cross-section (Q)"), kChamber, Quantity::Area, 1, Kind::Number, ""},
    {"c_N", QT_TRANSLATE_NOOP("grt::dbfields", "Number of grooves (N)"), kChamber, Quantity::None, 1, Kind::Number, ""},
    {"c_b_", QT_TRANSLATE_NOOP("grt::dbfields", "Groove width (b)"), kChamber, Quantity::Length, 1, Kind::Number, ""},
    {"c_u_", QT_TRANSLATE_NOOP("grt::dbfields", "Twist length (u)"), kChamber, Quantity::Length, 1, Kind::Number, ""},
    {"c_L3", QT_TRANSLATE_NOOP("grt::dbfields", "Chamber length (L3)"), kChamber, Quantity::Length, 1, Kind::Number, ""},
    {"sebert", QT_TRANSLATE_NOOP("grt::dbfields", "Sebert factor"), kChamber, Quantity::None, 1, Kind::Number, ""},
};

constexpr Spec kProjectile[] = {
    {"mname", QT_TRANSLATE_NOOP("grt::dbfields", "Manufacturer"), kName, Quantity::None, 1, Kind::Text, ""},
    {"pname", QT_TRANSLATE_NOOP("grt::dbfields", "Name"), kName, Quantity::None, 1, Kind::Text, ""},
    {"lotid", QT_TRANSLATE_NOOP("grt::dbfields", "Lot"), kName, Quantity::None, 1, Kind::Text, ""},
    {"caliber", QT_TRANSLATE_NOOP("grt::dbfields", "Caliber designation"), kName, Quantity::None, 1, Kind::Text, ""},
    {"gdia", QT_TRANSLATE_NOOP("grt::dbfields", "Diameter"), kBullet, Quantity::Length, 1, Kind::Number, ""},
    {"glen", QT_TRANSLATE_NOOP("grt::dbfields", "Length"), kBullet, Quantity::Length, 1, Kind::Number, ""},
    {"gmass", QT_TRANSLATE_NOOP("grt::dbfields", "Mass"), kBullet, Quantity::BulletMass, kGrain, Kind::Number, ""},
    {"gtailtype", QT_TRANSLATE_NOOP("grt::dbfields", "Base type"), kBullet, Quantity::None, 1, Kind::BaseType, ""},
    {"gtaildiaA", QT_TRANSLATE_NOOP("grt::dbfields", "Base diameter A"), kBullet, Quantity::Length, 1, Kind::Number, ""},
    {"gtaildiaB", QT_TRANSLATE_NOOP("grt::dbfields", "Base diameter B"), kBullet, Quantity::Length, 1, Kind::Number, ""},
    {"gtailh", QT_TRANSLATE_NOOP("grt::dbfields", "Boat-tail / cavity length"), kBullet, Quantity::Length, 1, Kind::Number, ""},
    {"gdepthmax", QT_TRANSLATE_NOOP("grt::dbfields", "Max. seating depth"), kBullet, Quantity::Length, 1, Kind::Number, ""},
    {"gpressure", QT_TRANSLATE_NOOP("grt::dbfields", "Start pressure"), kBullet, Quantity::Pressure, 1, Kind::Number, ""},
    {"gfriction", QT_TRANSLATE_NOOP("grt::dbfields", "Extra friction"), kBullet, Quantity::Pressure, 1, Kind::Number, ""},
    {"g1bc", QT_TRANSLATE_NOOP("grt::dbfields", "G1 ballistic coefficient"), kBullet, Quantity::None, 1, Kind::Number, ""},
    {"g7bc", QT_TRANSLATE_NOOP("grt::dbfields", "G7 ballistic coefficient"), kBullet, Quantity::None, 1, Kind::Number, ""},
    {"gUBCS", "UBCS", kBullet, Quantity::None, 1, Kind::Text, ""},
};

constexpr Spec kPropellant[] = {
    {"mname", QT_TRANSLATE_NOOP("grt::dbfields", "Manufacturer"), kName, Quantity::None, 1, Kind::Text, ""},
    {"pname", QT_TRANSLATE_NOOP("grt::dbfields", "Name"), kName, Quantity::None, 1, Kind::Text, ""},
    {"lotid", QT_TRANSLATE_NOOP("grt::dbfields", "Lot"), kName, Quantity::None, 1, Kind::Text, ""},
    {"Ba", QT_TRANSLATE_NOOP("grt::dbfields", "Burn rate coefficient (Ba)"), kModel, Quantity::None, 1, Kind::Number, ""},
    {"Qex", QT_TRANSLATE_NOOP("grt::dbfields", "Explosion heat (Qex)"), kModel, Quantity::None, 1, Kind::Number, "kJ/kg"},
    {"k", QT_TRANSLATE_NOOP("grt::dbfields", "Ratio of specific heats (k)"), kModel, Quantity::None, 1, Kind::Number, ""},
    {"eta", QT_TRANSLATE_NOOP("grt::dbfields", "Covolume factor (eta)"), kModel, Quantity::None, 1, Kind::Number, ""},
    {"a0", QT_TRANSLATE_NOOP("grt::dbfields", "Form factor a0"), kModel, Quantity::None, 1, Kind::Number, ""},
    {"a1", QT_TRANSLATE_NOOP("grt::dbfields", "Form factor a1"), kModel, Quantity::None, 1, Kind::Number, ""},
    {"z1", QT_TRANSLATE_NOOP("grt::dbfields", "Transition limit z1"), kModel, Quantity::None, 1, Kind::Number, ""},
    {"z2", QT_TRANSLATE_NOOP("grt::dbfields", "Transition limit z2"), kModel, Quantity::None, 1, Kind::Number, ""},
    {"pc", QT_TRANSLATE_NOOP("grt::dbfields", "Material density"), kModel, Quantity::Density, 1, Kind::Number, ""},
    {"pcd", QT_TRANSLATE_NOOP("grt::dbfields", "Bulk density"), kModel, Quantity::Density, 1, Kind::Number, ""},
    {"pt", QT_TRANSLATE_NOOP("grt::dbfields", "Reference temperature"), kModel, Quantity::Temperature, 1, Kind::Number, ""},
    {"tcc", QT_TRANSLATE_NOOP("grt::dbfields", "Temperature coefficient, cold"), kModel, Quantity::None, 1, Kind::Number, ""},
    {"tch", QT_TRANSLATE_NOOP("grt::dbfields", "Temperature coefficient, hot"), kModel, Quantity::None, 1, Kind::Number, ""},
    {"Br", QT_TRANSLATE_NOOP("grt::dbfields", "Brisance factor (Br)"), kModel, Quantity::None, 1, Kind::Number, ""},
    {"Bp", QT_TRANSLATE_NOOP("grt::dbfields", "Progressivity factor (Bp)"), kModel, Quantity::None, 1, Kind::Number, ""},
    {"Brp", QT_TRANSLATE_NOOP("grt::dbfields", "Combined factor (Brp)"), kModel, Quantity::None, 1, Kind::Number, ""},
    {"Qlty", QT_TRANSLATE_NOOP("grt::dbfields", "Model quality"), kModel, Quantity::None, 1, Kind::Number, ""},
};

QString tr(const char *text)
{
    return QCoreApplication::translate("grt::dbfields", text);
}

std::span<const Spec> specsOf(Table t)
{
    switch (t)
    {
    case Table::Caliber: return kCaliber;
    case Table::Projectile: return kProjectile;
    case Table::Propellant: return kPropellant;
    }
    return {};
}

const Spec *find(std::span<const Spec> specs, const QString &column)
{
    for (const Spec &s : specs)
        if (column == QLatin1String(s.column)) return &s;
    return nullptr;
}

// the group for columns without a description
const char *otherGroup(Table t, const QString &column)
{
    if (t == Table::Caliber) return column.startsWith(u"c_"_s) ? kChamber : kCase;
    if (t == Table::Propellant) return kModel;
    return kBullet;
}

}  // namespace

Field describe(Table t, const QString &column, const QString &sqlType)
{
    Field f;
    f.column = column;
    if (column == u"id" || column == u"geloescht")
    {
        f.kind = Kind::Hidden;
        return f;
    }
    const Spec *s = find(specsOf(t), column);
    if (!s) s = find(kCommon, column);
    if (s)
    {
        f.label = tr(s->label);
        f.group = tr(s->group);
        f.quantity = s->quantity;
        f.toBase = s->toBase;
        f.kind = s->kind;
        f.unitText = QString::fromUtf8(s->unitText);
    }
    else
    {
        f.label = column;
        f.group = tr(otherGroup(t, column));
        const QString type = sqlType.toUpper();
        f.kind = type == u"FLOAT" || type == u"INTEGER" ? Kind::Number : Kind::Text;
        // the remaining dimensions of the CIP drawings are lengths in mm (angles, counts and forces are not)
        static const QStringList notLengths{u"f"_s, u"c_alpha"_s, u"c_alpha1"_s, u"c_delta"_s, u"c_Fe"_s, u"c_Fe_proof"_s, u"c_Fe_auto"_s};
        if (t == Table::Caliber && f.kind == Kind::Number && !notLengths.contains(column)) f.quantity = Quantity::Length;
        if (column.startsWith(u"image"_s))
        {
            f.group = tr(kRecord);
            f.readOnly = true;
        }
    }
    // set by the program: the date of the last change, the burn factors (derived from the model values)
    f.readOnly = f.readOnly || column == u"mdate" || (t == Table::Propellant && (column == u"Br" || column == u"Bp" || column == u"Brp"));
    f.required = column == u"cipname" || column == u"pname";
    // delta, beta, alpha and M are numbers that the original database stores as text
    f.alignRight = f.kind == Kind::Number ||
                   (t == Table::Caliber && (column == u"delta" || column == u"beta" || column == u"alpha" || column == u"M"));
    return f;
}

QStringList groups(Table t)
{
    switch (t)
    {
    case Table::Caliber: return {tr(kName), tr(kCase), tr(kPressure), tr(kChamber), tr(kRecord)};
    case Table::Projectile: return {tr(kName), tr(kBullet), tr(kRecord)};
    case Table::Propellant: return {tr(kName), tr(kModel), tr(kRecord)};
    }
    return {};
}

std::vector<std::pair<QString, QString>> countries()
{
    std::vector<std::pair<QString, QString>> out;
    for (int i = 1; i <= static_cast<int>(QLocale::LastTerritory); ++i)
    {
        const auto territory = static_cast<QLocale::Territory>(i);
        const QString code = QLocale::territoryToCode(territory);
        if (code.size() != 2 || code.front().isDigit()) continue;  // regions like "419" are no countries
        if (std::ranges::any_of(out, [&](const auto &c) { return c.first == code; })) continue;
        out.emplace_back(code, QLocale::territoryToString(territory));
    }
    out.emplace_back(u"SU"_s, tr(QT_TRANSLATE_NOOP("grt::dbfields", "Soviet Union")));
    out.emplace_back(u"CS"_s, tr(QT_TRANSLATE_NOOP("grt::dbfields", "Czechoslovakia")));
    out.emplace_back(u"RSA"_s, tr(QT_TRANSLATE_NOOP("grt::dbfields", "South Africa (RSA)")));
    std::ranges::sort(out, {}, &std::pair<QString, QString>::first);
    return out;
}

std::vector<std::pair<QString, QString>> choices(Table t, const QString &column)
{
    if (t != Table::Caliber) return {};
    if (column == u"ciporigin")
    {
        std::vector<std::pair<QString, QString>> out;
        for (const auto &[code, name] : countries()) out.emplace_back(code, code + u" – "_s + name);
        return out;
    }
    if (column == u"standard")
        return {{u"CIP"_s, tr(QT_TRANSLATE_NOOP("grt::dbfields", "CIP (C.I.P., Europe)"))},
                {u"SAAMI"_s, tr(QT_TRANSLATE_NOOP("grt::dbfields", "SAAMI (USA)"))},
                {u"WildCat"_s, tr(QT_TRANSLATE_NOOP("grt::dbfields", "Wildcat (not standardized)"))}};
    if (column == u"ciptype")
        return {{u"1 rimless"_s, tr(QT_TRANSLATE_NOOP("grt::dbfields", "rimless"))},
                {u"2 rimmed"_s, tr(QT_TRANSLATE_NOOP("grt::dbfields", "rimmed"))},
                {u"3 belted"_s, tr(QT_TRANSLATE_NOOP("grt::dbfields", "belted"))},
                {u"3 magnum"_s, tr(QT_TRANSLATE_NOOP("grt::dbfields", "magnum"))},
                {u"4 pistol/revolver"_s, tr(QT_TRANSLATE_NOOP("grt::dbfields", "pistol / revolver"))},
                {u"5 rimfire"_s, tr(QT_TRANSLATE_NOOP("grt::dbfields", "rimfire"))},
                {u"7 shotshell"_s, tr(QT_TRANSLATE_NOOP("grt::dbfields", "shotshell"))},
                {u"10 other"_s, tr(QT_TRANSLATE_NOOP("grt::dbfields", "other"))}};
    return {};
}

QString recordTitle(Table t, const QVariantMap &r)
{
    if (t == Table::Caliber) return r.value(u"cipname"_s).toString();
    QString s = (r.value(u"mname"_s).toString() + u' ' + r.value(u"pname"_s).toString()).trimmed();
    if (const QString lot = r.value(u"lotid"_s).toString(); !lot.isEmpty()) s += u" ("_s + lot + u')';
    return s;
}

}  // namespace grt::dbfields
