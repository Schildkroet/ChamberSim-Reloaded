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

#include <QObject>
#include <QString>

// The user's choice of the solver calibration (Settings > Solver): the constants fitted to the original program, or those fitted
// to the manufacturers' published load data. Persisted in the application settings; changing it recalculates every open load.
class SolverPreferences : public QObject
{
    Q_OBJECT
public:
    static SolverPreferences &instance();

    [[nodiscard]] csr::CalibrationProfile profile() const noexcept { return m_profile; }
    // Sets (and stores) the profile; `remember` false: for this run only (command line).
    void setProfile(csr::CalibrationProfile profile, bool remember = true);

    // "GRT Original for original GRT files": a load whose file is the original program's *.grtload is simulated with GRT Original
    // (the original's constants and the file's own burn values), whatever profile() is, so its results match the original program.
    [[nodiscard]] bool originalForGrtFiles() const noexcept { return m_originalForGrtFiles; }
    void setOriginalForGrtFiles(bool on);
    // The profile a load is simulated with (`grtFile`: its file is a *.grtload) and its constants.
    [[nodiscard]] csr::CalibrationProfile profileFor(bool grtFile) const noexcept;
    [[nodiscard]] const csr::Calibration &calibrationFor(bool grtFile) const noexcept;

    [[nodiscard]] static QString name(csr::CalibrationProfile profile);
    [[nodiscard]] static QString description(csr::CalibrationProfile profile);
    // "original" or "published" (settings value and command line).
    [[nodiscard]] static QString key(csr::CalibrationProfile profile);
    [[nodiscard]] static std::optional<csr::CalibrationProfile> fromKey(const QString &key);

signals:
    void changed();

private:
    SolverPreferences();

    csr::CalibrationProfile m_profile{csr::CalibrationProfile::Physical};
    bool m_originalForGrtFiles{false};
};
