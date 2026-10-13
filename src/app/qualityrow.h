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

#include <QWidget>

class QLabel;
class QProgressBar;

// "Model quality: 0.85 [bar]": the fit quality of the selected powder, always visible below a list of powders.
class QualityRow : public QWidget
{
    Q_OBJECT
public:
    explicit QualityRow(QWidget *parent = nullptr);
    // quality 0..1; `known` false (nothing selected) or quality 0: "unknown" and an empty bar
    void setQuality(double quality, bool known = true);

private:
    QLabel *m_value;
    QProgressBar *m_bar;
};
