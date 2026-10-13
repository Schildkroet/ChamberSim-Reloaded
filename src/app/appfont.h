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

class QApplication;

// The application-wide font size, chosen by the user in the settings. Only the point size changes; the
// font family and style stay those of the platform. The size is limited to a range in which the program's
// layouts stay usable.
namespace appfont
{

inline constexpr int kMinSize = 8;  // points
inline constexpr int kMaxSize = 20;

// Limits a requested size to [kMinSize, kMaxSize].
[[nodiscard]] int clampSize(int points);

// The platform's own font size (read once, before the program changes anything), within the allowed range.
[[nodiscard]] int systemSize();

// The user's choice; systemSize() if none was stored.
[[nodiscard]] int storedSize();
[[nodiscard]] bool hasCustomSize();
// Stores the choice (clamped). A value equal to the system size removes the setting.
void storeSize(int points);

// Sets the font size of the whole application (clamped) and returns the size that was applied.
int apply(QApplication &app, int points);

}  // namespace appfont
