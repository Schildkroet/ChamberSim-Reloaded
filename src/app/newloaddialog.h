#pragma once

#include "database.h"
#include "loadbuilder.h"
#include "loadfile.h"
#include "tablebrowser.h"

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QLabel;
class QTabWidget;

// Pick a caliber, a bullet and a powder from the database, set the charge and gun data, and
// get a ready-to-simulate load.
class NewLoadDialog : public QDialog {
    Q_OBJECT
public:
    NewLoadDialog(const grt::Database &db, QWidget *parent = nullptr);

    [[nodiscard]] grt::LoadFile result() const;

    // Programmatic selection (used by tests); returns false if a record is not listed.
    bool select(int caliberId, int projectileId, int propellantId);

private:
    void onSelectionChanged();
    void applyDefaults();
    void updateSummary();

    const grt::Database &m_db;
    std::array<TableBrowser *, 3> m_browsers{};
    QTabWidget *m_tabs;
    QCheckBox *m_matchDiameter;
    QDoubleSpinBox *m_charge;
    QDoubleSpinBox *m_barrel;
    QDoubleSpinBox *m_depth;
    QDoubleSpinBox *m_twist;
    QDoubleSpinBox *m_temperature;
    QLabel *m_summary;
    QLabel *m_chargeGrains;
    QDialogButtonBox *m_buttons;
    std::optional<QVariantMap> m_caliber, m_projectile, m_propellant;
    bool m_temperatureSet{false};
};
