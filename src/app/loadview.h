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

#pragma once

#include "database.h"
#include "loadfile.h"
#include "reference.h"
#include "report.h"
#include "solver.h"

#include <QWidget>

#include <array>
#include <functional>
#include <optional>
#include <vector>

class CartridgeView;
class ChamberView;
class CurveChart;
class MeasurementView;
class PhotoView;
class PropertyPanel;
class QComboBox;
class QPushButton;
class QTabBar;
class ReportView;
class QTableWidget;
class ChargeView;
class QTabWidget;
class QFormLayout;
class QLabel;
class VivacityChart;

// One opened *.csrload (or the original's *.grtload) laid out like the original program's main screen: input panels for
// cartridge/gun, projectile and propellant, the key results, and the pressure/energy/velocity chart.
class LoadView : public QWidget
{
    Q_OBJECT
public:
    explicit LoadView(QWidget *parent = nullptr);

    [[nodiscard]] bool load(const QString &path, QString *error = nullptr);
    // Shows a freshly built load that has no file yet (it counts as modified until saved).
    void setNewLoad(csr::LoadFile file);
    [[nodiscard]] bool save(const QString &path, QString *error = nullptr);

    [[nodiscard]] const QString &path() const noexcept { return m_path; }
    [[nodiscard]] QString title() const;
    [[nodiscard]] bool isModified() const noexcept { return m_modified; }
    // The load's file is the original program's *.grtload (Settings > Solver can simulate those with GRT Original).
    [[nodiscard]] bool isGrtFile() const;

    // Load development ladder: number of curves, the selected (focus) one, and its charge.
    [[nodiscard]] int ladderSize() const noexcept { return static_cast<int>(m_ladder.size()); }
    [[nodiscard]] int ladderFocus() const noexcept { return m_focus; }
    [[nodiscard]] bool overpressureShown() const;
    void selectLadderStep(int index) { setFocus(index); }
    [[nodiscard]] double focusCharge_g() const noexcept { return m_focusInput.charge_g; }
    // The simulation result shown (the focused ladder step).
    [[nodiscard]] const csr::SimResult &result() const noexcept { return m_result; }

    // The database that the "Select ..." buttons read from (may be null; then the buttons are disabled).
    void setDatabase(const csr::Database *db);

    enum class Component
    {
        Caliber,
        Projectile,
        Propellant
    };
    // Replaces one component by the database record with this id and recomputes. `resetDependent`
    // also sets the values that depend on it (charge, seating depth, barrel) to their defaults.
    bool applyRecord(Component component, int id, bool resetDependent = false);

    [[nodiscard]] ReportView *report() const noexcept { return m_report; }
    [[nodiscard]] MeasurementView *measurements() const noexcept { return m_measurements; }
    // Stores a new record in the (own part of the) database; returns its id or -1. Set by the main window.
    using RecordSaver = std::function<int(csr::Table, const QVariantMap &)>;
    void setRecordSaver(RecordSaver saver) { m_saveRecord = std::move(saver); }
    // "Fit to measurements": fits the propellant to the Measurements tab (dialog).
    void fitPowder();

    // Results, Measurements, Report (the last one used is remembered).
    void showResultTab(int index);
    // Cartridge && gun, Projectile, Propellant.
    void showInputTab(int index);
    // Values and rendered diagrams for the report templates.
    [[nodiscard]] csr::report::Data reportData() const;

    // Draws the original program's curves dashed and shows deltas in the summary.
    void setReference(std::optional<csr::ReferenceRun> ref);
    [[nodiscard]] bool hasReference() const noexcept { return m_reference.has_value(); }

    // Undo / redo of the edits of this load (values, replaced components, fit, measurements). Every step keeps a copy of the
    // file as it was before the edit; the texts name the edit for the menu ("Undo Charge").
    [[nodiscard]] bool canUndo() const noexcept { return !m_undo.empty(); }
    [[nodiscard]] bool canRedo() const noexcept { return !m_redo.empty(); }
    [[nodiscard]] QString undoText() const { return m_undo.empty() ? QString() : m_undo.back().text; }
    [[nodiscard]] QString redoText() const { return m_redo.empty() ? QString() : m_redo.back().text; }
    void undo();
    void redo();

signals:
    void modifiedChanged(bool modified);
    void undoStateChanged();

private:
    enum SectionId
    {
        Caliber,
        Gun,
        Projectile,
        Propellant
    };

    [[nodiscard]] csr::EntryList &section(int id);
    void rebuildPanels();
    void updateOutdatedNotice();  // shows or hides the banner for powder values that differ from the database record
    void offerPowderUpdate();     // asks whether the load takes the current values of the database record
    void offerPhysicalBurn();     // asks whether the load takes the Physical Model's burn values of its powder
    void onValueEdited(int sectionId, const QString &key, const QString &text);
    void recompute();
    void setModified(bool m);
    // Remembers the file as it is now, to be called before an edit changes it.
    void pushUndo(const QString &text);
    void clearUndo();
    void restore(const csr::LoadFile &file);
    void applyChartUnits();
    void updateLadderUi();
    void setFocus(int index);
    void chooseRecord(Component component);
    void updateHeaders();
    void updatePhoto();
    [[nodiscard]] QVariantMap caliberRecord() const;
    void showResult();
    // The constants every simulation of this load uses (the chosen model, or GRT Original for a *.grtload, see SolverPreferences).
    [[nodiscard]] const csr::Calibration &calibration() const;
    // The results of the ladder steps other than the focused one (drawn faintly in the chart).
    [[nodiscard]] std::vector<csr::SimResult> otherLadderResults() const;
    // "Maker Name" of the propellant.
    [[nodiscard]] QString propellantName() const;
    [[nodiscard]] QString headingHtml() const;
    // Shows the measurement series of the file's appendix (`series`: the one to select).
    void loadMeasurement(int series = 0);
    void storeMeasurement();
    void addMeasurementReport(csr::report::Data &d) const;

    csr::LoadFile m_file;
    csr::SimResult m_result;  // the selected (focus) ladder step
    std::vector<csr::LadderStep> m_ladder;
    csr::SimInput m_focusInput;  // the input with the focus step's charge
    int m_focus{0};
    csr::SimInput m_input;
    std::optional<csr::ReferenceRun> m_reference;
    QString m_path;
    bool m_modified{false};

    struct UndoStep
    {
        csr::LoadFile file;  // the state to return to
        QString text;        // what the step undoes
    };
    static constexpr std::size_t kMaxUndoSteps = 100;
    std::vector<UndoStep> m_undo, m_redo;
    int m_cleanDepth{0};  // m_undo.size() of the saved state; -1: the saved state is no longer reachable

    const csr::Database *m_db{nullptr};
    QTabWidget *m_inputTabs;
    std::array<QLabel *, 3> m_itemLabels{};
    std::array<QPushButton *, 3> m_selectButtons{};
    PropertyPanel *m_cartridgePanel;
    QTabWidget *m_resultTabs;
    ReportView *m_report;
    MeasurementView *m_measurements;
    bool m_hasMeasurement{false};  // the file has a <Measurement> block
    RecordSaver m_saveRecord;
    QTabWidget *m_drawings;
    CartridgeView *m_cartridge;
    ChamberView *m_chamber;
    PropertyPanel *m_projectilePanel;
    PropertyPanel *m_propellantPanel;
    ChargeView *m_chargeView;  // fourth input tab: charge and ladder in large fields
    PhotoView *m_photo;
    int m_photoTab{-1};
    QLabel *m_heading;
    QLabel *m_status;
    QWidget *m_overpressure;  // warning banner: the maximum pressure is above the permitted one
    QLabel *m_overpressureText;
    QWidget *m_burnNotice;  // orange banner: the Physical Model has no burn values for this load (less accurate)
    QLabel *m_burnNoticeText;
    QWidget *m_outdatedNotice;  // orange banner: the powder values of the load differ from the database record of that powder
    QLabel *m_outdatedNoticeText;
    int m_outdatedRecordId{0};
    std::array<QLabel *, 16> m_values{};  // the results: 4 cards, then 4 + 3 + 5 rows in three groups
    QLabel *m_comparison;
    VivacityChart *m_vivacity;
    CurveChart *m_chart;
    QComboBox *m_xAxis;
    QTabBar *m_ladderTabs;
    QTableWidget *m_ladderTable;
};
