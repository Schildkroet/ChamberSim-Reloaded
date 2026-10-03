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
    switch (profile)
    {
    case grt::CalibrationProfile::Original: return tr("Original program (GRT)");
    case grt::CalibrationProfile::Published: return tr("Published load data");
    case grt::CalibrationProfile::Physical: return tr("Physical model");
    }
    return {};
}

QString SolverPreferences::description(grt::CalibrationProfile profile)
{
    switch (profile)
    {
    case grt::CalibrationProfile::Original:
        return tr("Constants fitted to runs of Gordon's Reloading Tool: results like the original program (velocity about 1 %, peak "
                  "pressure about 3 % off).");
    case grt::CalibrationProfile::Published:
        return tr("The original program's equations with constants fitted to the loads and measured pressures published by the powder "
                  "manufacturers (Hodgdon, Western Powders): closer to measured pressures, but no longer to the original program.");
    case grt::CalibrationProfile::Physical:
        return tr("The project's own physical model (ignition and flame spread, engraving and friction, heat loss, spin; peak pressure "
                  "independent of the barrel length), fitted to the same published loads. Uses the physical burn values of the "
                  "project's powders when the load holds such a powder unchanged.");
    }
    return {};
}

QString SolverPreferences::key(grt::CalibrationProfile profile)
{
    switch (profile)
    {
    case grt::CalibrationProfile::Original: return QStringLiteral("original");
    case grt::CalibrationProfile::Published: return QStringLiteral("published");
    case grt::CalibrationProfile::Physical: return QStringLiteral("physical");
    }
    return {};
}

std::optional<grt::CalibrationProfile> SolverPreferences::fromKey(const QString &key)
{
    if (key == QLatin1String("original")) return grt::CalibrationProfile::Original;
    if (key == QLatin1String("published")) return grt::CalibrationProfile::Published;
    if (key == QLatin1String("physical")) return grt::CalibrationProfile::Physical;
    return std::nullopt;
}
