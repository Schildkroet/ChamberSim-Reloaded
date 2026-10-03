#include "solverprefs.h"

#include <QSettings>

namespace
{
constexpr auto kSettingsKey = "solver/calibration";
}

SolverPreferences &SolverPreferences::instance()
{
    static SolverPreferences prefs;
    return prefs;
}

SolverPreferences::SolverPreferences()
{
    const auto stored = fromKey(QSettings().value(QLatin1String(kSettingsKey)).toString());
    m_profile = stored.value_or(grt::CalibrationProfile::Original);
    grt::setCalibrationProfile(m_profile);
}

void SolverPreferences::setProfile(grt::CalibrationProfile profile, bool remember)
{
    if (remember) QSettings().setValue(QLatin1String(kSettingsKey), key(profile));
    if (profile == m_profile) return;
    m_profile = profile;
    grt::setCalibrationProfile(profile);
    emit changed();
}

QString SolverPreferences::name(grt::CalibrationProfile profile)
{
    return profile == grt::CalibrationProfile::Original ? tr("Original program (GRT)") : tr("Published load data");
}

QString SolverPreferences::description(grt::CalibrationProfile profile)
{
    return profile == grt::CalibrationProfile::Original
               ? tr("Constants fitted to runs of Gordon's Reloading Tool: results like the original program (velocity about 1 %, peak "
                    "pressure about 3 % off).")
               : tr("Constants fitted to the loads and measured pressures published by the powder manufacturers (Hodgdon, Western "
                    "Powders): closer to measured pressures, but no longer to the original program.");
}

QString SolverPreferences::key(grt::CalibrationProfile profile)
{
    return profile == grt::CalibrationProfile::Original ? QStringLiteral("original") : QStringLiteral("published");
}

std::optional<grt::CalibrationProfile> SolverPreferences::fromKey(const QString &key)
{
    if (key == QLatin1String("original")) return grt::CalibrationProfile::Original;
    if (key == QLatin1String("published")) return grt::CalibrationProfile::Published;
    return std::nullopt;
}
