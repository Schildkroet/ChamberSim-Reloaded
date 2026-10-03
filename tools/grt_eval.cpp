// Simulates published load data (CSV of tools/loaddata) with the project's propellant records and an adjustable solver calibration;
// the solver calibration experiments (analysis/calibrate_published.py) call it.
//
//   grt_eval samples.csv [--data DIR] [--set name=value]... [--ba "Maker Name=value"]... [--steps n] [--list-params]
//
// Prints one line per sample that can be simulated: row (0-based, in the CSV), simulated muzzle velocity in m/s and peak pressure in bar.
// Samples that cannot be simulated (charge too high for the case, ...) are left out. The powder of a row is the record of the
// project data whose "Manufacturer Name" is the CSV column `powder`; --ba replaces its burn rate.

#include "database.h"
#include "loadbuilder.h"
#include "projectdata.h"
#include "simconfig.h"
#include "solver.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QMap>
#include <QTemporaryDir>
#include <QTextStream>

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
    cli.addOptions({{u"data"_s, u"project data directory"_s, u"dir"_s, QStringLiteral(GRT_SOURCE_DIR "/data")},
                    {u"set"_s, u"calibration value name=value (repeatable)"_s, u"nv"_s},
                    {u"ba"_s, u"burn rate of a powder: \"Maker Name=value\" (repeatable)"_s, u"nv"_s},
                    {u"steps"_s, u"solver steps"_s, u"n"_s, u"2000"_s},
                    {u"list-params"_s, u"print the calibration names and values"_s}});
    cli.process(app);
    QTextStream out(stdout);
    grt::Calibration cal;
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
    for (std::size_t n = 0; n < rows.size(); ++n)
    {
        const auto &row = rows[n];
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
        const grt::SimResult r = grt::simulate(grt::SimInput::fromLoad(grt::buildLoad(*cal0, bullet, powder, p, calColumns)), cal, steps);
        if (r.status != grt::SimStatus::Ok) continue;
        out << n << ',' << QString::asprintf("%.3f,%.1f", r.muzzleVelocity_ms, r.maxPressure_bar) << '\n';
    }
    return 0;
}
