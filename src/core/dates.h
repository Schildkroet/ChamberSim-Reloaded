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
#include <QVariant>

#include <optional>

namespace csr
{

// The database stores dates as text "yyyy-MM-dd" (like the original program). This brings a date into that form.
// Accepted: yyyy-MM-dd (also with a time, "yyyy-MM-dd hh:mm:ss" or ISO "...T..."), yyyy/MM/dd, dd.MM.yyyy and
// d.M.yyyy, and QDate / QDateTime values. An empty value and the original's "0000-00-00" give "" (no date);
// anything else that is not a valid date gives nullopt.
[[nodiscard]] std::optional<QString> normalizedDate(const QVariant &value);

}  // namespace csr
