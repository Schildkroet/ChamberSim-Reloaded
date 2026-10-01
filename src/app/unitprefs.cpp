#include "unitprefs.h"

#include <QSettings>

using namespace grt::units;

UnitPreferences &UnitPreferences::instance()
{
    static UnitPreferences prefs;
    return prefs;
}

UnitPreferences::UnitPreferences()
{
    load();
}

void UnitPreferences::load()
{
    QSettings s;
    const QString sys = s.value(QStringLiteral("units/system")).toString();
    m_system = sys == u"metric" ? System::Metric : sys == u"imperial" ? System::Imperial : System::Original;
    m_fieldUnits.clear();
    s.beginGroup(QStringLiteral("units/field"));
    for (const QString &key : s.allKeys()) m_fieldUnits.insert(key, s.value(key).toString());
    s.endGroup();
}

void UnitPreferences::save() const
{
    QSettings s;
    s.setValue(QStringLiteral("units/system"),
               m_system == System::Metric ? QStringLiteral("metric") : m_system == System::Imperial ? QStringLiteral("imperial") : QStringLiteral("original"));
    s.remove(QStringLiteral("units/field"));
    s.beginGroup(QStringLiteral("units/field"));
    for (auto it = m_fieldUnits.cbegin(); it != m_fieldUnits.cend(); ++it) s.setValue(it.key(), it.value());
    s.endGroup();
}

void UnitPreferences::setSystem(System system)
{
    if (m_system == system) return;
    m_system = system;
    save();
    emit changed();
}

const Unit &UnitPreferences::unitFor(Quantity q) const
{
    return systemUnit(q, m_system);
}

const Unit &UnitPreferences::unitForField(const QString &fieldKey, Quantity q) const
{
    if (const auto it = m_fieldUnits.constFind(fieldKey); it != m_fieldUnits.constEnd())
        if (const Unit *u = findUnit(q, it.value())) return *u;
    return unitFor(q);
}

bool UnitPreferences::hasOverride(const QString &fieldKey) const
{
    return m_fieldUnits.contains(fieldKey);
}

void UnitPreferences::setFieldUnit(const QString &fieldKey, Quantity q, const QString &unitId)
{
    if (unitId.isEmpty() || !findUnit(q, unitId)) m_fieldUnits.remove(fieldKey);
    else m_fieldUnits.insert(fieldKey, unitId);
    save();
    emit changed();
}

void UnitPreferences::resetFieldUnits()
{
    if (m_fieldUnits.isEmpty()) return;
    m_fieldUnits.clear();
    save();
    emit changed();
}
