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

#include "chargeview.h"
#include "unitprefs.h"
#include <QButtonGroup>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

using csr::units::Quantity;

namespace
{

const QString kChargeField = QStringLiteral("propellant/mc");
const QString kStepField = QStringLiteral("propellant/laddermc");

// A rounded panel painted from the current palette (like the result cards of the load view).
class Card : public QFrame
{
public:
    using QFrame::QFrame;

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(palette().brush(QPalette::AlternateBase));
        p.setPen(palette().color(QPalette::Mid));
        p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);
    }
};

// A small bold heading in capitals, like the groups of the results.
QLabel *caption(const QString &text)
{
    auto *l = new QLabel(text.toUpper());
    QFont f = l->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 0.85);
    f.setLetterSpacing(QFont::PercentageSpacing, 108);
    l->setFont(f);
    return l;
}

void enlarge(QWidget *w)
{
    QFont f = w->font();
    f.setPointSizeF(f.pointSizeF() * 2.0);
    w->setFont(f);
    w->setMinimumHeight(static_cast<int>(QFontMetrics(f).height() * 1.53));
    w->setMinimumWidth(150);
}

void setRange(QDoubleSpinBox *s, const csr::units::Unit &u, double maxBase)
{
    s->setDecimals(std::max(u.decimals, 1));
    s->setSuffix(QStringLiteral(" ") + QString::fromUtf8(u.symbol));
    s->setRange(0.0, csr::units::toDisplay(maxBase, u));
    s->setSingleStep(std::pow(10.0, -std::max(u.decimals, 1)));
    s->setGroupSeparatorShown(false);
}

QString shown(double base, Quantity q, const QString &field = {})
{
    const csr::units::Unit &u =
        field.isEmpty() ? UnitPreferences::instance().unitFor(q) : UnitPreferences::instance().unitForField(field, q);
    return csr::units::format(base, u) + QLatin1Char(' ') + QString::fromUtf8(u.symbol);
}

}  // namespace

ChargeView::ChargeView(QWidget *parent)
    : QWidget(parent)
    , m_charge(new QDoubleSpinBox)
    , m_steps(new QSpinBox)
    , m_step(new QDoubleSpinBox)
    , m_unitGroup(new QButtonGroup(this))
    , m_chargeOther(new QLabel)
    , m_ladderSummary(new QLabel)
    , m_table(new QTableWidget(0, 3))
{
    m_charge->setKeyboardTracking(false);
    m_step->setKeyboardTracking(false);
    m_steps->setKeyboardTracking(false);
    m_steps->setRange(1, csr::kMaxLadderSteps);
    m_steps->setSpecialValueText(tr("1 (no ladder)"));
    for (QWidget *w : {static_cast<QWidget *>(m_charge), static_cast<QWidget *>(m_steps), static_cast<QWidget *>(m_step)}) enlarge(w);

    m_chargeOther->setForegroundRole(QPalette::PlaceholderText);
    m_ladderSummary->setForegroundRole(QPalette::PlaceholderText);
    m_ladderSummary->setWordWrap(true);
    m_table->setToolTip(tr("The charge is the highest of the ladder; every further step has one charge step less powder. "
                           "Click a row to show its curve."));

    m_table->setHorizontalHeaderLabels({tr("Charge"), tr("Max. pressure"), tr("Velocity")});
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->setFrameShape(QFrame::NoFrame);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->horizontalHeader()->setHighlightSections(false);

    // grain / gram: the unit of the charge and of the charge step together (the same per-field units as in the propellant table)
    const std::array<std::pair<const char *, QString>, 2> units{{{"grain", tr("grain")}, {"g", tr("gram")}}};
    auto *unitRow = new QHBoxLayout;
    unitRow->setSpacing(0);
    for (std::size_t i = 0; i < units.size(); ++i)
    {
        auto *b = new QPushButton(units[i].second);
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setProperty("unitId", QString::fromLatin1(units[i].first));
        b->setToolTip(tr("Show the charge and the charge step in %1").arg(units[i].second));
        // a segmented switch: the chosen unit is filled with the accent color, the two buttons share their inner edge
        const QString radius = i == 0 ? QStringLiteral("border-top-left-radius: 6px; border-bottom-left-radius: 6px;")
                                      : QStringLiteral("border-top-right-radius: 6px; border-bottom-right-radius: 6px; border-left: none;");
        b->setStyleSheet(QStringLiteral("QPushButton { padding: 6px 22px; border: 1px solid palette(mid); background: palette(button);") +
                         radius +
                         QStringLiteral(" }"
                                        "QPushButton:hover { background: palette(midlight); }"
                                        "QPushButton:checked { background: palette(highlight); color: palette(highlighted-text);"
                                        "                      border-color: palette(highlight); font-weight: bold; }"));
        m_unitGroup->addButton(b);
        m_unitButtons[i] = b;
        unitRow->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, b] { chooseUnit(b->property("unitId").toString()); });
    }
    m_unitGroup->setExclusive(true);

    // the charge: one big field and its value in the other unit
    auto *chargeCard = new Card;
    auto *cc = new QVBoxLayout(chargeCard);
    cc->setContentsMargins(16, 12, 16, 14);
    cc->setSpacing(6);
    auto *chargeHead = new QHBoxLayout;
    chargeHead->addWidget(caption(tr("Propellant charge")), 1);
    chargeHead->addLayout(unitRow);
    cc->addLayout(chargeHead);
    cc->addWidget(m_charge);
    cc->addWidget(m_chargeOther);

    // the ladder: number of steps and the powder removed per step
    auto *ladderCard = new Card;
    auto *lc = new QVBoxLayout(ladderCard);
    lc->setContentsMargins(16, 12, 16, 14);
    lc->setSpacing(6);
    lc->addWidget(caption(tr("Load development ladder")));
    auto *fields = new QGridLayout;
    fields->setHorizontalSpacing(16);
    fields->addWidget(new QLabel(tr("Steps")), 0, 0);
    fields->addWidget(new QLabel(tr("Charge step")), 0, 1);
    fields->addWidget(m_steps, 1, 0);
    fields->addWidget(m_step, 1, 1);
    fields->setColumnStretch(0, 1);
    fields->setColumnStretch(1, 1);
    lc->addLayout(fields);
    lc->addWidget(m_ladderSummary);

    auto *tableCard = new Card;
    auto *tc = new QVBoxLayout(tableCard);
    tc->setContentsMargins(10, 10, 10, 8);
    tc->setSpacing(4);
    tc->addWidget(caption(tr("Ladder results")));
    tc->addWidget(m_table, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 10, 8, 8);
    layout->setSpacing(10);
    layout->addWidget(chargeCard);
    layout->addWidget(ladderCard);
    layout->addWidget(tableCard, 1);

    applyUnits();
    connect(&UnitPreferences::instance(), &UnitPreferences::changed, this, [this] { applyUnits(); });
    connect(m_charge, &QDoubleSpinBox::valueChanged, this,
            [this](double v)
            {
                const double base =
                    csr::units::fromDisplay(v, UnitPreferences::instance().unitForField(kChargeField, Quantity::BulletMass));
                m_charge_g = base;
                emit edited(QStringLiteral("mc"), QString::number(base, 'g', 15));
            });
    connect(m_step, &QDoubleSpinBox::valueChanged, this,
            [this](double v)
            {
                const double base = csr::units::fromDisplay(v, UnitPreferences::instance().unitForField(kStepField, Quantity::BulletMass));
                m_step_g = base;
                emit edited(QStringLiteral("laddermc"), QString::number(base, 'g', 15));
            });
    connect(m_steps, &QSpinBox::valueChanged, this,
            [this](int v)
            {
                m_stepCount = v;
                emit edited(QStringLiteral("laddercnt"), QString::number(v));
            });
    connect(m_table, &QTableWidget::cellClicked, this, [this](int row, int) { emit stepSelected(row); });
}

void ChargeView::chooseUnit(const QString &id)
{
    UnitPreferences &prefs = UnitPreferences::instance();
    for (const QString &field : {kChargeField, kStepField})
    {
        // the system's own unit is stored as "no override" so that switching the system still works
        const bool isSystemUnit = id == QLatin1String(prefs.unitFor(Quantity::BulletMass).id);
        prefs.setFieldUnit(field, Quantity::BulletMass, isSystemUnit ? QString() : id);
    }
}

void ChargeView::applyUnits()
{
    const UnitPreferences &prefs = UnitPreferences::instance();
    const QString current = QString::fromLatin1(prefs.unitForField(kChargeField, Quantity::BulletMass).id);
    for (QPushButton *b : m_unitButtons)
    {
        const QSignalBlocker block(b);
        b->setChecked(b->property("unitId").toString() == current);
    }
    {
        const QSignalBlocker block(m_charge);
        setRange(m_charge, prefs.unitForField(kChargeField, Quantity::BulletMass), 1000.0);
    }
    {
        const QSignalBlocker block(m_step);
        setRange(m_step, prefs.unitForField(kStepField, Quantity::BulletMass), 1000.0);
    }
    showValues();
}

void ChargeView::showValues()
{
    const UnitPreferences &prefs = UnitPreferences::instance();
    const QSignalBlocker b1(m_charge), b2(m_step), b3(m_steps);
    m_charge->setValue(csr::units::toDisplay(m_charge_g, prefs.unitForField(kChargeField, Quantity::BulletMass)));
    m_step->setValue(csr::units::toDisplay(m_step_g, prefs.unitForField(kStepField, Quantity::BulletMass)));
    m_steps->setValue(std::clamp(m_stepCount, 1, csr::kMaxLadderSteps));
    m_step->setEnabled(m_stepCount >= 2);
    // the charge in the other unit, and what the ladder covers
    const csr::units::Unit &u = prefs.unitForField(kChargeField, Quantity::BulletMass);
    const csr::units::Unit &other = u.id == QLatin1String("grain") ? *csr::units::findUnit(Quantity::BulletMass, QStringLiteral("g"))
                                                                   : *csr::units::findUnit(Quantity::BulletMass, QStringLiteral("grain"));
    m_chargeOther->setText(QStringLiteral("= %1 %2").arg(csr::units::format(m_charge_g, other), QString::fromUtf8(other.symbol)));
    if (m_stepCount >= 2 && m_step_g > 0.0)
    {
        const double lowest = std::max(0.0, m_charge_g - (m_stepCount - 1) * m_step_g);
        m_ladderSummary->setText(
            tr("%n steps: %1 down to %2", nullptr, m_stepCount)
                .arg(shown(m_charge_g, Quantity::BulletMass, kChargeField), shown(lowest, Quantity::BulletMass, kChargeField)));
    }
    else m_ladderSummary->setText(tr("No ladder: only the charge above is simulated."));
}

void ChargeView::setValues(double charge_g, int steps, double step_g)
{
    m_charge_g = charge_g;
    m_stepCount = std::max(steps, 1);
    m_step_g = step_g;
    showValues();
}

void ChargeView::setLadder(const std::vector<csr::LadderStep> &ladder, int focus, double maxPressure_bar)
{
    m_table->setRowCount(0);
    for (std::size_t i = 0; i < ladder.size(); ++i)
    {
        const csr::LadderStep &s = ladder[i];
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        const auto cell = [&](int col, const QString &text)
        {
            auto *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_table->setItem(row, col, item);
            return item;
        };
        cell(0, shown(s.charge_g, Quantity::BulletMass));
        if (s.result.ok())
        {
            QTableWidgetItem *p = cell(1, shown(s.result.maxPressure_bar, Quantity::Pressure));
            if (maxPressure_bar > 0.0 && s.result.maxPressure_bar > maxPressure_bar)
            {
                p->setForeground(Qt::white);
                p->setBackground(QColor(0xc4, 0x2b, 0x1c));
                p->setText(QStringLiteral("⚠ ") + p->text());
                p->setToolTip(tr("Above the permitted maximum pressure %1").arg(shown(maxPressure_bar, Quantity::Pressure)));
            }
            cell(2, shown(s.result.muzzleVelocity_ms, Quantity::Velocity));
        }
        else
        {
            cell(1, tr("charge too high"));
            cell(2, QStringLiteral("–"));
        }
    }
    m_table->selectRow(std::clamp(focus, 0, std::max(0, m_table->rowCount() - 1)));
}
