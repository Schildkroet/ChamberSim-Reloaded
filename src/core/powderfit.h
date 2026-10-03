#pragma once

#include "loadfile.h"
#include "projectdata.h"
#include "simconfig.h"

#include <QString>
#include <QStringList>

#include <vector>

// Fitting a propellant model to measurements: the measured charges of one or more loads (Measurements tab:
// chronograph velocity, optionally peak pressure) with the same powder; the powder's model values are changed until
// the simulation matches them as well as possible (least squares, Levenberg-Marquardt).
namespace grt::powderfit
{

enum class Param
{
    Ba,
    A0,
    Z1,
    Z2
};

[[nodiscard]] QString name(Param p);                                                    // "Ba", "a0", "z1", "z2"
[[nodiscard]] std::vector<Param> parse(const QString &list, QString *error = nullptr);  // "Ba,a0"

// One measured charge of a load.
struct Sample
{
    QString source;  // file name, for messages
    SimInput input;  // the load (the charge is set from the measurement)
    double charge_g{};
    int shots{};           // velocity shots
    double velocity_ms{};  // mean measured muzzle velocity
    int pressureShots{};
    double pressure_bar{};  // mean measured peak pressure (0: not measured)
};

// The measured charges of a load file (charges without velocity shots are skipped). Adds warnings about input that
// spoils a fit (e.g. a case volume that is still the database's CIP value instead of a measured fired case).
[[nodiscard]] std::vector<Sample> samples(const LoadFile &load, const QString &source, QStringList *warnings);

// The parameters worth fitting for this much data: Ba alone for one or two charges, Ba and a0 for a few, the whole
// form function (Ba, a0, z1, z2) from six charges in at least two loads (e.g. the 2x2x2 workup).
[[nodiscard]] std::vector<Param> defaultParams(const std::vector<Sample> &samples);

struct Options
{
    std::vector<Param> params;
    double pressureWeight{1.0};  // weight of the relative pressure errors against the velocity errors
    int maxIterations{60};
    int threads{0};  // threads for the simulations (0: all cores); the samples are independent of each other
};

struct SampleResult
{
    double velocityBefore{}, velocityAfter{};  // simulated, m/s (0: charge cannot be simulated)
    double pressureBefore{}, pressureAfter{};  // simulated peak pressure, bar
};

struct Result
{
    std::vector<Param> params;
    std::vector<double> before, after;               // parameter values
    std::vector<SampleResult> samples;               // in the order of the input
    double velocityRmsBefore{}, velocityRmsAfter{};  // relative, shot-weighted
    double pressureRmsBefore{}, pressureRmsAfter{};  // relative (0 without pressure measurements)
    int iterations{};
    bool improved{};
    // the fitted values are the load's physical burn values (physBa, physA0, ...: physical model active and the load has them); they are
    // kept in the load only, a propellant record has no columns for them
    bool physical{};
};

// Starts from the model values of the first sample's load; all samples share them.
[[nodiscard]] Result fit(const std::vector<Sample> &samples, const Options &options);

// Whether fit() would change the physical burn values of these samples' load (see Result::physical).
[[nodiscard]] bool fitsPhysical(const std::vector<Sample> &samples);

// The fitted values put into a load's propellant section (for saving or exporting); physical values into physBa ... .
void apply(EntryList &propellant, const Result &result);

// A propellant record (database columns of `schema`) from a load's propellant section with the fitted values:
// reference temperature 21 degC (the fitted Ba refers to it), the photos of the powder (Database::insert copies them
// into the own database), the description `descr` and the date.
[[nodiscard]] QVariantMap toRecord(const EntryList &propellant, const Result &result, const projectdata::Schema &schema,
                                   const QString &descr);

// "Fitted with ... to N charges / M shots ...": a description of the fit for a record.
[[nodiscard]] QString describe(const Result &result, const std::vector<Sample> &samples);

}  // namespace grt::powderfit
