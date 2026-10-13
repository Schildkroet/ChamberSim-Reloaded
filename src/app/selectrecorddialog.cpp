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

#include "selectrecorddialog.h"

#include "qualityrow.h"
#include "screenfit.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

using namespace Qt::StringLiterals;

SelectRecordDialog::SelectRecordDialog(const csr::Database &db, csr::Table table, const QString &title, QWidget *parent)
    : QDialog(parent)
    , m_browser(new TableBrowser(table))
    , m_buttons(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel))
{
    setWindowTitle(title);
    resizeWithinScreen(this, {980, 600});
    m_browser->setDatabase(&db);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_browser, 1);
    if (table ==
        csr::Table::Propellant)  // the quality of the selected powder's model, always visible (the table's column may be scrolled away)
        layout->addWidget(m_qualityRow = new QualityRow);
    layout->addWidget(m_buttons);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_browser, &TableBrowser::currentIdChanged, this, [this] { updateOk(); });
    updateOk();
}

void SelectRecordDialog::setRestriction(const QString &sqlCondition, const QString &checkBoxText)
{
    m_condition = sqlCondition;
    if (!m_restrict)
    {
        m_restrict = new QCheckBox(checkBoxText);
        m_restrict->setChecked(true);
        static_cast<QVBoxLayout *>(layout())->insertWidget(0, m_restrict);
        connect(m_restrict, &QCheckBox::toggled, this, [this](bool on) { m_browser->setRowFilter(on ? m_condition : QString()); });
    }
    m_restrict->setText(checkBoxText);
    m_browser->setRowFilter(m_restrict->isChecked() ? m_condition : QString());
}

void SelectRecordDialog::setResetOption(const QString &text, bool checked)
{
    if (!m_reset)
    {
        m_reset = new QCheckBox;
        static_cast<QVBoxLayout *>(layout())->insertWidget(layout()->count() - 1, m_reset);
    }
    m_reset->setText(text);
    m_reset->setChecked(checked);
}

bool SelectRecordDialog::resetChecked() const
{
    return m_reset && m_reset->isChecked();
}

bool SelectRecordDialog::select(int id)
{
    return m_browser->selectId(id);
}

void SelectRecordDialog::updateOk()
{
    if (m_qualityRow) m_qualityRow->setQuality(m_browser->currentNumber(u"Qlty"_s), m_browser->currentId() >= 0);
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(m_browser->currentId() >= 0);
}
