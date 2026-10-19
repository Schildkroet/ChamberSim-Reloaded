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

#include "measurementview.h"

#include "buttonstyle.h"
#include "layoutstate.h"
#include "log.h"
#include "unitprefs.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScopedValueRollback>
#include <QSettings>
#include <QShortcut>
#include <QSplitter>
#include <QStyle>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <set>

using namespace Qt::StringLiterals;
using csr::units::Quantity;

namespace
{

enum ChargeColumn
{
    Name,
    Charge,
    Shots,
    Velocity,
    VelocitySd,
    VelocityEs,
    SimVelocity,
    VelocityDelta,
    Pressure,
    SimPressure,
    PressureDelta,
    ColumnCount
};

const csr::units::Unit &unitOf(Quantity q)
{
    return UnitPreferences::instance().unitFor(q);
}

QString symbol(Quantity q)
{
    return QString::fromUtf8(unitOf(q).symbol);
}

QString shownValue(double base, Quantity q, int decimals = -1)
{
    return csr::units::format(base, unitOf(q), decimals);
}

QTableWidgetItem *readOnly(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

QTableWidgetItem *editable(const QString &text, bool right = true)
{
    auto *item = new QTableWidgetItem(text);
    if (right) item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

// The name a new charge gets: "12.5 g" in the user's unit.
QString chargeName(double charge_g)
{
    return QStringLiteral("%1 %2").arg(shownValue(charge_g, Quantity::BulletMass), symbol(Quantity::BulletMass));
}

bool isListed(const csr::Measurement &m, double charge_g)
{
    return std::ranges::any_of(m.charges, [&](const csr::MeasuredCharge &c) { return std::abs(c.charge_g() - charge_g) < 1e-6; });
}

QString percent(double value, double reference)
{
    if (reference <= 0.0 || value <= 0.0) return QStringLiteral("–");
    return QStringLiteral("%1 %").arg(QLocale().toString(100.0 * (value / reference - 1.0), 'f', 1));
}

}  // namespace

MeasurementView::MeasurementView(QWidget *parent)
    : QWidget(parent)
    , m_seriesBox(new QComboBox)
    , m_newSeries(new QPushButton(tr("New series")))
    , m_removeSeries(new QPushButton(tr("Remove series")))
    , m_title(new QLineEdit)
    , m_charges(new QTableWidget(0, ColumnCount))
    , m_shots(new QTableWidget(0, 2))
    , m_note(new QLineEdit)
    , m_shotsCaption(new QLabel)
    , m_hint(new QLabel)
    , m_addCharge(new QPushButton(tr("Add charge")))
    , m_addLadder(new QPushButton(tr("Add ladder charges")))
    , m_removeCharge(new QPushButton(tr("Remove charge")))
    , m_addShot(new QPushButton(tr("Add shot")))
    , m_removeShot(new QPushButton(tr("Remove shot")))
    , m_paste(new QPushButton(tr("Paste shots")))
    , m_import(new QPushButton(tr("Import chronograph file")))
    , m_fitButton(new QPushButton(tr("Fit to measurements")))
    , m_chart(new MeasurementChart)
{
    m_title->setPlaceholderText(tr("Title of the measurement series"));
    m_seriesBox->setToolTip(tr("The measurement series of this load (for example one per range session or rifle)"));
    m_newSeries->setToolTip(tr("Start another, empty measurement series"));
    m_removeSeries->setToolTip(tr("Remove the shown series with all its charges (the last one stays, but is emptied)"));
    m_addCharge->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    m_addCharge->setToolTip(tr("Add a charge (the load's selected charge, or the next one of the ladder)"));
    m_addLadder->setToolTip(tr("Add every charge of the load development ladder that is not listed yet"));
    m_removeCharge->setToolTip(tr("Remove the selected charge with all its shots"));
    m_addShot->setToolTip(tr("Add a shot to the selected charge"));
    m_removeShot->setToolTip(tr("Remove the selected shots (Del)"));
    m_fitButton->setToolTip(tr("Change the propellant's model values until the simulation agrees with the measured charges"));
    m_paste->setToolTip(tr("Add shots from the clipboard: one shot per line, velocity and optionally the peak "
                           "pressure in the shown units (e.g. copied from a chronograph export or a spreadsheet)"));
    m_import->setToolTip(
        tr("Read the shots of a Garmin Xero (FIT or CSV), MagnetoSpeed or Labradar file (Labradar: the \"Report.csv\" of a series)"));
    markPrimary(m_addCharge);

    m_charges->setObjectName(u"charges"_s);
    m_shots->setObjectName(u"shots"_s);
    m_charges->verticalHeader()->hide();
    m_charges->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_charges->setSelectionMode(QAbstractItemView::SingleSelection);
    m_charges->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_charges->horizontalHeader()->setSectionResizeMode(Name, QHeaderView::Stretch);
    m_charges->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
    m_shots->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_shots->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_shots->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
    m_note->setPlaceholderText(tr("Note (rifle, chronograph, conditions, …)"));
    m_hint->setWordWrap(true);
    m_hint->setText(tr("<i>Record chronograph results here: add a charge, then its shots (velocity and, if measured, the "
                       "peak pressure). The statistics are compared with the simulation of this load.</i>"));

    auto *bar = new QHBoxLayout;
    bar->addWidget(m_addCharge);
    bar->addWidget(m_addLadder);
    bar->addWidget(m_removeCharge);
    bar->addWidget(m_import);
    bar->addWidget(m_fitButton);
    bar->addStretch(1);

    auto *seriesBar = new QHBoxLayout;
    seriesBar->addWidget(new QLabel(tr("Series:")));
    seriesBar->addWidget(m_seriesBox);
    seriesBar->addWidget(m_newSeries);
    seriesBar->addWidget(m_removeSeries);
    seriesBar->addSpacing(16);
    seriesBar->addWidget(new QLabel(tr("Title:")));
    seriesBar->addWidget(m_title, 1);

    auto *shotBar = new QHBoxLayout;
    shotBar->addWidget(m_addShot);
    shotBar->addWidget(m_removeShot);
    shotBar->addWidget(m_paste);
    shotBar->addStretch(1);

    auto *shotPanel = new QWidget;
    auto *sl = new QVBoxLayout(shotPanel);
    sl->setContentsMargins(0, 0, 0, 0);
    sl->addWidget(m_shotsCaption);
    sl->addWidget(m_note);
    sl->addLayout(shotBar);
    sl->addWidget(m_shots, 1);

    auto *bottom = new QSplitter(Qt::Horizontal);
    bottom->addWidget(shotPanel);
    bottom->addWidget(m_chart);
    bottom->setStretchFactor(0, 0);
    bottom->setStretchFactor(1, 1);
    bottom->setSizes({330, 700});
    rememberSplitter(bottom, QStringLiteral("measurementBottom"));

    auto *top = new QWidget;
    auto *tl = new QVBoxLayout(top);
    tl->setContentsMargins(0, 0, 0, 0);
    tl->addLayout(seriesBar);
    tl->addLayout(bar);
    tl->addWidget(m_hint);
    tl->addWidget(m_charges, 1);

    auto *split = new QSplitter(Qt::Vertical);
    split->addWidget(top);
    split->addWidget(bottom);
    split->setSizes({260, 480});
    rememberSplitter(split, QStringLiteral("measurement"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addWidget(split);

    connect(m_addCharge, &QPushButton::clicked, this, &MeasurementView::addCharge);
    connect(m_addLadder, &QPushButton::clicked, this, &MeasurementView::addLadderCharges);
    connect(m_removeCharge, &QPushButton::clicked, this, &MeasurementView::removeCharge);
    connect(m_addShot, &QPushButton::clicked, this, &MeasurementView::addShot);
    connect(m_removeShot, &QPushButton::clicked, this, &MeasurementView::removeShots);
    connect(m_fitButton, &QPushButton::clicked, this, &MeasurementView::fitRequested);
    connect(m_newSeries, &QPushButton::clicked, this, &MeasurementView::addSeries);
    connect(m_removeSeries, &QPushButton::clicked, this, &MeasurementView::removeSeries);
    connect(m_seriesBox, &QComboBox::currentIndexChanged, this,
            [this](int i)
            {
                if (!m_updating) selectSeries(i);
            });
    connect(m_import, &QPushButton::clicked, this, &MeasurementView::importChronograph);
    connect(m_paste, &QPushButton::clicked, this, [this] { pasteShots(QApplication::clipboard()->text()); });
    auto *del = new QShortcut(QKeySequence::Delete, m_shots, nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
    connect(del, &QShortcut::activated, this, &MeasurementView::removeShots);
    connect(m_charges, &QTableWidget::cellChanged, this, &MeasurementView::onChargeChanged);
    connect(m_shots, &QTableWidget::cellChanged, this, &MeasurementView::onShotChanged);
    connect(m_charges, &QTableWidget::itemSelectionChanged, this,
            [this]
            {
                if (m_updating) return;
                refreshShots();
                updateButtons();
            });
    connect(m_title, &QLineEdit::editingFinished, this,
            [this]
            {
                if (m_title->text() == data().title) return;
                data().title = m_title->text();
                refreshSeriesBox();
                emit edited();
            });
    connect(m_note, &QLineEdit::editingFinished, this,
            [this]
            {
                const int i = currentCharge();
                if (i < 0 || m_note->text() == data().charges[static_cast<std::size_t>(i)].note) return;
                data().charges[static_cast<std::size_t>(i)].note = m_note->text();
                emit edited();
            });
    connect(&UnitPreferences::instance(), &UnitPreferences::changed, this, &MeasurementView::refreshAll);
    refreshAll();
}

void MeasurementView::setMeasurements(std::vector<csr::Measurement> series, int current)
{
    m_series = std::move(series);
    if (m_series.empty()) m_series.push_back(emptySeries(1));
    m_index = std::clamp(current, 0, static_cast<int>(m_series.size()) - 1);
    refreshSeriesBox();
    refreshAll();
    if (!data().charges.empty()) selectCharge(0);
}

void MeasurementView::setMeasurement(const csr::Measurement &m)
{
    setMeasurements({m});
}

csr::Measurement MeasurementView::emptySeries(int number)
{
    return {.title = number > 1 ? tr("MEASUREMENTS %1").arg(number) : u"MEASUREMENTS"_s, .charges = {}, .attributes = {}};
}

void MeasurementView::refreshSeriesBox()
{
    const QScopedValueRollback updating(m_updating, true);
    m_seriesBox->clear();
    for (std::size_t i = 0; i < m_series.size(); ++i)
        m_seriesBox->addItem(m_series[i].title.isEmpty() ? tr("Series %1").arg(i + 1) : m_series[i].title);
    m_seriesBox->setCurrentIndex(m_index);
    m_seriesBox->setEnabled(m_series.size() > 1);
    m_removeSeries->setEnabled(m_series.size() > 1 || !data().charges.empty());
}

void MeasurementView::selectSeries(int index)
{
    if (index < 0 || index >= static_cast<int>(m_series.size()) || index == m_index) return;
    m_index = index;
    refreshSeriesBox();
    refreshAll();
    if (!data().charges.empty()) selectCharge(0);
    emit seriesChanged();
}

void MeasurementView::addSeries()
{
    m_series.push_back(emptySeries(static_cast<int>(m_series.size()) + 1));
    m_index = static_cast<int>(m_series.size()) - 1;
    refreshSeriesBox();
    refreshAll();
    emit seriesChanged();
    emit edited();
}

void MeasurementView::removeSeries()
{
    if (m_series.size() == 1) data() = emptySeries(1);  // the load keeps one (empty) series
    else m_series.erase(m_series.begin() + m_index);
    m_index = std::min(m_index, static_cast<int>(m_series.size()) - 1);
    refreshSeriesBox();
    refreshAll();
    if (!data().charges.empty()) selectCharge(0);
    emit seriesChanged();
    emit edited();
}

void MeasurementView::setPredictor(Predictor predictor)
{
    m_predictor = std::move(predictor);
    m_cache.clear();
    refreshCharges();
    refreshChart();
}

void MeasurementView::setSuggestedCharges(std::vector<double> charges_g)
{
    m_suggested = std::move(charges_g);
    updateButtons();
}

void MeasurementView::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_stale) return;
    m_stale = false;
    refreshCharges();
    refreshChart();
}

int MeasurementView::currentCharge() const
{
    const auto rows = m_charges->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.front().row();
}

void MeasurementView::selectCharge(int index)
{
    if (index < 0 || index >= m_charges->rowCount()) return;
    m_charges->selectRow(index);
    refreshShots();
    updateButtons();
}

MeasurementView::Simulated MeasurementView::simulated(double charge_g)
{
    if (!m_predictor || charge_g <= 0.0) return {};
    if (!isVisible())
    {  // simulating costs time: only for the tab on screen
        m_stale = true;
        return {};
    }
    const long long key = std::llround(charge_g * 1e6);
    if (const auto it = m_cache.find(key); it != m_cache.end()) return it->second;
    const csr::SimResult r = m_predictor(charge_g);
    const Simulated s{.ok = r.ok(), .velocity_ms = r.muzzleVelocity_ms, .pressure_bar = r.maxPressure_bar};
    m_cache.emplace(key, s);
    return s;
}

void MeasurementView::refreshAll()
{
    m_title->setText(data().title);
    m_charges->setHorizontalHeaderLabels({tr("Name"), tr("Charge [%1]").arg(symbol(Quantity::BulletMass)), tr("Shots"),
                                          tr("Velocity [%1]").arg(symbol(Quantity::Velocity)), tr("SD"), tr("ES"), tr("Simulated"), tr("Δ"),
                                          tr("Pressure [%1]").arg(symbol(Quantity::Pressure)), tr("Simulated"), tr("Δ")});
    m_charges->horizontalHeaderItem(Velocity)->setToolTip(tr("Mean muzzle velocity of the shots"));
    m_charges->horizontalHeaderItem(VelocitySd)->setToolTip(tr("Standard deviation of the velocity"));
    m_charges->horizontalHeaderItem(VelocityEs)->setToolTip(tr("Extreme spread of the velocity (fastest - slowest)"));
    m_charges->horizontalHeaderItem(SimVelocity)->setToolTip(tr("Muzzle velocity simulated for this charge"));
    m_charges->horizontalHeaderItem(VelocityDelta)->setToolTip(tr("Simulated velocity relative to the measured mean"));
    m_charges->horizontalHeaderItem(Pressure)->setToolTip(tr("Mean measured peak pressure"));
    m_charges->horizontalHeaderItem(SimPressure)->setToolTip(tr("Peak pressure simulated for this charge"));
    m_charges->horizontalHeaderItem(PressureDelta)->setToolTip(tr("Simulated pressure relative to the measured mean"));
    m_shots->setHorizontalHeaderLabels(
        {tr("Velocity [%1]").arg(symbol(Quantity::Velocity)), tr("Pressure [%1]").arg(symbol(Quantity::Pressure))});
    m_chart->setUnits(unitOf(Quantity::BulletMass), unitOf(Quantity::Velocity), unitOf(Quantity::Pressure));
    refreshCharges();
    refreshShots();
    refreshChart();
    updateButtons();
}

void MeasurementView::refreshCharges()
{
    const int keep = currentCharge();
    m_updating = true;
    m_charges->setRowCount(static_cast<int>(data().charges.size()));
    for (int row = 0; row < m_charges->rowCount(); ++row)
    {
        const csr::MeasuredCharge &c = data().charges[static_cast<std::size_t>(row)];
        const csr::ShotStatistics v = csr::velocityStatistics(c);
        const csr::ShotStatistics p = csr::pressureStatistics(c);
        const Simulated sim = simulated(c.charge_g());
        const auto opt = [](bool ok, const QString &text) { return ok ? text : QStringLiteral("–"); };
        m_charges->setItem(row, Name, editable(c.name, false));
        m_charges->setItem(row, Charge, editable(shownValue(c.charge_g(), Quantity::BulletMass)));
        m_charges->setItem(row, Shots, readOnly(QString::number(c.shots.size())));
        m_charges->setItem(row, Velocity, readOnly(opt(v.count > 0, shownValue(v.mean, Quantity::Velocity, 1))));
        m_charges->setItem(row, VelocitySd, readOnly(opt(v.count > 1, shownValue(v.sd, Quantity::Velocity, 1))));
        m_charges->setItem(row, VelocityEs, readOnly(opt(v.count > 1, shownValue(v.extremeSpread(), Quantity::Velocity, 1))));
        m_charges->setItem(row, SimVelocity, readOnly(opt(sim.ok, shownValue(sim.velocity_ms, Quantity::Velocity, 1))));
        m_charges->setItem(row, VelocityDelta, readOnly(sim.ok ? percent(sim.velocity_ms, v.mean) : QStringLiteral("–")));
        m_charges->setItem(row, Pressure, readOnly(opt(p.count > 0, shownValue(p.mean, Quantity::Pressure, 0))));
        m_charges->setItem(row, SimPressure, readOnly(opt(sim.ok, shownValue(sim.pressure_bar, Quantity::Pressure, 0))));
        m_charges->setItem(row, PressureDelta, readOnly(sim.ok ? percent(sim.pressure_bar, p.mean) : QStringLiteral("–")));
        if (!sim.ok && m_predictor)
            for (int col : {SimVelocity, SimPressure})
                m_charges->item(row, col)->setToolTip(tr("This charge cannot be simulated (does not fit into the case?)"));
    }
    if (keep >= 0 && keep < m_charges->rowCount()) m_charges->selectRow(keep);
    m_hint->setVisible(data().charges.empty());
    m_updating = false;
}

void MeasurementView::refreshShots()
{
    m_updating = true;
    const int i = currentCharge();
    const csr::MeasuredCharge *c = i >= 0 ? &data().charges[static_cast<std::size_t>(i)] : nullptr;
    m_shots->setRowCount(c ? static_cast<int>(c->shots.size()) : 0);
    for (int row = 0; c && row < m_shots->rowCount(); ++row)
    {
        const csr::Shot &s = c->shots[static_cast<std::size_t>(row)];
        m_shots->setItem(row, 0, editable(s.velocity_ms > 0.0 ? shownValue(s.velocity_ms, Quantity::Velocity, 1) : QString()));
        m_shots->setItem(row, 1, editable(s.pressure_bar > 0.0 ? shownValue(s.pressure_bar, Quantity::Pressure, 0) : QString()));
    }
    m_note->setText(c ? c->note : QString());
    m_note->setEnabled(c != nullptr);
    m_shotsCaption->setText(
        c ? tr("<b>Shots of %1</b> (%2 %3)")
                .arg(c->name.toHtmlEscaped(), shownValue(c->charge_g(), Quantity::BulletMass), symbol(Quantity::BulletMass))
          : tr("<b>Shots</b> – select a charge"));
    m_updating = false;
}

void MeasurementView::refreshChart()
{
    std::vector<MeasurementChart::Prediction> curve;
    if (m_predictor)
    {
        double lo = 0.0, hi = 0.0;
        bool any = false;
        const auto take = [&](double g)
        {
            if (g <= 0.0) return;
            lo = any ? std::min(lo, g) : g;
            hi = any ? std::max(hi, g) : g;
            any = true;
        };
        for (const csr::MeasuredCharge &c : data().charges) take(c.charge_g());
        if (!any && !m_suggested.empty()) take(m_suggested.front());
        if (any)
        {
            const double span = std::max(hi - lo, 0.1 * hi);
            lo = std::max(lo - 0.15 * span, 0.01);
            hi += 0.15 * span;
            constexpr int kPoints = 13;
            for (int k = 0; k < kPoints; ++k)
            {
                const double g = lo + (hi - lo) * k / (kPoints - 1);
                const Simulated s = simulated(g);
                if (s.ok) curve.push_back({.charge_g = g, .velocity_ms = s.velocity_ms, .pressure_bar = s.pressure_bar});
            }
        }
    }
    m_chart->setData(data(), curve);
}

void MeasurementView::updateButtons()
{
    const bool selected = currentCharge() >= 0;
    m_removeCharge->setEnabled(selected);
    m_addShot->setEnabled(selected);
    m_paste->setEnabled(selected);
    m_fitButton->setEnabled(
        std::ranges::any_of(data().charges, [](const csr::MeasuredCharge &c) { return csr::velocityStatistics(c).count > 0; }));
    m_removeShot->setEnabled(selected && m_shots->rowCount() > 0);
    m_addLadder->setVisible(m_suggested.size() > 1);
}

void MeasurementView::changed()
{
    const int keep = currentCharge();
    refreshCharges();
    if (keep >= 0) m_charges->selectRow(std::min(keep, m_charges->rowCount() - 1));
    refreshShots();
    refreshChart();
    updateButtons();
    emit edited();
}

void MeasurementView::addCharge()
{
    // the selected charge first; if it is already listed, the next ladder charge that is not
    double charge = m_suggested.empty() ? 0.0 : m_suggested.front();
    if (const auto it = std::ranges::find_if(m_suggested, [&](double g) { return !isListed(data(), g); }); it != m_suggested.end())
        charge = *it;
    if (charge <= 0.0 && !data().charges.empty()) charge = data().charges.back().charge_g();
    data().charges.push_back(csr::MeasuredCharge::create(chargeName(charge), charge));
    refreshCharges();
    selectCharge(static_cast<int>(data().charges.size()) - 1);
    changed();
}

void MeasurementView::addLadderCharges()
{
    bool added = false;
    std::vector<double> sorted = m_suggested;
    std::ranges::sort(sorted);
    for (double g : sorted)
    {
        if (isListed(data(), g)) continue;
        data().charges.push_back(csr::MeasuredCharge::create(chargeName(g), g));
        added = true;
    }
    if (!added) return;
    refreshCharges();
    selectCharge(0);
    changed();
}

void MeasurementView::removeCharge()
{
    const int i = currentCharge();
    if (i < 0) return;
    data().charges.erase(data().charges.begin() + i);
    refreshCharges();
    selectCharge(std::min(i, static_cast<int>(data().charges.size()) - 1));
    changed();
}

void MeasurementView::addShot()
{
    const int i = currentCharge();
    if (i < 0) return;
    data().charges[static_cast<std::size_t>(i)].shots.push_back({});
    changed();
    const int row = m_shots->rowCount() - 1;
    m_shots->setCurrentCell(row, 0);
    m_shots->editItem(m_shots->item(row, 0));
}

void MeasurementView::removeShots()
{
    const int i = currentCharge();
    if (i < 0) return;
    std::set<int> rows;
    for (const QModelIndex &idx : m_shots->selectionModel()->selectedIndexes()) rows.insert(idx.row());
    if (rows.empty() && m_shots->currentRow() >= 0) rows.insert(m_shots->currentRow());
    if (rows.empty()) return;
    auto &shots = data().charges[static_cast<std::size_t>(i)].shots;
    for (auto it = rows.rbegin(); it != rows.rend(); ++it)
        if (*it < static_cast<int>(shots.size())) shots.erase(shots.begin() + *it);
    changed();
}

int MeasurementView::pasteShots(const QString &text)
{
    const int i = currentCharge();
    if (i < 0) return 0;
    auto &shots = data().charges[static_cast<std::size_t>(i)].shots;
    int added = 0;
    static const QRegularExpression separators(QStringLiteral("[\\t;\\s]+"));
    for (const QString &line : text.split(u'\n'))
    {
        const QStringList fields = line.trimmed().split(separators, Qt::SkipEmptyParts);
        if (fields.isEmpty()) continue;
        const auto v = csr::units::parseToBase(fields[0], unitOf(Quantity::Velocity));
        if (!v || *v <= 0.0) continue;  // header or text line
        csr::Shot shot{.velocity_ms = *v};
        if (fields.size() > 1)
            if (const auto p = csr::units::parseToBase(fields[1], unitOf(Quantity::Pressure)); p && *p > 0.0) shot.pressure_bar = *p;
        shots.push_back(shot);
        ++added;
    }
    if (added > 0) changed();
    return added;
}

void MeasurementView::importChronograph()
{
    QSettings settings;
    const QString dir = settings.value(u"files/importDir"_s).toString();
    const QString path = QFileDialog::getOpenFileName(this, tr("Import chronograph file"), dir,
                                                      tr("Chronograph files (*.fit *.FIT *.csv *.CSV);;All files (*)"));
    if (path.isEmpty()) return;
    settings.setValue(u"files/importDir"_s, QFileInfo(path).absolutePath());
    QFile f(path);
    QString error;
    std::optional<csr::ChronographFile> file;
    if (f.open(QIODevice::ReadOnly)) file = csr::readChronograph(f.readAll(), &error);
    else error = f.errorString();
    if (!file)
    {
        LOG_Warn() << "Chronograph import failed:" << path << ":" << error;
        QMessageBox::warning(this, tr("Import chronograph file"),
                             tr("%1 cannot be imported: %2").arg(QDir::toNativeSeparators(path), error));
        return;
    }
    ChronographImportDialog dialog(std::move(*file), m_suggested.empty() ? 0.0 : m_suggested.front(), this);
    if (dialog.exec() == QDialog::Accepted) importCharges(dialog.selected());
}

int MeasurementView::importCharges(const std::vector<ImportedCharge> &charges)
{
    for (const ImportedCharge &c : charges)
    {
        csr::MeasuredCharge charge = csr::MeasuredCharge::create(c.series.name, c.charge_g);
        for (double v : c.series.velocities_ms) charge.shots.push_back({.velocity_ms = v});
        data().charges.push_back(std::move(charge));
    }
    if (charges.empty()) return 0;
    LOG_Info() << "Imported" << charges.size() << "chronograph series";
    refreshCharges();
    selectCharge(static_cast<int>(data().charges.size()) - 1);
    changed();
    return static_cast<int>(charges.size());
}

void MeasurementView::onChargeChanged(int row, int column)
{
    if (m_updating || row < 0 || row >= static_cast<int>(data().charges.size())) return;
    csr::MeasuredCharge &c = data().charges[static_cast<std::size_t>(row)];
    const QString text = m_charges->item(row, column)->text();
    if (column == Name)
    {
        c.name = text;
    }
    else if (column == Charge)
    {
        const auto g = csr::units::parseToBase(text, unitOf(Quantity::BulletMass));
        if (g && *g > 0.0) c.setCharge_g(*g);
    }
    else
    {
        return;
    }
    changed();
}

void MeasurementView::onShotChanged(int row, int column)
{
    const int i = currentCharge();
    if (m_updating || i < 0) return;
    auto &shots = data().charges[static_cast<std::size_t>(i)].shots;
    if (row < 0 || row >= static_cast<int>(shots.size())) return;
    const QString text = m_shots->item(row, column)->text().trimmed();
    const Quantity q = column == 0 ? Quantity::Velocity : Quantity::Pressure;
    double value = 0.0;
    if (!text.isEmpty())
    {
        const auto parsed = csr::units::parseToBase(text, unitOf(q));
        if (parsed && *parsed >= 0.0) value = *parsed;
    }
    (column == 0 ? shots[static_cast<std::size_t>(row)].velocity_ms : shots[static_cast<std::size_t>(row)].pressure_bar) = value;
    changed();
}
