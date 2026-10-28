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

#include "chronographimportdialog.h"

#include "screenfit.h"
#include "measurement.h"
#include "displayformat.h"
#include "unitprefs.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

using csr::units::Quantity;

namespace
{

enum Column
{
    Use,
    Name,
    Shots,
    Mean,
    Sd,
    Charge,
    ColumnCount
};

const csr::units::Unit &unitOf(Quantity q)
{
    return UnitPreferences::instance().unitFor(q);
}

QTableWidgetItem *readOnly(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return item;
}

}  // namespace

ChronographImportDialog::ChronographImportDialog(csr::ChronographFile file, double defaultCharge_g, QWidget *parent)
    : QDialog(parent)
    , m_file(std::move(file))
    , m_default(defaultCharge_g)
    , m_table(new QTableWidget(static_cast<int>(m_file.series.size()), ColumnCount))
{
    setWindowTitle(tr("Import chronograph file"));
    const QString chargeUnit = QString::fromUtf8(unitOf(Quantity::BulletMass).symbol);
    m_table->setHorizontalHeaderLabels({tr("Import"), tr("Series"), tr("Shots"),
                                        tr("Velocity [%1]").arg(QString::fromUtf8(unitOf(Quantity::Velocity).symbol)), tr("SD"),
                                        tr("Charge [%1]").arg(chargeUnit)});
    m_table->horizontalHeaderItem(Charge)->setToolTip(tr("The powder charge of this series (the file does not contain it)"));
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(Name, QHeaderView::Stretch);
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        const auto &s = m_file.series[static_cast<std::size_t>(row)];
        const csr::ShotStatistics st = csr::statistics(s.velocities_ms);
        auto *use = new QTableWidgetItem;
        use->setFlags((use->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
        use->setCheckState(Qt::Checked);
        m_table->setItem(row, Use, use);
        m_table->setItem(row, Name, readOnly(s.name));
        m_table->item(row, Name)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_table->setItem(row, Shots, readOnly(QString::number(s.velocities_ms.size())));
        m_table->setItem(row, Mean, readOnly(csr::units::format(st.mean, unitOf(Quantity::Velocity), 1)));
        m_table->setItem(row, Sd, readOnly(st.count > 1 ? csr::units::format(st.sd, unitOf(Quantity::Velocity), 1) : noValueText()));
        auto *charge = new QTableWidgetItem(csr::units::format(m_default, unitOf(Quantity::BulletMass)));
        charge->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, Charge, charge);
    }

    auto *intro = new QLabel(
        tr("%1 file with %n series. Every ticked series becomes a charge with its shots.", nullptr, static_cast<int>(m_file.series.size()))
            .arg(csr::formatName(m_file.format)));
    intro->setWordWrap(true);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Import"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(intro);
    layout->addWidget(m_table, 1);
    layout->addWidget(buttons);
    resizeWithinScreen(this, {640, 360});
}

std::vector<ImportedCharge> ChronographImportDialog::selected() const
{
    std::vector<ImportedCharge> out;
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        if (m_table->item(row, Use)->checkState() != Qt::Checked) continue;
        // the shown text is rounded: an untouched charge keeps its exact value
        const QString text = m_table->item(row, Charge)->text();
        const auto g = text == csr::units::format(m_default, unitOf(Quantity::BulletMass))
                           ? std::nullopt
                           : csr::units::parseToBase(text, unitOf(Quantity::BulletMass));
        out.push_back({.series = m_file.series[static_cast<std::size_t>(row)], .charge_g = g && *g > 0.0 ? *g : m_default});
    }
    return out;
}

void ChronographImportDialog::setChecked(int row, bool on)
{
    m_table->item(row, Use)->setCheckState(on ? Qt::Checked : Qt::Unchecked);
}
