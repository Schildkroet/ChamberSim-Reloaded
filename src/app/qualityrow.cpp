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

#include "qualityrow.h"

#include "qualitydelegate.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>

#include <algorithm>

QualityRow::QualityRow(QWidget *parent)
    : QWidget(parent)
    , m_value(new QLabel)
    , m_bar(makeQualityBar(0.0))
{
    auto *row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->addWidget(new QLabel(tr("Model quality:")));
    m_value->setMinimumWidth(m_value->fontMetrics().horizontalAdvance(tr("unknown")) + 8);
    row->addWidget(m_value);
    row->addWidget(m_bar, 1);
    setToolTip(tr("Quality of the fit of the powder model: 1 = very good, 0 = unknown"));
    setQuality(0.0, false);
}

void QualityRow::setQuality(double quality, bool known)
{
    quality = known ? std::clamp(quality, 0.0, 1.0) : 0.0;
    m_value->setText(quality > 0.0 ? QLocale().toString(quality, 'f', 2) : tr("unknown"));
    setQualityBar(m_bar, quality);
}
