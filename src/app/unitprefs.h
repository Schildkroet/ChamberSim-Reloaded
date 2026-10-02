#pragma once

#include "units.h"

#include <QHash>
#include <QObject>

// The user's unit choices: one preset system for everything, plus per-field overrides (the "ruler"
// of the original program). Persisted in the application settings. Everything stored in files and
// in the database stays in the base units; this only changes what is shown and typed.
class UnitPreferences : public QObject
{
    Q_OBJECT
public:
    static UnitPreferences &instance();

    [[nodiscard]] grt::units::System system() const noexcept { return m_system; }
    void setSystem(grt::units::System system);

    // Unit of the preset system for a quantity (used for results, chart axes, dialogs).
    [[nodiscard]] const grt::units::Unit &unitFor(grt::units::Quantity q) const;

    // Unit of a field: its override if there is one, else the system's. `fieldKey` is "section/name".
    [[nodiscard]] const grt::units::Unit &unitForField(const QString &fieldKey, grt::units::Quantity q) const;
    [[nodiscard]] bool hasOverride(const QString &fieldKey) const;
    void setFieldUnit(const QString &fieldKey, grt::units::Quantity q, const QString &unitId);  // empty id: reset
    void resetFieldUnits();

signals:
    void changed();

private:
    UnitPreferences();
    void load();
    void save() const;

    grt::units::System m_system{grt::units::System::Original};
    QHash<QString, QString> m_fieldUnits;  // "section/name" -> unit id
};
