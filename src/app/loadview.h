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

class CartridgeView;
class ChamberView;
class CurveChart;
class MeasurementView;
class PropertyPanel;
class QComboBox;
class QPushButton;
class QTabBar;
class ReportView;
class QTableWidget;
class QTabWidget;
class QFormLayout;
class QLabel;
class VivacityChart;

// One opened *.grtload laid out like the original program's main screen: input panels for
// cartridge/gun, projectile and propellant, the key results, and the pressure/energy/velocity chart.
class LoadView : public QWidget {
    Q_OBJECT
public:
    explicit LoadView(QWidget *parent = nullptr);

    [[nodiscard]] bool load(const QString &path, QString *error = nullptr);
    // Shows a freshly built load that has no file yet (it counts as modified until saved).
    void setNewLoad(grt::LoadFile file);
    [[nodiscard]] bool save(const QString &path, QString *error = nullptr);

    [[nodiscard]] const QString &path() const noexcept { return m_path; }
    [[nodiscard]] QString title() const;
    [[nodiscard]] bool isModified() const noexcept { return m_modified; }

    // Load development ladder: number of curves, the selected (focus) one, and its charge.
    [[nodiscard]] int ladderSize() const noexcept { return static_cast<int>(m_ladder.size()); }
    [[nodiscard]] int ladderFocus() const noexcept { return m_focus; }
    void selectLadderStep(int index) { setFocus(index); }
    [[nodiscard]] double focusCharge_g() const noexcept { return m_focusInput.charge_g; }

    // The database that "Select from database…" reads from (may be null; then the buttons are disabled).
    void setDatabase(const grt::Database *db);

    enum class Component { Caliber, Projectile, Propellant };
    // Replaces one component by the database record with this id and recomputes. `resetDependent`
    // also sets the values that depend on it (charge, seating depth, barrel) to their defaults.
    bool applyRecord(Component component, int id, bool resetDependent = false);

    [[nodiscard]] ReportView *report() const noexcept { return m_report; }
    [[nodiscard]] MeasurementView *measurements() const noexcept { return m_measurements; }
    // Stores a new record in the (own part of the) database; returns its id or -1. Set by the main window.
    using RecordSaver = std::function<int(grt::Table, const QVariantMap &)>;
    void setRecordSaver(RecordSaver saver) { m_saveRecord = std::move(saver); }
    // "Fit to measurements": fits the propellant to the Measurements tab (dialog).
    void fitPowder();

    // Results, Measurements, Report (the last one used is remembered).
    void showResultTab(int index);
    // Values and rendered diagrams for the report templates.
    [[nodiscard]] grt::report::Data reportData() const;

    // Draws the original program's curves dashed and shows deltas in the summary.
    void setReference(std::optional<grt::ReferenceRun> ref);
    [[nodiscard]] bool hasReference() const noexcept { return m_reference.has_value(); }

signals:
    void modifiedChanged(bool modified);

private:
    enum SectionId { Caliber, Gun, Projectile, Propellant };

    [[nodiscard]] grt::EntryList &section(int id);
    void rebuildPanels();
    void onValueEdited(int sectionId, const QString &key, const QString &text);
    void recompute();
    void setModified(bool m);
    void applyChartUnits();
    void updateLadderUi();
    void setFocus(int index);
    void chooseRecord(Component component);
    void updateHeaders();
    [[nodiscard]] QVariantMap caliberRecord() const;
    void showResult();
    [[nodiscard]] QString headingHtml() const;
    void loadMeasurement();
    void storeMeasurement();
    void addMeasurementReport(grt::report::Data &d) const;

    grt::LoadFile m_file;
    grt::SimResult m_result;                    // the selected (focus) ladder step
    std::vector<grt::LadderStep> m_ladder;
    grt::SimInput m_focusInput;                 // the input with the focus step's charge
    int m_focus{0};
    grt::SimInput m_input;
    std::optional<grt::ReferenceRun> m_reference;
    QString m_path;
    bool m_modified{false};

    const grt::Database *m_db{nullptr};
    QTabWidget *m_inputTabs;
    std::array<QLabel *, 3> m_itemLabels{};
    std::array<QPushButton *, 3> m_selectButtons{};
    PropertyPanel *m_cartridgePanel;
    QTabWidget *m_resultTabs;
    ReportView *m_report;
    MeasurementView *m_measurements;
    bool m_hasMeasurement{false};
    RecordSaver m_saveRecord;  // the file has a <Measurement> block
    QTabWidget *m_drawings;
    CartridgeView *m_cartridge;
    ChamberView *m_chamber;
    PropertyPanel *m_projectilePanel;
    PropertyPanel *m_propellantPanel;
    QLabel *m_heading;
    QLabel *m_status;
    QFormLayout *m_leftResults;
    QFormLayout *m_rightResults;
    QLabel *m_comparison;
    VivacityChart *m_vivacity;
    CurveChart *m_chart;
    QComboBox *m_xAxis;
    QTabBar *m_ladderTabs;
    QTableWidget *m_ladderTable;
};
