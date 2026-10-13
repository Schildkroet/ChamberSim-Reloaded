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

#include "solver.h"

#include <QString>
#include <QVariantMap>

#include <optional>
#include <vector>

namespace csr
{

// One result file written by the reference runner (tools/GRTRef): the full
// curves and the summary values of the original program for one load.
struct ReferenceRun
{
    QString job;
    bool stale{};  // the runner could not confirm it read the right tab
    std::vector<SimPoint> points;
    QVariantMap summary;  // raw strings, e.g. "MaxPressure" -> "2290.07 bar"

    // Leading number of a summary value ("2290.07 bar" -> 2290.07), or `fallback`.
    [[nodiscard]] double number(const QString &key, double fallback = 0.0) const;
};

[[nodiscard]] std::optional<ReferenceRun> readReferenceRun(const QString &jsonPath, QString *error = nullptr);

}  // namespace csr
