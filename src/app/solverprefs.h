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

    [[nodiscard]] grt::CalibrationProfile profile() const noexcept { return m_profile; }
    // Sets (and stores) the profile; `remember` false: for this run only (command line).
    void setProfile(grt::CalibrationProfile profile, bool remember = true);

    [[nodiscard]] static QString name(grt::CalibrationProfile profile);
    [[nodiscard]] static QString description(grt::CalibrationProfile profile);
    // "original" or "published" (settings value and command line).
    [[nodiscard]] static QString key(grt::CalibrationProfile profile);
    [[nodiscard]] static std::optional<grt::CalibrationProfile> fromKey(const QString &key);

signals:
    void changed();

private:
    SolverPreferences();

    grt::CalibrationProfile m_profile{grt::CalibrationProfile::Original};
};
