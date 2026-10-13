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

#include "newloaddialog.h"

#include "qualityrow.h"
#include "screenfit.h"
#include "buttonstyle.h"
#include "unitprefs.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace
{

using csr::units::Quantity;

// Spin box in the user's unit for `q`; values are exchanged in base units via setBase()/base().
QDoubleSpinBox *spin(Quantity q, double minBase, double maxBase, QWidget *parent)
{
    const csr::units::Unit &u = UnitPreferences::instance().unitFor(q);
    auto *s = new QDoubleSpinBox(parent);
    s->setProperty("quantity", static_cast<int>(q));
    s->setProperty("unit", QString::fromLatin1(u.id));
    s->setDecimals(u.decimals);
    s->setSuffix(QStringLiteral(" ") + QString::fromUtf8(u.symbol));
    s->setRange(csr::units::toDisplay(minBase, u), csr::units::toDisplay(maxBase, u));
    s->setGroupSeparatorShown(false);
    return s;
}

const csr::units::Unit &unitOf(const QDoubleSpinBox *s)
{
    return *csr::units::findUnit(static_cast<Quantity>(s->property("quantity").toInt()), s->property("unit").toString());
}

double base(const QDoubleSpinBox *s)
{
    return csr::units::fromDisplay(s->value(), unitOf(s));
}
void setBase(QDoubleSpinBox *s, double b)
{
    s->setValue(csr::units::toDisplay(b, unitOf(s)));
}

QString describe(const std::optional<QVariantMap> &r, const QString &a, const QString &b, const QString &placeholder)
{
    if (!r) return placeholder;
    return QStringLiteral("%1 %2").arg(r->value(a).toString(), r->value(b).toString()).trimmed();
}

}  // namespace

NewLoadDialog::NewLoadDialog(const csr::Database &db, QWidget *parent)
    : QDialog(parent)
    , m_db(db)
    , m_pages(new QStackedWidget)
    , m_tabs(new QTabWidget)
    , m_matchDiameter(new QCheckBox(tr("Show only bullets that fit the caliber")))
    , m_charge(spin(Quantity::BulletMass, 0.0, 100.0, this))
    , m_barrel(spin(Quantity::Length, 0.0, 2000.0, this))
    , m_depth(spin(Quantity::Length, 0.0, 100.0, this))
    , m_twist(spin(Quantity::Length, 0.0, 2000.0, this))
    , m_temperature(spin(Quantity::Temperature, -40.0, 80.0, this))
    , m_chargeGrains(new QLabel)
    , m_buttons(new QDialogButtonBox(QDialogButtonBox::Cancel))
{
    setWindowTitle(tr("New load"));
    resizeWithinScreen(this, {1100, 640});

    const std::array titles{tr("1. Caliber"), tr("2. Projectile"), tr("3. Propellant")};
    for (std::size_t i = 0; i < csr::kRecordTables.size(); ++i)
    {
        m_browsers[i] = new TableBrowser(csr::kRecordTables[i]);
        m_browsers[i]->setDatabase(&m_db);
        if (i == 1)
        {
            auto *page = new QWidget;
            auto *l = new QVBoxLayout(page);
            l->setContentsMargins(0, 0, 0, 0);
            m_matchDiameter->setChecked(true);
            l->addWidget(m_matchDiameter);
            l->addWidget(m_browsers[i], 1);
            m_tabs->addTab(page, titles[i]);
        }
        else if (i == 2)  // the quality of the selected powder below the list
        {
            auto *page = new QWidget;
            auto *l = new QVBoxLayout(page);
            l->setContentsMargins(0, 0, 0, 0);
            l->addWidget(m_browsers[i], 1);
            l->addWidget(m_quality = new QualityRow);
            m_tabs->addTab(page, titles[i]);
        }
        else
        {
            m_tabs->addTab(m_browsers[i], titles[i]);
        }
        connect(m_browsers[i], &TableBrowser::currentIdChanged, this, &NewLoadDialog::onSelectionChanged);
    }
    connect(m_matchDiameter, &QCheckBox::toggled, this, &NewLoadDialog::onSelectionChanged);

    auto *params = new QGroupBox(tr("Load data"));
    auto *form = new QFormLayout(params);
    form->addRow(tr("Propellant charge"), m_charge);
    form->addRow(QString(), m_chargeGrains);
    form->addRow(tr("Barrel length"), m_barrel);
    form->addRow(tr("Bullet seating depth"), m_depth);
    form->addRow(tr("Twist length"), m_twist);
    form->addRow(tr("Propellant temperature"), m_temperature);
    auto *reset = new QPushButton(tr("Reset to defaults"));
    form->addRow(reset);
    connect(reset, &QPushButton::clicked, this, &NewLoadDialog::applyDefaults);
    connect(m_charge, &QDoubleSpinBox::valueChanged, this,
            [this]
            {
                // show the charge in the "other" mass unit as a convenience
                const bool inGrain = unitOf(m_charge).id == QLatin1String("grain");
                m_chargeGrains->setText(inGrain ? tr("= %1 g").arg(base(m_charge), 0, 'f', 3)
                                                : tr("= %1 grains").arg(base(m_charge) / csr::kGrainToGram, 0, 'f', 1));
            });

    params->setMinimumWidth(std::max(360, params->sizeHint().width()));
    auto *dataPage = new QWidget;
    auto *dataRow = new QHBoxLayout(dataPage);
    auto *dataCol = new QVBoxLayout;
    dataCol->addWidget(makeSummary(m_summary));
    dataCol->addWidget(params);
    dataCol->addStretch(1);
    dataRow->addLayout(dataCol);
    dataRow->addStretch(1);

    auto *selectPage = new QWidget;
    auto *selectCol = new QVBoxLayout(selectPage);
    selectCol->setContentsMargins(0, 0, 0, 0);
    selectCol->addWidget(m_tabs, 1);
    selectCol->addWidget(makeSummary(m_selection));
    m_pages->addWidget(selectPage);
    m_pages->addWidget(dataPage);

    auto *main = new QVBoxLayout(this);
    main->addWidget(m_pages, 1);
    main->addWidget(m_buttons);
    m_back = m_buttons->addButton(tr("< Back"), QDialogButtonBox::ActionRole);
    m_next = m_buttons->addButton(tr("Next >"), QDialogButtonBox::ActionRole);
    connect(m_back, &QPushButton::clicked, this, &NewLoadDialog::goBack);
    connect(m_next, &QPushButton::clicked, this, &NewLoadDialog::goNext);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    m_next->setDefault(true);
    markPrimary(m_next);
    m_back->hide();
    m_next->setEnabled(false);
    m_charge->valueChanged(0.0);
    updateSummary();
}

void NewLoadDialog::onSelectionChanged()
{
    const auto fetch = [&](csr::Table t, std::size_t idx) -> std::optional<QVariantMap>
    {
        const int id = m_browsers[idx]->currentId();
        return id < 0 ? std::nullopt : m_db.record(t, id);
    };
    const auto oldCaliber = m_caliber;
    m_caliber = fetch(csr::Table::Caliber, 0);
    m_projectile = fetch(csr::Table::Projectile, 1);
    m_propellant = fetch(csr::Table::Propellant, 2);

    // narrow the bullet list to the caliber's groove diameter
    if (m_caliber && m_matchDiameter->isChecked())
    {
        const double z = m_caliber->value(QStringLiteral("c_Z")).toDouble();
        m_browsers[1]->setRowFilter(z > 0 ? QStringLiteral("ABS(gdia - %1) < 0.25").arg(z) : QString());
    }
    else
    {
        m_browsers[1]->setRowFilter({});
    }
    if (m_caliber != oldCaliber) m_projectile = fetch(csr::Table::Projectile, 1);

    if (m_quality) m_quality->setQuality(m_browsers[2]->currentNumber(QStringLiteral("Qlty")), m_propellant.has_value());
    updateSummary();
    m_next->setEnabled(m_pages->currentIndex() == 1 || (m_caliber && m_projectile && m_propellant));
}

// Step 2 (or "Create load" there): the data fields get the defaults of the selection unless they were filled for this very selection,
// so going back and forth without changing it keeps the edits.
void NewLoadDialog::goNext()
{
    if (m_pages->currentIndex() == 1)
    {
        accept();
        return;
    }
    if (!(m_caliber && m_projectile && m_propellant)) return;
    const std::array<int, 3> ids{m_browsers[0]->currentId(), m_browsers[1]->currentId(), m_browsers[2]->currentId()};
    if (ids != m_defaultsFor)
    {
        applyDefaults();
        m_defaultsFor = ids;
    }
    m_pages->setCurrentIndex(1);
    m_back->show();
    m_next->setText(tr("Create load"));
}

void NewLoadDialog::goBack()
{
    m_pages->setCurrentIndex(0);
    m_back->hide();
    m_next->setText(tr("Next >"));
    m_next->setEnabled(m_caliber && m_projectile && m_propellant);
}

void NewLoadDialog::applyDefaults()
{
    if (m_caliber)
    {
        setBase(m_barrel, csr::defaultBarrelLength_mm(*m_caliber));
        if (m_projectile) setBase(m_depth, csr::defaultSeatingDepth_mm(*m_caliber, *m_projectile));
        if (m_propellant) setBase(m_charge, csr::defaultCharge_g(*m_caliber, *m_propellant));
    }
    if (m_twist->value() == 0.0) setBase(m_twist, 240.0);
    if (!m_temperatureSet)
    {
        setBase(m_temperature, 21.0);
        m_temperatureSet = true;
    }
}

// The selected components below their labels, all starting at the same x position.
QWidget *NewLoadDialog::makeSummary(std::array<QLabel *, 3> &values)
{
    auto *w = new QWidget;
    auto *grid = new QGridLayout(w);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(16);
    const std::array titles{tr("Caliber:"), tr("Projectile:"), tr("Propellant:")};
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        auto *title = new QLabel(QStringLiteral("<b>%1</b>").arg(titles[i]));
        title->setTextFormat(Qt::RichText);
        values[i] = new QLabel;
        values[i]->setWordWrap(true);
        grid->addWidget(title, static_cast<int>(i), 0, Qt::AlignTop);
        grid->addWidget(values[i], static_cast<int>(i), 1);
    }
    grid->setColumnStretch(1, 1);
    return w;
}

void NewLoadDialog::updateSummary()
{
    const QString none = tr("<i>not selected</i>");
    const std::array<QString, 3> text{describe(m_caliber, QStringLiteral("cipname"), QStringLiteral("altname"), none),
                                      describe(m_projectile, QStringLiteral("mname"), QStringLiteral("pname"), none),
                                      describe(m_propellant, QStringLiteral("mname"), QStringLiteral("pname"), none)};
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        m_summary[i]->setText(text[i]);
        m_selection[i]->setText(text[i]);
    }
}

bool NewLoadDialog::select(int caliberId, int projectileId, int propellantId)
{
    bool ok = m_browsers[0]->selectId(caliberId);  // narrows the bullet list
    ok = m_browsers[1]->selectId(projectileId) && ok;
    ok = m_browsers[2]->selectId(propellantId) && ok;
    goNext();
    return ok;
}

csr::LoadFile NewLoadDialog::result() const
{
    if (!(m_caliber && m_projectile && m_propellant)) return {};
    csr::LoadParams p;
    p.charge_g = base(m_charge);
    p.barrelLength_mm = base(m_barrel);
    p.seatingDepth_mm = base(m_depth);
    p.twistLength_mm = base(m_twist);
    p.powderTemperature_C = base(m_temperature);
    return csr::buildLoad(*m_caliber, *m_projectile, *m_propellant, p, m_db.columns(csr::Table::Caliber));
}
