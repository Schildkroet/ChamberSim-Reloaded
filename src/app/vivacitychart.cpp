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

#include "vivacitychart.h"

#include <QChart>
#include <QEvent>
#include <QLegendMarker>
#include <QLineSeries>
#include <QValueAxis>

#include <algorithm>

VivacityChart::VivacityChart(QWidget *parent)
    : QChartView(parent)
    , m_curve(new QLineSeries)
    , m_z1(new QLineSeries)
    , m_z2(new QLineSeries)
    , m_axisX(new QValueAxis)
    , m_axisY(new QValueAxis)
{
    auto *chart = new QChart;
    chart->legend()->hide();
    chart->setMargins({2, 2, 2, 2});
    chart->setTitle(tr("Burn rate Ba·φ(z)"));
    m_axisX->setTitleText(tr("burnt propellant z"));
    m_axisX->setRange(0.0, 1.0);
    m_axisX->setTickCount(6);
    m_axisX->setLabelFormat("%.1f");
    m_axisY->setLabelFormat("%.1f");
    m_axisY->setTickCount(5);
    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisY, Qt::AlignLeft);
    for (QLineSeries *s : {m_curve, m_z1, m_z2})
    {
        chart->addSeries(s);
        s->attachAxis(m_axisX);
        s->attachAxis(m_axisY);
    }
    setChart(chart);
    setRenderHint(QPainter::Antialiasing);
    setMinimumSize(260, 170);
    applyTheme();
    applyFonts();
}

void VivacityChart::applyTheme()
{
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    chart()->setTheme(dark ? QChart::ChartThemeDark : QChart::ChartThemeLight);
    chart()->setBackgroundBrush(palette().brush(QPalette::Base));
    m_curve->setPen(QPen(QColor(0xe0, 0x6a, 0x6a), 2));
    for (QLineSeries *s : {m_z1, m_z2}) s->setPen(QPen(QColor(0x90, 0xa0, 0xb0), 1, Qt::DashLine));
}

void VivacityChart::applyFonts()
{
    const QFont base = font();
    QFont small = base;
    small.setPointSizeF(base.pointSizeF() * 0.9);
    QFont title = base;
    title.setPointSizeF(base.pointSizeF() * 0.95);
    chart()->setTitleFont(title);
    for (QValueAxis *a : {m_axisX, m_axisY})
    {
        a->setLabelsFont(small);
        a->setTitleFont(small);
    }
}

void VivacityChart::changeEvent(QEvent *event)
{
    QChartView::changeEvent(event);
    if (event->type() == QEvent::PaletteChange)
    {
        applyTheme();
        applyFonts();
    }
    else if (event->type() == QEvent::FontChange || event->type() == QEvent::ApplicationFontChange)
    {
        applyFonts();
    }
}

void VivacityChart::setCurve(const std::vector<double> &values, double z1, double z2)
{
    QList<QPointF> pts;
    pts.reserve(static_cast<qsizetype>(values.size()));
    for (std::size_t i = 0; i < values.size(); ++i)
        pts.append({static_cast<double>(i) / static_cast<double>(values.size() - 1), values[i]});
    m_curve->replace(pts);
    const double top = values.empty() ? 1.0 : *std::ranges::max_element(values) * 1.1;
    m_axisY->setRange(0.0, std::max(top, 0.1));
    m_z1->replace({{z1, 0.0}, {z1, m_axisY->max()}});
    m_z2->replace({{z2, 0.0}, {z2, m_axisY->max()}});
}

void VivacityChart::clear()
{
    m_curve->clear();
    m_z1->clear();
    m_z2->clear();
}
