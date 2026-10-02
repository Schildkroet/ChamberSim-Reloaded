// Fits a propellant model to chronograph (and pressure) measurements: the charges in the Measurements tab of one or
// more *.grtload files with the same powder. The powder's model values (burn rate Ba, form function a0, z1, z2) are
// changed until GRT Reloaded's simulation matches the measured velocities (and pressures) as well as possible.
//
//   grt_fitpowder [options] load.grtload [more loads ...]
//     --params Ba,a0,z1,z2    values to fit (default: chosen from the amount of data)
//     --pressure-weight W     weight of pressure errors against velocity errors (default 1; 0 = velocities only)
//     --out FILE.json         write the fitted powder as a record file (data/propellants/ format, see data/README.md)
//     --save                  write each load with the fitted values as <name>-fitted.grtload next to it
//
// Good data: measure the case volume of a FIRED case (water), the real bullet length and cartridge length, and use a
// good chronograph; for a whole new model shoot 2 calibers x 2 bullets x 2 charges x 5 shots (see CONTRIBUTING.md).

#include "database.h"
#include "loadfile.h"
#include "powderfit.h"
#include "projectdata.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDate>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <set>

using namespace Qt::StringLiterals;

namespace
{

QString pct(double value, double reference)
{
    if (value <= 0.0 || reference <= 0.0) return u"     -"_s;
    return QString::asprintf("%+6.2f", 100.0 * (value / reference - 1.0));
}

}  // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser cli;
    cli.setApplicationDescription(u"Fits a propellant model to the measurements in *.grtload files."_s);
    cli.addHelpOption();
    cli.addPositionalArgument(u"loads"_s, u"*.grtload files with a Measurements tab (same powder)"_s, u"load..."_s);
    cli.addOptions({{u"params"_s, u"values to fit, e.g. Ba,a0,z1,z2 (default: chosen from the data)"_s, u"list"_s},
                    {u"pressure-weight"_s, u"weight of pressure errors (default 1, 0 = velocities only)"_s, u"w"_s, u"1"_s},
                    {u"out"_s, u"write the fitted powder as a record file (JSON)"_s, u"file"_s},
                    {u"save"_s, u"write each load with the fitted values as <name>-fitted.grtload"_s}});
    cli.process(app);
    QTextStream out(stdout);
    QTextStream err(stderr);
    if (cli.positionalArguments().isEmpty())
    {
        err << "No load files given (see --help).\n";
        return 2;
    }

    // ---- read the measurements ----
    QStringList warnings;
    std::vector<grt::powderfit::Sample> samples;
    std::vector<std::pair<QString, grt::LoadFile>> loads;
    std::set<QString> powders;
    for (const QString &path : cli.positionalArguments())
    {
        QString error;
        auto lf = grt::LoadFile::read(path, &error);
        if (!lf)
        {
            err << "Cannot read " << path << ": " << error << '\n';
            return 2;
        }
        const QString name = QFileInfo(path).fileName();
        for (auto &s : grt::powderfit::samples(*lf, name, &warnings)) samples.push_back(std::move(s));
        powders.insert((lf->propellant.text(u"mname") + u' ' + lf->propellant.text(u"pname")).trimmed());
        loads.emplace_back(path, std::move(*lf));
    }
    if (powders.size() > 1)
        warnings << u"the loads use different powders (%1): one model is fitted for all"_s.arg(
            QStringList(powders.begin(), powders.end()).join(u", "_s));
    if (samples.empty())
    {
        for (const QString &w : warnings) err << "WARNING " << w << '\n';
        err << "No measured charges found.\n";
        return 1;
    }

    grt::powderfit::Options options;
    if (cli.isSet(u"params"_s))
    {
        QString error;
        options.params = grt::powderfit::parse(cli.value(u"params"_s), &error);
        if (options.params.empty())
        {
            err << error << '\n';
            return 2;
        }
    }
    else
    {
        options.params = grt::powderfit::defaultParams(samples);
    }
    options.pressureWeight = cli.value(u"pressure-weight"_s).toDouble();
    int shots = 0;
    for (const auto &s : samples) shots += s.shots;
    if (static_cast<std::size_t>(samples.size()) < options.params.size())
        warnings
            << u"%1 measured charges for %2 values: the result is not determined well"_s.arg(samples.size()).arg(options.params.size());

    QStringList names;
    for (const auto p : options.params) names << grt::powderfit::name(p);
    out << "Powder:  " << QStringList(powders.begin(), powders.end()).join(u", "_s) << '\n';
    out << "Data:    " << samples.size() << " charges, " << shots << " shots in " << loads.size() << " file(s)\n";
    out << "Fitting: " << names.join(u", "_s) << "\n\n";
    for (const QString &w : warnings) out << "WARNING " << w << '\n';
    if (!warnings.isEmpty()) out << '\n';

    const grt::powderfit::Result res = grt::powderfit::fit(samples, options);

    // ---- report ----
    out << QString::asprintf("%-34s %9s %5s %9s %9s %7s %9s %7s", "file", "charge g", "shots", "v meas", "v before", "%", "v after", "%")
        << '\n';
    bool pressure = false;
    for (std::size_t i = 0; i < samples.size(); ++i)
    {
        const auto &s = samples[i];
        const auto &r = res.samples[i];
        out << QString::asprintf("%-34s %9.4f %5d %9.1f %9.1f %7s %9.1f %7s", qPrintable(s.source.left(34)), s.charge_g, s.shots,
                                 s.velocity_ms, r.velocityBefore, qPrintable(pct(r.velocityBefore, s.velocity_ms)), r.velocityAfter,
                                 qPrintable(pct(r.velocityAfter, s.velocity_ms)))
            << '\n';
        pressure = pressure || s.pressureShots > 0;
    }
    if (pressure)
    {
        out << '\n'
            << QString::asprintf("%-34s %9s %9s %9s %7s %9s %7s", "file", "charge g", "p meas", "p before", "%", "p after", "%") << '\n';
        for (std::size_t i = 0; i < samples.size(); ++i)
        {
            const auto &s = samples[i];
            if (s.pressureShots == 0) continue;
            const auto &r = res.samples[i];
            out << QString::asprintf("%-34s %9.4f %9.0f %9.0f %7s %9.0f %7s", qPrintable(s.source.left(34)), s.charge_g, s.pressure_bar,
                                     r.pressureBefore, qPrintable(pct(r.pressureBefore, s.pressure_bar)), r.pressureAfter,
                                     qPrintable(pct(r.pressureAfter, s.pressure_bar)))
                << '\n';
        }
    }
    out << "\nModel values:\n";
    for (std::size_t i = 0; i < res.params.size(); ++i)
        out << QString::asprintf("  %-3s %10.5f -> %10.5f", qPrintable(grt::powderfit::name(res.params[i])), res.before[i], res.after[i])
            << '\n';
    out << QString::asprintf("\nVelocity error (rms): %.2f %% -> %.2f %%", 100 * res.velocityRmsBefore, 100 * res.velocityRmsAfter);
    if (pressure)
        out << QString::asprintf(";  pressure error (rms): %.2f %% -> %.2f %%", 100 * res.pressureRmsBefore, 100 * res.pressureRmsAfter);
    out << QString::asprintf("  (%d iterations)\n", res.iterations);
    if (!res.improved) out << "The model values were not changed: the start values fit best.\n";

    // ---- results as files ----
    if (cli.isSet(u"save"_s))
    {
        for (auto &[path, lf] : loads)
        {
            grt::powderfit::apply(lf.propellant, res);
            const QFileInfo fi(path);
            const QString target = fi.dir().filePath(fi.completeBaseName() + u"-fitted.grtload"_s);
            QString error;
            if (!lf.write(target, &error))
            {
                err << "Cannot write " << target << ": " << error << '\n';
                return 2;
            }
            out << "Saved " << target << '\n';
        }
    }
    if (cli.isSet(u"out"_s))
    {
        // the propellant of the first load, with the fitted values, in the database's columns
        QTemporaryDir tmp;
        grt::Database db;
        grt::Database::setProjectDataLocation(tmp.path());
        if (!db.open({}, tmp.filePath(u"own.db"_s)))
        {
            err << "Cannot create the record (database)\n";
            return 2;
        }
        const auto schema = db.schema(grt::Table::Propellant);
        const QVariantMap record =
            grt::powderfit::toRecord(loads.front().second.propellant, res, schema, grt::powderfit::describe(res, samples));
        QFile f(cli.value(u"out"_s));
        if (!f.open(QIODevice::WriteOnly) || f.write(grt::projectdata::toJson(record, schema)) < 0)
        {
            err << "Cannot write " << f.fileName() << '\n';
            return 2;
        }
        out << "Saved " << f.fileName() << '\n';
        for (const QString &p : grt::projectdata::check(grt::Table::Propellant, record, schema)) out << "  note: " << p << '\n';
        QStringList kept;
        for (const QString &c : {u"Qex"_s, u"k"_s, u"a0"_s, u"a1"_s, u"z1"_s, u"z2"_s, u"pc"_s, u"pcd"_s, u"tcc"_s, u"tch"_s})
            if (std::ranges::none_of(res.params, [&](auto p) { return grt::powderfit::name(p) == c; })) kept << c;
        out << "  note: " << kept.join(u", "_s)
            << " are taken unchanged from the load. Before contributing the record, make sure they come from the\n"
               "        manufacturer or your own measurements, not from another program's database (see CONTRIBUTING.md).\n";
    }
    return 0;
}
