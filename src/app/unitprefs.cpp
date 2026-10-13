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

#include "unitprefs.h"
#include "log.h"

#include <QSettings>

using namespace csr::units;

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
    s.setValue(QStringLiteral("units/system"), m_system == System::Metric     ? QStringLiteral("metric")
                                               : m_system == System::Imperial ? QStringLiteral("imperial")
                                                                              : QStringLiteral("original"));
    s.remove(QStringLiteral("units/field"));
    s.beginGroup(QStringLiteral("units/field"));
    for (auto it = m_fieldUnits.cbegin(); it != m_fieldUnits.cend(); ++it) s.setValue(it.key(), it.value());
    s.endGroup();
}

void UnitPreferences::setSystem(System system)
{
    if (m_system == system) return;
    LOG_Info() << "Unit system changed to" << systemName(system);
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
