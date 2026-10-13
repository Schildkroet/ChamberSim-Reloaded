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

#include <QByteArray>
#include <QString>

#include <optional>
#include <vector>

namespace csr
{

enum class ChronographFormat
{
    Unknown,
    MagnetoSpeed,
    Labradar,
    GarminXero
};

// One series of a chronograph file: the velocities in m/s in shot order (missed shots are left out).
struct ChronographSeries
{
    QString name;
    std::vector<double> velocities_ms;
};

struct ChronographFile
{
    ChronographFormat format{ChronographFormat::Unknown};
    std::vector<ChronographSeries> series;
};

[[nodiscard]] QString formatName(ChronographFormat f);

// Reads a chronograph export. Recognised: the Garmin Xero C1 Pro's FIT shot session file (binary; chrono shot data messages) and the
// CSV the Garmin Xero app exports ("#,SPEED (MPS),..." table), MagnetoSpeed CSV ("Series, n, Shots:, count" blocks with one
// "series, shot, speed, unit" line per shot; one file can hold several series) and the Labradar "Report.csv"
// ("Series No", "Units velocity" and a "Shot ID;V0;..." table, V0 = muzzle velocity). nullopt with `error` set if
// the text is none of them or has no shots.
[[nodiscard]] std::optional<ChronographFile> readChronograph(const QByteArray &data, QString *error = nullptr);

}  // namespace csr
