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

#include "measurement.h"
#include "chronographimportdialog.h"
#include "measurementchart.h"
#include "solver.h"

#include <QWidget>

#include <functional>
#include <map>
#include <vector>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

// The "Measurements" tab: chronograph (and pressure) results per charge with their statistics, compared with the
// simulated velocity and peak pressure, and a chart over the charge. Stored in the load file's appendix like the
// original program does (<Measurement>).
class MeasurementView : public QWidget
{
    Q_OBJECT
public:
    // Simulates the load with another charge [g].
    using Predictor = std::function<csr::SimResult(double charge_g)>;

    explicit MeasurementView(QWidget *parent = nullptr);

    // All measurement series of the load (the original has one <Measurement> block per series); `current` is shown.
    void setMeasurements(std::vector<csr::Measurement> series, int current = 0);
    [[nodiscard]] const std::vector<csr::Measurement> &allMeasurements() const noexcept { return m_series; }
    // The shown series.
    [[nodiscard]] const csr::Measurement &measurement() const noexcept { return m_series[static_cast<std::size_t>(m_index)]; }
    [[nodiscard]] int currentSeries() const noexcept { return m_index; }
    void selectSeries(int index);
    void addSeries();
    void removeSeries();

    void setPredictor(Predictor predictor);
    // Charges offered by "Add charge" / "Add ladder charges": the selected charge first, then the ladder steps.
    void setSuggestedCharges(std::vector<double> charges_g);

    // Index of the charge whose shots are shown (-1: none).
    [[nodiscard]] int currentCharge() const;
    void selectCharge(int index);

    void addCharge();
    void addLadderCharges();
    void removeCharge();
    void addShot();
    void removeShots();
    // Reads shots from text: one shot per line, velocity and optionally pressure (in the shown units), separated by
    // tabs, semicolons or spaces; lines without a number are skipped. Returns the number of shots added.
    int pasteShots(const QString &text);
    // "Import chronograph file…": asks for a MagnetoSpeed / Labradar file and the charges of its series.
    void importChronograph();
    // Adds a charge with its shots for every entry; returns the number of charges added.
    int importCharges(const std::vector<ImportedCharge> &charges);

signals:
    void edited();
    void seriesChanged();  // another series is shown (the chart and the report follow it)
    void fitRequested();   // "Fit to measurements…"

protected:
    void showEvent(QShowEvent *event) override;

private:
    struct Simulated
    {
        bool ok{false};
        double velocity_ms{};
        double pressure_bar{};
    };

    [[nodiscard]] Simulated simulated(double charge_g);
    [[nodiscard]] csr::Measurement &data() noexcept { return m_series[static_cast<std::size_t>(m_index)]; }
    [[nodiscard]] static csr::Measurement emptySeries(int number);
    void refreshSeriesBox();
    void refreshAll();
    void refreshCharges();
    void refreshShots();
    void refreshChart();
    void updateButtons();
    void onChargeChanged(int row, int column);
    void onShotChanged(int row, int column);
    void changed();

    std::vector<csr::Measurement> m_series{emptySeries(1)};  // never empty
    int m_index{0};
    Predictor m_predictor;
    std::vector<double> m_suggested;
    std::map<long long, Simulated> m_cache;  // by charge in micrograms
    bool m_updating{false};
    bool m_stale{false};  // simulated values were skipped while hidden

    QComboBox *m_seriesBox;
    QPushButton *m_newSeries;
    QPushButton *m_removeSeries;
    QLineEdit *m_title;
    QTableWidget *m_charges;
    QTableWidget *m_shots;
    QLineEdit *m_note;
    QLabel *m_shotsCaption;
    QLabel *m_hint;
    QPushButton *m_addCharge;
    QPushButton *m_addLadder;
    QPushButton *m_removeCharge;
    QPushButton *m_addShot;
    QPushButton *m_removeShot;
    QPushButton *m_paste;
    QPushButton *m_import;
    QPushButton *m_fitButton;
    MeasurementChart *m_chart;
};
