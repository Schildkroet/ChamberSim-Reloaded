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

#pragma once

#include "loadfile.h"
#include "projectdata.h"
#include "simconfig.h"

#include <QString>
#include <QStringList>

#include <array>
#include <vector>

// Fits a powder's model values to the measured charges of one or more loads (Levenberg-Marquardt).
namespace csr::powderfit
{

enum class Param
{
    Ba,
    A0,
    Z1,
    Z2
};

// The values a fit changes: Original = the load's Ba, a0, z1, z2; Physical = physBa ... physZ2.
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

// The measured charges of a load with velocity shots; warns about input that spoils a fit (e.g. the CIP case volume).
[[nodiscard]] std::vector<Sample> samples(const LoadFile &load, const QString &source, QStringList *warnings);

// What the data supports: Ba (1-2 charges), Ba and a0 (a few), all four (six charges in two or more loads).
[[nodiscard]] std::vector<Param> defaultParams(const std::vector<Sample> &samples);

struct Options
{
    std::vector<Param> params;
    double pressureWeight{1.0};  // weight of the relative pressure errors against the velocity errors
    int maxIterations{60};
    int threads{0};                // 0: all cores
    Model model{Model::Original};  // fitted with that model's constants, whatever profile is active
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
    Model model{Model::Original};
    std::array<double, 4> base{};  // Ba, a0, z1, z2 at the start
};

// Starts from the model values of the first sample's load; all samples share them.
[[nodiscard]] Result fit(const std::vector<Sample> &samples, const Options &options);

// Fits both models (shotshells: the original only); the original's result first.
[[nodiscard]] std::vector<Result> fitAll(const std::vector<Sample> &samples, Options options);

// Writes the fitted values into a load's propellant section; the list version only the improved results.
void apply(EntryList &propellant, const Result &result);
void apply(EntryList &propellant, const std::vector<Result> &results);

// Qlty of a fit, 0.05..1 in steps of 0.05 (0: no charges): 0 at 10 % velocity or 20 % pressure error, lower below 100 charges.
// Python twin: scripts/data/fit_quality.py.
[[nodiscard]] double fitQuality(const Result &result);

// A propellant record from a load's propellant section with the fitted values (21 degC, the powder's photos, `descr`, the date).
[[nodiscard]] QVariantMap toRecord(const EntryList &propellant, const Result &result, const projectdata::Schema &schema,
                                   const QString &descr);

// "Fitted with ... to N charges / M shots ..." for a record.
[[nodiscard]] QString describe(const Result &result, const std::vector<Sample> &samples);

}  // namespace csr::powderfit
