#include "solverprefs.h"

#include <QSettings>

namespace
{
constexpr auto kSettingsKey = "solver/calibration";
constexpr auto kOriginalForGrtFilesKey = "solver/originalForGrtFiles";
}  // namespace

SolverPreferences &SolverPreferences::instance()
{
    static SolverPreferences prefs;
    return prefs;
}

SolverPreferences::SolverPreferences()
{
    const auto stored = fromKey(QSettings().value(QLatin1String(kSettingsKey)).toString());
    m_profile = stored.value_or(csr::CalibrationProfile::Physical);  // the default: the project's own physical model
    csr::setCalibrationProfile(m_profile);
    m_originalForGrtFiles = QSettings().value(QLatin1String(kOriginalForGrtFilesKey), false).toBool();
}

void SolverPreferences::setProfile(csr::CalibrationProfile profile, bool remember)
{
    if (remember) QSettings().setValue(QLatin1String(kSettingsKey), key(profile));
    if (profile == m_profile) return;
    m_profile = profile;
    csr::setCalibrationProfile(profile);
    emit changed();
}

void SolverPreferences::setOriginalForGrtFiles(bool on)
{
    QSettings().setValue(QLatin1String(kOriginalForGrtFilesKey), on);
    if (on == m_originalForGrtFiles) return;
    m_originalForGrtFiles = on;
    emit changed();
}

csr::CalibrationProfile SolverPreferences::profileFor(bool grtFile) const noexcept
{
    return grtFile && m_originalForGrtFiles ? csr::CalibrationProfile::Original : m_profile;
}

const csr::Calibration &SolverPreferences::calibrationFor(bool grtFile) const noexcept
{
    static const csr::Calibration original = csr::calibrationFor(csr::CalibrationProfile::Original);
    return profileFor(grtFile) == m_profile ? csr::activeCalibration() : original;
}

QString SolverPreferences::name(csr::CalibrationProfile profile)
{
    switch (profile)
    {
    case csr::CalibrationProfile::Original: return tr("GRT Original");
    case csr::CalibrationProfile::Published: return tr("GRT Tuned");
    case csr::CalibrationProfile::Physical: return tr("Physical Model");
    }
    return {};
}

QString SolverPreferences::description(csr::CalibrationProfile profile)
{
    switch (profile)
    {
    case csr::CalibrationProfile::Original:
        return tr("Reproduces the results of Gordon's Reloading Tool (within about 1 % in velocity and 3 % in peak pressure). Choose it "
                  "to compare with GRT. Against real load data it is the least accurate model.");
    case csr::CalibrationProfile::Published:
        return tr("The equations of Gordon's Reloading Tool with constants fitted to the powder makers' published loads. Velocity is "
                  "about as accurate as with the Physical Model, but peak pressure is usually too low (about 5 % at maximum loads), "
                  "and it is less reliable for pistol and shotgun powders. Kept for comparison.");
    case csr::CalibrationProfile::Physical:
        return tr("Recommended. The program's own model of the burning powder and the bullet's travel, fitted to the powder makers' "
                  "published loads and measured pressures: typically about 6 % off in peak pressure and 2.5 % in velocity (median "
                  "against those tables). Most accurate for the 88 project powders that have their own burn values.");
    }
    return {};
}

QString SolverPreferences::key(csr::CalibrationProfile profile)
{
    switch (profile)
    {
    case csr::CalibrationProfile::Original: return QStringLiteral("original");
    case csr::CalibrationProfile::Published: return QStringLiteral("published");
    case csr::CalibrationProfile::Physical: return QStringLiteral("physical");
    }
    return {};
}

std::optional<csr::CalibrationProfile> SolverPreferences::fromKey(const QString &key)
{
    if (key == QLatin1String("original")) return csr::CalibrationProfile::Original;
    if (key == QLatin1String("published")) return csr::CalibrationProfile::Published;
    if (key == QLatin1String("physical")) return csr::CalibrationProfile::Physical;
    return std::nullopt;
}
