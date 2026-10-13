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

#include "recordfields.h"

#include "unitprefs.h"

#include <QLocale>

#include <algorithm>
#include <cmath>

namespace recordfields
{

const csr::units::Unit *unitOf(const csr::dbfields::Field &f)
{
    return f.quantity == csr::units::Quantity::None ? nullptr : &UnitPreferences::instance().unitFor(f.quantity);
}

QString shownNumber(const csr::dbfields::Field &f, double stored)
{
    QLocale locale;
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    const auto *u = unitOf(f);
    const double v = u ? csr::units::toDisplay(stored * f.toBase, *u) : stored;
    for (int d = u ? u->decimals : 0; d < 8; ++d)
    {
        const double scale = std::pow(10.0, d);
        if (std::abs(std::round(v * scale) / scale - v) <= 1e-9 * std::max(1.0, std::abs(v))) return locale.toString(v, 'f', d);
    }
    return locale.toString(v, 'g', 10);
}

}  // namespace recordfields
