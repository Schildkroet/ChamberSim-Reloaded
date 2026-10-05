// Fits a propellant model to published load data (samples from scripts/data/loaddata/vv_extract.py) instead of own measurements.
//
//   grt_fitdata [options] samples.csv
//     --data DIR              project data (calibers), default: the repository's data/
//     --params Ba,a0,z1,z2    values to fit (default: Ba,a0,z1,z2)
//     --start Ba=0.6,a0=1.2,... start values (default: generic single-base powder)
//     --pressure-fraction F   treat the maximum loads as F x the caliber's maximum pressure (0: velocities only, default 0)
//     --iterations N          at most N iterations (0: no fit, only simulate the start values; default 60)
//     --use-pressure          use the pressure_bar column of the CSV (measured pressure of the max loads)
//     --pressure-weight W     weight of these pressures against the velocities (default 0.5)
//     --kinds start|max|both  which load lines are fitted (default both)
//     --holdout CAL1,CAL2     calibers that are not fitted but evaluated with the fitted values
//     --table                 one line per sample
//     --out FILE.json         write the fitted powder as a record file
//
// The CSV has the columns of vv_extract.py: caliber, bullet, bullet_gr, bullet_dia_mm, bullet_len_mm, tail_type, col_mm,
// barrel_mm, twist_mm, kind (start|max), charge_g, velocity_ms, pmax_bar.

#include "database.h"
#include "loadbuilder.h"
#include "powderfit.h"
#include "projectdata.h"
#include "simconfig.h"
#include "solver.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QMap>
#include <QTemporaryDir>
#include <QTextStream>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <set>

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

struct Item
{
    QString caliber, kind, bullet;
    grt::powderfit::Sample sample;
};

QString pct(double value, double reference)
{
    return reference > 0.0 && value > 0.0 ? QString::asprintf("%+6.2f", 100.0 * (value / reference - 1.0)) : u"     -"_s;
}

double rms(const std::vector<double> &v)
{
    if (v.empty()) return 0.0;
    double s = 0.0;
    for (double x : v) s += x * x;
    return std::sqrt(s / static_cast<double>(v.size()));
}

}  // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser cli;
    cli.setApplicationDescription(u"Fits a propellant model to published load data (CSV)."_s);
    cli.addHelpOption();
    cli.addPositionalArgument(u"samples"_s, u"CSV file from scripts/data/loaddata/vv_extract.py"_s);
    cli.addOptions({{u"data"_s, u"project data directory"_s, u"dir"_s, QStringLiteral(GRT_SOURCE_DIR "/data")},
                    {u"params"_s, u"values to fit"_s, u"list"_s, u"Ba,a0,z1,z2"_s},
                    {u"start"_s, u"start values, e.g. Ba=0.6,a0=1.2"_s, u"list"_s},
                    {u"pressure-fraction"_s, u"maximum loads as a fraction of the caliber's maximum pressure"_s, u"f"_s, u"0"_s},
                    {u"iterations"_s, u"at most N iterations (0: only simulate)"_s, u"N"_s, u"60"_s},
                    {u"use-pressure"_s, u"use the pressure_bar column of the CSV for the max loads"_s},
                    {u"pressure-weight"_s, u"weight of these pressures"_s, u"w"_s, u"0.5"_s},
                    {u"kinds"_s, u"start, max or both"_s, u"kind"_s, u"both"_s},
                    {u"holdout"_s, u"calibers evaluated but not fitted"_s, u"list"_s},
                    {u"table"_s, u"one line per sample"_s},
                    {u"benchmark"_s, u"time one simulation at several step counts"_s},
                    {u"debug"_s, u"print the inputs and the status of the first samples"_s},
                    {u"out"_s, u"write the fitted powder as a record file (JSON)"_s, u"file"_s}});
    cli.process(app);
    QTextStream out(stdout);
    QTextStream err(stderr);
    if (cli.positionalArguments().size() != 1)
    {
        err << "One CSV file expected (see --help).\n";
        return 2;
    }

    QTemporaryDir tmp;
    grt::Database db;
    grt::Database::setProjectDataLocation(tmp.path());
    if (!db.open({}, tmp.filePath(u"own.db"_s)))
    {
        err << "Cannot open a database for the schema\n";
        return 2;
    }
    const auto calSchema = db.schema(grt::Table::Caliber);
    const auto loaded = grt::projectdata::load(cli.value(u"data"_s), grt::Table::Caliber, calSchema);
    QMap<QString, QVariantMap> calibers;
    for (const QVariantMap &r : loaded.records) calibers.insert(r.value(u"cipname"_s).toString(), r);
    QStringList calColumns;
    for (const auto &[name, type] : calSchema) calColumns << name;

    // start values: a generic single-base powder
    QVariantMap powder{{u"mname"_s, u"Test"_s}, {u"pname"_s, u"fit"_s}, {u"lotid"_s, QString()}, {u"Ba"_s, 0.6},  {u"Qex"_s, 3800.0},
                       {u"k"_s, 1.23},          {u"a0"_s, 1.2},         {u"a1"_s, 0.0},          {u"z1"_s, 0.45}, {u"z2"_s, 0.8},
                       {u"pc"_s, 1580.0},       {u"pcd"_s, 900.0},      {u"pt"_s, 21.0},         {u"tcc"_s, 0.0}, {u"tch"_s, 0.0}};
    for (const QString &kv : cli.value(u"start"_s).split(u',', Qt::SkipEmptyParts))
        powder.insert(kv.section(u'=', 0, 0).trimmed(), kv.section(u'=', 1).toDouble());

    const double pressureFraction = cli.value(u"pressure-fraction"_s).toDouble();
    const bool usePressure = cli.isSet(u"use-pressure"_s);
    const QString kinds = cli.value(u"kinds"_s);
    const QStringList holdoutList = cli.value(u"holdout"_s).split(u',', Qt::SkipEmptyParts);
    const QSet<QString> holdout(holdoutList.begin(), holdoutList.end());

    std::vector<Item> train, test;
    int skipped = 0;
    for (const auto &row : readCsv(cli.positionalArguments().front()))
    {
        if (kinds != u"both" && row.value(u"kind"_s) != kinds) continue;
        const auto cal = calibers.constFind(row.value(u"caliber"_s));
        if (cal == calibers.constEnd())
        {
            ++skipped;
            continue;
        }
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
        p.seatingDepth_mm = cal->value(u"L3"_s).toDouble() + bullet.value(u"glen"_s).toDouble() - row.value(u"col_mm"_s).toDouble();
        if (!row.value(u"twist_mm"_s).isEmpty()) p.twistLength_mm = row.value(u"twist_mm"_s).toDouble();
        if (p.seatingDepth_mm <= 1.0 || p.barrelLength_mm <= 0.0)
        {
            ++skipped;
            continue;
        }
        const grt::LoadFile lf = grt::buildLoad(*cal, bullet, powder, p, calColumns);
        Item item{.caliber = row.value(u"caliber"_s), .kind = row.value(u"kind"_s), .bullet = row.value(u"bullet"_s)};
        item.sample = {.source = item.caliber + u' ' + item.kind,
                       .input = grt::SimInput::fromLoad(lf),
                       .charge_g = p.charge_g,
                       .shots = 1,
                       .velocity_ms = row.value(u"velocity_ms"_s).toDouble()};
        if (pressureFraction > 0.0 && item.kind == u"max" && cal->value(u"Pmax"_s).toDouble() > 0.0)
        {
            item.sample.pressureShots = 1;
            item.sample.pressure_bar = pressureFraction * cal->value(u"Pmax"_s).toDouble();
        }
        if (usePressure && row.value(u"pressure_bar"_s).toDouble() > 0.0)
        {
            item.sample.pressureShots = 1;
            item.sample.pressure_bar = row.value(u"pressure_bar"_s).toDouble();
        }
        (holdout.contains(item.caliber) ? test : train).push_back(std::move(item));
    }
    // samples the solver rejects with the start values (a charge that does not fit the case at the assumed bulk density, ...)
    // would count as 100 % error and pull the fit: they are left out and reported
    int rejectedHigh = 0, rejectedOther = 0;
    for (auto *list : {&train, &test})
        std::erase_if(*list,
                      [&](const Item &i)
                      {
                          const auto status = grt::simulate(i.sample.input).status;
                          rejectedHigh += status == grt::SimStatus::ChargeTooHigh;
                          rejectedOther += status == grt::SimStatus::InvalidInput;
                          return status != grt::SimStatus::Ok;
                      });
    if (train.empty())
    {
        err << "No usable samples (" << skipped << " skipped).\n";
        return 1;
    }

    if (cli.isSet(u"debug"_s))
        for (std::size_t i = 0; i < std::min<std::size_t>(train.size(), 3); ++i)
        {
            const grt::SimInput &in = train[i].sample.input;
            const grt::SimResult r = grt::simulate(in);
            out << QString::asprintf(
                "status %d v %.1f p %.0f | case %.2f mm vol %.3f cm3 bore %.2f mm2 barrel %.0f dia %.2f m %.3f g len %.2f depth %.2f "
                "charge %.3f Ba %.3f a0 %.3f z1 %.3f z2 %.3f Qex %.0f k %.3f\n",
                static_cast<int>(r.status), r.muzzleVelocity_ms, r.maxPressure_bar, in.caseLength_mm, in.caseVolume_cm3, in.boreArea_mm2,
                in.barrelLength_mm, in.bulletDiameter_mm, in.bulletMass_g, in.bulletLength_mm, in.seatingDepth_mm, in.charge_g,
                in.vivacityBa, in.a0, in.z1, in.z2, in.heatQex_kJkg, in.kappa);
        }
    if (cli.isSet(u"benchmark"_s))
    {
        const grt::SimInput &in = train[std::min<std::size_t>(1, train.size() - 1)].sample.input;
        for (const int steps : {2000, 1000, 500, 250})
        {
            const auto t0 = std::chrono::steady_clock::now();
            grt::SimResult r;
            for (int i = 0; i < 100; ++i) r = grt::simulate(in, {}, steps);
            const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / 100.0;
            out << QString::asprintf("steps %4d: %6.2f ms per simulation, v %.2f m/s, p %.1f bar\n", steps, ms, r.muzzleVelocity_ms,
                                     r.maxPressure_bar);
        }
    }
    grt::powderfit::Options options;
    QString error;
    options.params = grt::powderfit::parse(cli.value(u"params"_s), &error);
    options.maxIterations = cli.value(u"iterations"_s).toInt();
    options.pressureWeight = cli.value(u"pressure-weight"_s).toDouble();
    std::vector<grt::powderfit::Sample> samples;
    for (const Item &i : train) samples.push_back(i.sample);
    const grt::powderfit::Result res = grt::powderfit::fit(samples, options);

    out << "Samples: " << train.size() << " fitted, " << test.size() << " held out, " << skipped << " skipped (data), " << rejectedHigh
        << " charge too high for the case, " << rejectedOther << " not simulable\n";
    out << "Fitting: ";
    for (std::size_t i = 0; i < res.params.size(); ++i)
        out << QString::asprintf("%s %.4f -> %.4f   ", qPrintable(grt::powderfit::name(res.params[i])), res.before[i], res.after[i]);
    out << QString::asprintf("\nVelocity error (rms): %.2f %% -> %.2f %%", 100 * res.velocityRmsBefore, 100 * res.velocityRmsAfter);
    if (pressureFraction > 0.0)
        out << QString::asprintf(";  pressure error (rms, max loads vs %.2f x Pmax): %.2f %% -> %.2f %%", pressureFraction,
                                 100 * res.pressureRmsBefore, 100 * res.pressureRmsAfter);
    else if (usePressure)
        out << QString::asprintf(";  pressure error (rms, measured): %.2f %% -> %.2f %%", 100 * res.pressureRmsBefore,
                                 100 * res.pressureRmsAfter);
    out << QString::asprintf("  (%d iterations)\n\n", res.iterations);

    // errors per caliber and kind
    std::map<QString, std::vector<double>> byCaliber;
    std::map<QString, std::vector<double>> byKind;
    for (std::size_t i = 0; i < train.size(); ++i)
    {
        const double e = res.samples[i].velocityAfter / train[i].sample.velocity_ms - 1.0;
        if (res.samples[i].velocityAfter <= 0.0) continue;
        byCaliber[train[i].caliber].push_back(e);
        byKind[train[i].kind].push_back(e);
        if (cli.isSet(u"table"_s))
            out << QString::asprintf("%-22s %-5s %-28s %7.3f g  v %7.1f  sim %7.1f %s %%  p sim %6.0f bar\n",
                                     qPrintable(train[i].caliber.left(22)), qPrintable(train[i].kind), qPrintable(train[i].bullet.left(28)),
                                     train[i].sample.charge_g, train[i].sample.velocity_ms, res.samples[i].velocityAfter,
                                     qPrintable(pct(res.samples[i].velocityAfter, train[i].sample.velocity_ms)),
                                     res.samples[i].pressureAfter);
    }
    out << "Velocity error by kind (rms / mean):\n";
    for (const auto &[k, v] : byKind)
    {
        double mean = 0.0;
        for (double x : v) mean += x;
        out << QString::asprintf("  %-6s n=%4zu  %5.2f %%  %+5.2f %%\n", qPrintable(k), v.size(), 100 * rms(v),
                                 100 * mean / static_cast<double>(v.size()));
    }
    out << "Velocity error by caliber (rms / mean):\n";
    for (const auto &[k, v] : byCaliber)
    {
        double mean = 0.0;
        for (double x : v) mean += x;
        out << QString::asprintf("  %-24s n=%4zu  %5.2f %%  %+5.2f %%\n", qPrintable(k), v.size(), 100 * rms(v),
                                 100 * mean / static_cast<double>(v.size()));
    }

    // held-out samples: simulate with the fitted values (a "fit" without iterations)
    if (!test.empty())
    {
        std::vector<grt::powderfit::Sample> held;
        for (Item &i : test)
        {
            for (std::size_t k = 0; k < res.params.size(); ++k)
            {
                switch (res.params[k])
                {
                case grt::powderfit::Param::Ba: i.sample.input.vivacityBa = res.after[k]; break;
                case grt::powderfit::Param::A0: i.sample.input.a0 = res.after[k]; break;
                case grt::powderfit::Param::Z1: i.sample.input.z1 = res.after[k]; break;
                case grt::powderfit::Param::Z2: i.sample.input.z2 = res.after[k]; break;
                }
            }
            held.push_back(i.sample);
        }
        grt::powderfit::Options none;
        none.params = {grt::powderfit::Param::Ba};
        none.maxIterations = 0;
        const auto r = grt::powderfit::fit(held, none);
        std::map<QString, std::vector<double>> heldByCaliber;
        std::vector<double> all;
        for (std::size_t i = 0; i < test.size(); ++i)
        {
            if (r.samples[i].velocityBefore <= 0.0) continue;
            const double e = r.samples[i].velocityBefore / test[i].sample.velocity_ms - 1.0;
            heldByCaliber[test[i].caliber].push_back(e);
            all.push_back(e);
        }
        out << QString::asprintf("\nHeld-out calibers (not fitted): rms %.2f %% over %zu samples\n", 100 * rms(all), all.size());
        for (const auto &[k, v] : heldByCaliber)
        {
            double mean = 0.0;
            for (double x : v) mean += x;
            out << QString::asprintf("  %-24s n=%4zu  %5.2f %%  %+5.2f %%\n", qPrintable(k), v.size(), 100 * rms(v),
                                     100 * mean / static_cast<double>(v.size()));
        }
    }
    (void)pct;
    return 0;
}
