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
    m_profile = stored.value_or(grt::CalibrationProfile::Physical);  // the default: the project's own physical model
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
    case grt::CalibrationProfile::Original: return tr("GRT Original");
    case grt::CalibrationProfile::Published: return tr("GRT Tuned");
    case grt::CalibrationProfile::Physical: return tr("Physical Model");
    }
    return {};
}

QString SolverPreferences::description(grt::CalibrationProfile profile)
{
    switch (profile)
    {
    case grt::CalibrationProfile::Original:
        return tr("Reproduces the results of Gordon's Reloading Tool (within about 1 % in velocity and 3 % in peak pressure). Choose it "
                  "to compare with GRT. Against real load data it is the least accurate model.");
    case grt::CalibrationProfile::Published:
        return tr("The equations of Gordon's Reloading Tool with constants fitted to the powder makers' published loads. Velocity is "
                  "about as accurate as with the Physical Model, but peak pressure is usually too low (about 5 % at maximum loads), "
                  "and it is less reliable for pistol and shotgun powders. Kept for comparison.");
    case grt::CalibrationProfile::Physical:
        return tr("Recommended. The program's own model of the burning powder and the bullet's travel, fitted to the powder makers' "
                  "published loads and measured pressures: typically about 6 % off in peak pressure and 2.5 % in velocity (median "
                  "against those tables). Most accurate for the 88 project powders that have their own burn values.");
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
