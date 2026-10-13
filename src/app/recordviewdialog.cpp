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

#include "recordviewdialog.h"

#include "dates.h"
#include "qualitydelegate.h"
#include "recordfields.h"
#include "screenfit.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QColor>
#include <QLabel>
#include <QGuiApplication>
#include <QLocale>
#include <QProgressBar>
#include <QScreen>
#include <QScrollBar>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>
#include <map>

using namespace Qt::StringLiterals;
using csr::dbfields::Kind;

namespace
{

QString choiceText(csr::Table table, const csr::dbfields::Field &f, const QString &value)
{
    for (const auto &[v, text] : csr::dbfields::choices(table, f.column))
        if (v.compare(value, Qt::CaseInsensitive) == 0) return text;
    return value;
}

QString shownValue(csr::Table table, const csr::dbfields::Field &f, const QVariant &stored)
{
    switch (f.kind)
    {
    case Kind::Number: return stored.isValid() ? recordfields::shownNumber(f, stored.toDouble()) : QString();
    case Kind::BaseType:
        switch (stored.toInt())
        {
        case 1: return QCoreApplication::translate("RecordDialog", "hollow base");
        case 2: return QCoreApplication::translate("RecordDialog", "boat tail");
        default: return QCoreApplication::translate("RecordDialog", "flat base");
        }
    case Kind::Choice: return choiceText(table, f, stored.toString());
    case Kind::Date:
    {
        const auto date = csr::normalizedDate(stored);
        return date ? *date : stored.toString();
    }
    default:
        if (f.alignRight && stored.metaType().id() == QMetaType::QString)
        {
            bool ok = false;
            const double v = QLocale::c().toDouble(stored.toString(), &ok);
            if (ok) return recordfields::shownNumber({.column = f.column}, v);
        }
        return stored.toString();
    }
}

}  // namespace

RecordViewDialog::RecordViewDialog(const csr::Database &db, csr::Table table, int id, const QVariantMap &record, QWidget *parent)
    : QDialog(parent)
{
    setObjectName(u"recordViewDialog"_s);
    const QString kind = table == csr::Table::Caliber      ? tr("Caliber")
                         : table == csr::Table::Projectile ? tr("Projectile")
                                                           : tr("Propellant");
    setWindowTitle(u"%1 – %2"_s.arg(kind, csr::dbfields::recordTitle(table, record)));

    std::map<QString, std::vector<std::pair<csr::dbfields::Field, QVariant>>> byGroup;
    for (const auto &[column, type] : db.schema(table))
    {
        const csr::dbfields::Field f = csr::dbfields::describe(table, column, type);
        if (f.kind == Kind::Hidden) continue;
        byGroup[f.group].push_back({f, record.value(column)});
    }
    auto *content = new QWidget;
    auto *contentLayout = new QVBoxLayout(content);
    for (const QString &group : csr::dbfields::groups(table))
    {
        const auto it = byGroup.find(group);
        if (it == byGroup.end()) continue;
        auto *box = new QGroupBox(group);
        auto *grid = new QGridLayout(box);
        grid->setHorizontalSpacing(10);
        // groups with many fields get two label / value pairs per row, like the edit dialog
        const int pairs = it->second.size() > 10 ? 2 : 1;
        for (int c = 0; c < pairs; ++c) grid->setColumnStretch(2 * c + 1, 1);
        int i = 0;
        for (const auto &[f, stored] : it->second)
        {
            QString label = f.label;
            if (const auto *u = recordfields::unitOf(f)) label += u" [%1]"_s.arg(QString::fromUtf8(u->symbol));
            else if (!f.unitText.isEmpty()) label += u" [%1]"_s.arg(f.unitText);
            auto *value = new QLabel(shownValue(table, f, stored));
            value->setObjectName(f.column);
            value->setTextInteractionFlags(Qt::TextSelectableByMouse);
            value->setWordWrap(true);
            if (f.alignRight) value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            const bool wide = f.kind == Kind::Text && stored.toString().size() > 24;  // long texts (origin, description) use the full row
            const int row = wide ? (i + pairs - 1) / pairs : i / pairs, col = wide ? 0 : 2 * (i % pairs);
            grid->addWidget(new QLabel(label + u':'), row, col);
            if (f.column == u"Qlty"_s)  // the fit quality 0..1 of a powder: a bar next to the number (0: unknown)
            {
                auto *bar = makeQualityBar(stored.toDouble());
                bar->setToolTip(tr("Quality of the fit of the powder model: 1 = very good, 0 = unknown"));
                value->setWordWrap(false);
                auto *cell = new QWidget;
                auto *cellLayout = new QHBoxLayout(cell);
                cellLayout->setContentsMargins(0, 0, 0, 0);
                cellLayout->addWidget(value);
                cellLayout->addWidget(bar, 1);
                grid->addWidget(cell, row, col + 1, 1, 1);
            }
            else grid->addWidget(value, row, col + 1, 1, wide ? 2 * pairs - 1 : 1);
            i = wide ? ((i + pairs - 1) / pairs + 1) * pairs : i + 1;
            ++m_fieldCount;
        }
        contentLayout->addWidget(box);
    }
    contentLayout->addStretch(1);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidget(content);
    auto *buttons = new QDialogButtonBox;
    const bool own = csr::Database::isOwnId(id);
    auto *edit = buttons->addButton(own ? tr("Edit…") : tr("Copy…"), QDialogButtonBox::ActionRole);
    edit->setObjectName(own ? u"editRecordButton"_s : u"copyRecordButton"_s);
    edit->setToolTip(own ? tr("Change this record") : tr("Only your own records can be changed: add a changed copy"));
    connect(edit, &QPushButton::clicked, this, [this, own] { done(own ? EditRequested : CopyRequested); });
    buttons->addButton(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(scroll, 1);
    layout->addWidget(buttons);
    // fit the dialog to its content (the fields of the table), within the screen
    const QMargins m = layout->contentsMargins();
    const QSize want =
        content->sizeHint() + QSize(scroll->verticalScrollBar()->sizeHint().width() + 2 * scroll->frameWidth() + m.left() + m.right(),
                                    buttons->sizeHint().height() + layout->spacing() + 2 * scroll->frameWidth() + m.top() + m.bottom());
    resizeWithinScreen(this, want.expandedTo({table == csr::Table::Caliber ? 760 : 620, 400}));
}

QString RecordViewDialog::valueText(const QString &column) const
{
    const auto *label = findChild<QLabel *>(column);
    return label ? label->text() : QString();
}
