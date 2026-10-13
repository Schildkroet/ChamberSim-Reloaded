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

#include <QString>

#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace csr
{

// Attributes in file order with their raw (still percent-encoded) values.
using RawAttributes = std::vector<std::pair<QString, QString>>;

// One shot of a measured charge: chronograph velocity and peak pressure (0 = not measured).
struct Shot
{
    QString name;
    double velocity_ms{};
    double pressure_bar{};
};

// One charge of the <Measurement> block of a load file's appendix. The file stores the charge in kg, velocities in
// m/s and pressures in bar. Attributes and child elements this program does not edit (menu, source, a
// <PressureTraceFile>, ...) are kept and written back unchanged.
struct MeasuredCharge
{
    QString name;
    double charge_kg{};
    QString note;
    std::vector<Shot> shots;
    RawAttributes attributes;  // all attributes as read (name, note and value are taken from the fields above)
    QString extraXml;          // child elements other than <shot>, verbatim

    [[nodiscard]] double charge_g() const noexcept { return charge_kg * 1e3; }
    void setCharge_g(double g) noexcept { charge_kg = g * 1e-3; }

    // A new charge with the attributes the original program writes.
    [[nodiscard]] static MeasuredCharge create(const QString &name, double charge_g);
};

// The measurement results of a load ("Measurement" tab of the original program).
struct Measurement
{
    QString title;
    std::vector<MeasuredCharge> charges;
    RawAttributes attributes;  // index, showinreport, title, ... as read
};

struct ShotStatistics
{
    int count{};
    double mean{};
    double sd{};  // sample standard deviation (n - 1)
    double min{};
    double max{};

    [[nodiscard]] double extremeSpread() const noexcept { return max - min; }
};

// Statistics over the values above 0 (0 means "not measured").
[[nodiscard]] ShotStatistics statistics(std::span<const double> values);
[[nodiscard]] ShotStatistics velocityStatistics(const MeasuredCharge &charge);
[[nodiscard]] ShotStatistics pressureStatistics(const MeasuredCharge &charge);

// Reads all <Measurement> blocks of an appendix (see LoadFile::appendixXml), in file order (one per series).
[[nodiscard]] std::vector<Measurement> readMeasurements(const QString &appendixXml);

// The appendix with its <Measurement> blocks replaced, in order, by `series`: blocks beyond the given series are
// removed, further series are appended (with the next free tab index). Everything else in the appendix is kept.
[[nodiscard]] QString writeMeasurements(const QString &appendixXml, std::span<const Measurement> series);

// The first block only: nullopt if there is none, or the appendix with the first block replaced by `m` (appended if
// there was none) and the other blocks kept.
[[nodiscard]] std::optional<Measurement> readMeasurement(const QString &appendixXml);
[[nodiscard]] QString writeMeasurement(const QString &appendixXml, const Measurement &m);

}  // namespace csr
