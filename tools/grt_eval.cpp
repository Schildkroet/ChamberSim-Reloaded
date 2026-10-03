// Simulates published load data (CSV of tools/loaddata) with the project's propellant records and an adjustable solver calibration;
// the solver calibration experiments (analysis/calibrate_published.py) call it.
//
//   grt_eval samples.csv [--profile original|published|physical] [--data DIR] [--set name=value]... [--ba "Maker Name=value"]... [--steps n]
//            [--physical FILE] [--phys "Maker Name=Ba,a0,z1,z2"]... [--list-params]
//
// --physical is the file of the physical burn values (default: the project data's); --phys sets those of one powder (fits).
// --profile chooses the starting constants (and with them the model: physical = the own physical model); --set changes single ones.
//
// Prints one line per sample that can be simulated: row (0-based, in the CSV), simulated muzzle velocity in m/s and peak pressure in bar.
// Samples that cannot be simulated (charge too high for the case, ...) are left out. The powder of a row is the record of the
// project data whose "Manufacturer Name" is the CSV column `powder`; --ba replaces its burn rate.

#include "database.h"
#include "loadbuilder.h"
#include "physicalpowders.h"
#include "projectdata.h"
#include "simconfig.h"
#include "solver.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QMap>
#include <QTemporaryDir>
#include <QTextStream>

#include <map>

using namespace Qt::StringLiterals;

namespace
{

// A CSV file with a header line; fields in double quotes may contain commas.
std::vector<QMap<QString, QString>> readCsv(const QString &path)
{
    QFile f(path);
    std::vector<QMap<QString, QString>> rows;
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return rows;
    QStringList header;
    while (!f.atEnd())
    {
        const QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (line.isEmpty()) continue;
        QStringList fields;
        QString cur;
        bool quoted = false;
        for (qsizetype i = 0; i < line.size(); ++i)
        {
            const QChar c = line.at(i);
            if (c == u'"')
            {
                if (quoted && i + 1 < line.size() && line.at(i + 1) == u'"') cur += u'"', ++i;
                else quoted = !quoted;
            }
            else if (c == u',' && !quoted) fields << std::exchange(cur, QString());
            else cur += c;
        }
        fields << cur;
        if (header.isEmpty())
        {
            header = fields;
            continue;
        }
        QMap<QString, QString> row;
        for (qsizetype i = 0; i < header.size() && i < fields.size(); ++i) row.insert(header.at(i), fields.at(i));
        rows.push_back(row);
    }
    return rows;
}

}  // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser cli;
    cli.addHelpOption();
    cli.addPositionalArgument(u"samples"_s, u"CSV file (tools/loaddata)"_s);
    cli.addOptions(
        {{u"data"_s, u"project data directory"_s, u"dir"_s, QStringLiteral(GRT_SOURCE_DIR "/data")},
         {u"set"_s, u"calibration value name=value (repeatable)"_s, u"nv"_s},
         {u"ba"_s, u"burn rate of a powder: \"Maker Name=value\" (repeatable)"_s, u"nv"_s},
         {u"steps"_s, u"solver steps"_s, u"n"_s, u"2000"_s},
         {u"profile"_s, u"starting constants: original, published or physical"_s, u"name"_s, u"original"_s},
         {u"physical"_s, u"physical burn values file"_s, u"file"_s},
         {u"phys"_s, u"physical burn values of a powder: \"Maker Name=Ba,a0,z1,z2\" (repeatable)"_s, u"nv"_s},
         {u"ps-handgun"_s, u"initial gas pressure [bar] of the bullets in pistol and revolver cartridges (C.I.P. table IV)"_s, u"bar"_s},
         {u"list-params"_s, u"print the calibration names and values"_s}});
    cli.process(app);
    QTextStream out(stdout);
    const QString profile = cli.value(u"profile"_s);
    grt::Calibration cal = grt::calibrationFor(profile == u"published"_s  ? grt::CalibrationProfile::Published
                                               : profile == u"physical"_s ? grt::CalibrationProfile::Physical
                                                                          : grt::CalibrationProfile::Original);
    auto named = grt::parameters(cal);
    if (cli.isSet(u"list-params"_s))
    {
        for (const auto &p : named) out << QString::fromLatin1(p.name) << ' ' << *p.value << '\n';
        return 0;
    }
    for (const QString &nv : cli.values(u"set"_s))
    {
        const QString name = nv.section(u'=', 0, 0);
        const auto it = std::ranges::find_if(named, [&](const grt::NamedParameter &p) { return name == QLatin1String(p.name); });
        if (it == named.end())
        {
            qCritical("unknown calibration name %s", qPrintable(name));
            return 2;
        }
        *it->value = nv.section(u'=', 1).toDouble();
    }
    grt::setPhysicalBurnFile(cli.isSet(u"physical"_s) ? cli.value(u"physical"_s) : cli.value(u"data"_s) + u"/physical/propellants.json"_s);
    QMap<QString, grt::PhysicalBurn> phys;
    for (const QString &nv : cli.values(u"phys"_s))
    {
        const QStringList v = nv.section(u'=', 1).split(u',');
        if (v.size() == 4)
            phys.insert(nv.section(u'=', 0, 0).toLower(),
                        {.Ba = v[0].toDouble(), .a0 = v[1].toDouble(), .z1 = v[2].toDouble(), .z2 = v[3].toDouble()});
    }
    // the load of a row: the physical burn values of --phys replace the file's
    const auto withPhys = [&](grt::LoadFile lf, const QString &key)
    {
        if (phys.contains(key)) grt::writePhysicalBurn(lf.propellant, phys.value(key));
        return grt::SimInput::fromLoad(lf);
    };
    QMap<QString, double> ba;
    for (const QString &nv : cli.values(u"ba"_s)) ba.insert(nv.section(u'=', 0, 0).toLower(), nv.section(u'=', 1).toDouble());
    if (cli.positionalArguments().isEmpty()) return 2;

    QTemporaryDir tmp;
    grt::Database db;
    grt::Database::setProjectDataLocation(tmp.path());
    if (!db.open({}, tmp.filePath(u"own.db"_s))) return 2;
    const auto calSchema = db.schema(grt::Table::Caliber);
    QMap<QString, QVariantMap> calibers;
    for (const QVariantMap &r : grt::projectdata::load(cli.value(u"data"_s), grt::Table::Caliber, calSchema).records)
        calibers.insert(r.value(u"cipname"_s).toString(), r);
    QStringList calColumns;
    for (const auto &[name, type] : calSchema) calColumns << name;
    QMap<QString, QVariantMap> powders;
    for (const QVariantMap &r :
         grt::projectdata::load(cli.value(u"data"_s), grt::Table::Propellant, db.schema(grt::Table::Propellant)).records)
        powders.insert((r.value(u"mname"_s).toString() + u' ' + r.value(u"pname"_s).toString()).toLower(), r);

    const int steps = cli.value(u"steps"_s).toInt();
    const auto rows = readCsv(cli.positionalArguments().front());
    // shotshell rows (column shot_g): the CIP dimensions of the hull (bore diameter, area, volume in grain of water) by gauge and length
    struct Gauge
    {
        double bore_mm, area_mm2;
        std::map<int, double> volume_gr;
    };
    static const std::map<int, Gauge> gauges{{12, {18.2, 260.16, {{65, 246}, {70, 266}, {76, 292}, {89, 346}}}},
                                             {16, {16.8, 221.67, {{65, 204}, {70, 221}, {76, 243}}}},
                                             {20, {15.7, 193.59, {{65, 176}, {70, 191}, {76, 210}, {89, 248}}}},
                                             {28, {13.8, 149.57, {{65, 138}, {70, 150}, {76, 165.2}}}},
                                             {410, {10.2, 81.71, {{65, 73}, {70, 80}, {76, 88}}}}};
    for (std::size_t n = 0; n < rows.size(); ++n)
    {
        const auto &row = rows[n];
        if (row.contains(u"shot_g"_s))
        {
            const auto g = gauges.find(row.value(u"gauge"_s).toInt());
            const QString key = row.value(u"powder"_s).toLower();
            if (g == gauges.end() || !powders.contains(key)) continue;
            int hull = static_cast<int>(row.value(u"hull_mm"_s).toDouble() + 0.5);
            if (hull == 69) hull = 70;
            const auto volume = g->second.volume_gr.find(hull);
            if (volume == g->second.volume_gr.end()) continue;
            QVariantMap powder = powders.value(key);
            if (ba.contains(key)) powder.insert(u"Ba"_s, ba.value(key));
            const double shot = row.value(u"shot_g"_s).toDouble();
            const double column_mm = shot / (11.34 * 0.62) * 1000.0 / g->second.area_mm2 + 25.0;  // shot (packing 0.62) + wad 25 mm
            const QVariantMap caliber{{u"cipname"_s, u"%1/%2"_s.arg(g->first).arg(hull)},
                                      {u"L3"_s, hull == 70 ? 69.8 : double(hull)},
                                      {u"V"_s, volume->second},
                                      {u"c_Z"_s, g->second.bore_mm},
                                      {u"c_Q"_s, g->second.area_mm2},
                                      {u"Pmax"_s, 740.0},
                                      {u"standard"_s, u"CIP"_s}};
            const QVariantMap bullet{{u"mname"_s, u"Shot"_s},
                                     {u"pname"_s, QString()},
                                     {u"gdia"_s, g->second.bore_mm},
                                     {u"glen"_s, column_mm},
                                     {u"gmass"_s, (shot + 2.5) / grt::kGrainToGram},
                                     {u"gpressure"_s, 20.0},
                                     {u"gtailtype"_s, 0}};
            grt::LoadParams p;
            p.charge_g = row.value(u"charge_g"_s).toDouble();
            p.barrelLength_mm = row.value(u"barrel_mm"_s, u"711"_s).toDouble();
            p.seatingDepth_mm = column_mm;  // completely inside the hull
            grt::LoadFile lf = grt::buildLoad(caliber, bullet, powder, p, calColumns);
            lf.caliber.set(u"shotshell"_s, u"true"_s);
            const grt::SimResult r = grt::simulate(withPhys(lf, key), cal, steps);
            if (r.status != grt::SimStatus::Ok) continue;
            out << n << ',' << QString::asprintf("%.3f,%.1f", r.muzzleVelocity_ms, r.maxPressure_bar) << '\n';
            continue;
        }
        const auto cal0 = calibers.constFind(row.value(u"caliber"_s));
        const QString key = row.value(u"powder"_s).toLower();
        if (cal0 == calibers.constEnd() || !powders.contains(key)) continue;
        QVariantMap powder = powders.value(key);
        if (ba.contains(key)) powder.insert(u"Ba"_s, ba.value(key));
        QVariantMap bullet{{u"mname"_s, row.value(u"bullet"_s)},
                           {u"pname"_s, QString()},
                           {u"gdia"_s, row.value(u"bullet_dia_mm"_s).toDouble()},
                           {u"glen"_s, row.value(u"bullet_len_mm"_s).toDouble()},
                           {u"gmass"_s, row.value(u"bullet_gr"_s).toDouble()},
                           {u"gtailtype"_s, row.value(u"tail_type"_s).toInt()}};
        // pistol and revolver cartridges (C.I.P. table IV): another initial gas pressure (GRT: about 150 bar, rifles 250)
        if (cli.isSet(u"ps-handgun"_s) && cal0->value(u"cippdf"_s).toString().contains(u"/tab-iv/"_s))
            bullet.insert(u"gpressure"_s, cli.value(u"ps-handgun"_s).toDouble());
        if (row.value(u"tail_type"_s).toInt() == 2)
        {
            bullet.insert(u"gtaildiaA"_s, row.value(u"bullet_dia_mm"_s).toDouble());
            bullet.insert(u"gtaildiaB"_s, 0.88 * row.value(u"bullet_dia_mm"_s).toDouble());
            bullet.insert(u"gtailh"_s, 0.45 * row.value(u"bullet_dia_mm"_s).toDouble());
        }
        grt::LoadParams p;
        p.charge_g = row.value(u"charge_g"_s).toDouble();
        p.barrelLength_mm = row.value(u"barrel_mm"_s).toDouble();
        p.seatingDepth_mm = cal0->value(u"L3"_s).toDouble() + bullet.value(u"glen"_s).toDouble() - row.value(u"col_mm"_s).toDouble();
        if (!row.value(u"twist_mm"_s).isEmpty()) p.twistLength_mm = row.value(u"twist_mm"_s).toDouble();
        if (p.seatingDepth_mm <= 1.0 || p.barrelLength_mm <= 0.0) continue;
        const grt::SimResult r = grt::simulate(withPhys(grt::buildLoad(*cal0, bullet, powder, p, calColumns), key), cal, steps);
        if (r.status != grt::SimStatus::Ok) continue;
        out << n << ',' << QString::asprintf("%.3f,%.1f", r.muzzleVelocity_ms, r.maxPressure_bar) << '\n';
    }
    return 0;
}
