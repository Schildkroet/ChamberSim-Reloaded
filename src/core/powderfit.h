#pragma once

#include "loadfile.h"
#include "projectdata.h"
#include "simconfig.h"

#include <QString>
#include <QStringList>

#include <array>
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

// Which model's values a fit changes: the original's (Ba, a0, z1, z2 of the load; also used by the Published profile) or the physical
// model's (physBa, physA0, physZ1, physZ2; a load without them gets them).
enum class Model
{
    Original,
    Physical
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
    int threads{0};                // threads for the simulations (0: all cores); the samples are independent of each other
    Model model{Model::Original};  // fitted with that model's own constants, whatever profile is active
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
    Model model{Model::Original};  // physical: the load's physical burn values (a propellant record has no columns for them)
    std::array<double, 4> base{};  // Ba, a0, z1, z2 of the model at the start (apply() replaces the fitted ones)
};

// Starts from the model values of the first sample's load; all samples share them.
[[nodiscard]] Result fit(const std::vector<Sample> &samples, const Options &options);

// Fits both models (the physical one not for shotshells: their model uses the original's constants): one result per model, the
// original's first.
[[nodiscard]] std::vector<Result> fitAll(const std::vector<Sample> &samples, Options options);

// The fitted values put into a load's propellant section (for saving or exporting); physical values into physBa ... . The list
// version applies the improved results, the original model's first (the physical values refer to the load's values).
void apply(EntryList &propellant, const Result &result);
void apply(EntryList &propellant, const std::vector<Result> &results);

// Quality of a fit (the propellant column Qlty, 0..1 in steps of 0.05, like the original's): falls with the velocity error (0 at
// 10 %) and the pressure error (0 at 20 %, only with pressure measurements) and with fewer than 100 charges, at least 0.05. 0 (unknown) without charges.
// Python twin: scripts/data/fit_quality.py.
[[nodiscard]] double fitQuality(const Result &result);

// A propellant record (database columns of `schema`) from a load's propellant section with the fitted values:
// reference temperature 21 degC (the fitted Ba refers to it), the photos of the powder (Database::insert copies them
// into the own database), the description `descr` and the date.
[[nodiscard]] QVariantMap toRecord(const EntryList &propellant, const Result &result, const projectdata::Schema &schema,
                                   const QString &descr);

// "Fitted with ... to N charges / M shots ...": a description of the fit for a record.
[[nodiscard]] QString describe(const Result &result, const std::vector<Sample> &samples);

}  // namespace grt::powderfit
