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
class QPushButton;
class QStackedWidget;
class QualityRow;
class QTabWidget;

// Two steps: first pick a caliber, a bullet and a powder from the database; "Next" then shows the charge and gun data with the
// defaults of that selection, which can be changed freely (they are only filled in when the step is entered with another selection).
// The result is a ready-to-simulate load.
class NewLoadDialog : public QDialog
{
    Q_OBJECT
public:
    NewLoadDialog(const grt::Database &db, QWidget *parent = nullptr);

    [[nodiscard]] grt::LoadFile result() const;

    // Programmatic selection (used by tests); returns false if a record is not listed. Goes on to the second step.
    bool select(int caliberId, int projectileId, int propellantId);

private:
    void onSelectionChanged();
    void goNext();
    void goBack();
    void applyDefaults();
    void updateSummary();
    [[nodiscard]] QWidget *makeSummary(std::array<QLabel *, 3> &values);

    const grt::Database &m_db;
    std::array<TableBrowser *, 3> m_browsers{};
    QStackedWidget *m_pages;
    QTabWidget *m_tabs;
    QCheckBox *m_matchDiameter;
    QualityRow *m_quality{nullptr};  // third list: the fit quality of the selected powder
    QDoubleSpinBox *m_charge;
    QDoubleSpinBox *m_barrel;
    QDoubleSpinBox *m_depth;
    QDoubleSpinBox *m_twist;
    QDoubleSpinBox *m_temperature;
    std::array<QLabel *, 3> m_summary{};    // second step: what was selected (caliber, projectile, propellant)
    std::array<QLabel *, 3> m_selection{};  // first step: the same, below the lists
    QLabel *m_chargeGrains;
    QDialogButtonBox *m_buttons;
    QPushButton *m_back;
    QPushButton *m_next;  // "Next", on the second step "Create load"
    std::optional<QVariantMap> m_caliber, m_projectile, m_propellant;
    bool m_temperatureSet{false};
    std::array<int, 3> m_defaultsFor{-1, -1, -1};  // the records the data fields were last filled for
};
