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

#include <QColor>
#include <QPainter>
#include <QProgressBar>
#include <QStyledItemDelegate>

#include <algorithm>

// The fit quality 0..1 of a powder (column Qlty) as a bar over the full width of the cell, red (poor) over yellow to green (good).
// The number stays readable on top; 0 means unknown and has no bar. `role` is the item role that holds the number.
class QualityDelegate : public QStyledItemDelegate
{
public:
    explicit QualityDelegate(QObject *parent = nullptr, int role = Qt::DisplayRole)
        : QStyledItemDelegate(parent)
        , m_role(role)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const double quality = std::clamp(index.data(m_role).toDouble(), 0.0, 1.0);
        if (quality > 0.0)
        {
            QColor bar = QColor::fromHsvF(quality / 3.0, 0.65, 0.85);  // hue 0 (red) .. 1/3 (green)
            bar.setAlpha(option.state & QStyle::State_Selected ? 130 : 105);
            QRect r = option.rect.adjusted(1, 2, -1, -2);
            r.setWidth(static_cast<int>(r.width() * quality));
            painter->fillRect(r, bar);
        }
        QStyledItemDelegate::paint(painter, option, index);
    }

private:
    int m_role;
};

// Shows the fit quality 0..1 on a bar (same colors as the delegate).
inline void setQualityBar(QProgressBar *bar, double quality)
{
    quality = std::clamp(quality, 0.0, 1.0);
    bar->setValue(qRound(100 * quality));
    bar->setStyleSheet(QStringLiteral("QProgressBar { border: 1px solid palette(mid); border-radius: 3px; background: transparent; }"
                                      "QProgressBar::chunk { background-color: %1; border-radius: 2px; }")
                           .arg(QColor::fromHsvF(quality / 3.0, 0.65, 0.85).name()));
}

// A thin bar for the fit quality 0..1; the caller adds the tooltip.
[[nodiscard]] inline QProgressBar *makeQualityBar(double quality)
{
    auto *bar = new QProgressBar;
    bar->setRange(0, 100);
    bar->setTextVisible(false);
    bar->setFixedHeight(10);
    bar->setMinimumWidth(80);
    setQualityBar(bar, quality);
    return bar;
}
