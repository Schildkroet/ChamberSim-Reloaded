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
