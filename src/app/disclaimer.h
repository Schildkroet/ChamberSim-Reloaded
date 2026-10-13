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

#include <memory>

class QMessageBox;
class QWidget;

// The safety notice at start-up: handloading is dangerous, the results are estimates, no liability. "Do not show this again" is
// remembered (setting ui/hideDisclaimer) only together with "I understand".
namespace disclaimer
{

// Whether the notice is to be shown (not hidden by the user).
[[nodiscard]] bool wanted();

// The notice as a dialog: buttons "I understand" (accept) and "Quit", the check box "Do not show this again".
[[nodiscard]] std::unique_ptr<QMessageBox> create(QWidget *parent = nullptr);

// Shows the notice if wanted; false when the user quits.
[[nodiscard]] bool confirm(QWidget *parent = nullptr);

}  // namespace disclaimer
