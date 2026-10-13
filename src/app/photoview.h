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

#include <QImage>
#include <QString>
#include <QWidget>

class QToolButton;

// The photos of a propellant (overview and detail) with a scale bar: the "Powder" tab of the drawing panel (the
// owner shows the tab only while there are photos). A double click opens the picture larger.
class PhotoView : public QWidget
{
    Q_OBJECT
public:
    struct Photo
    {
        QImage image;
        QString name;
        double pxPerMm{};  // 0: unknown, no scale bar
    };

    explicit PhotoView(QWidget *parent = nullptr);

    // Either may be empty. `key` identifies the pair (to skip
    // reloading the same pictures); the overview is shown first.
    void setPhotos(const Photo &overview, const Photo &detail, const QString &key);
    void clear();
    [[nodiscard]] QString key() const { return m_key; }
    [[nodiscard]] bool hasPhotos() const noexcept { return !m_photos[0].image.isNull() || !m_photos[1].image.isNull(); }
    // 0 overview, 1 detail; -1: nothing shown.
    [[nodiscard]] int shown() const noexcept { return m_current; }
    void show(int which);

    // Length [mm] of the scale bar for a picture shown `shownPx` pixels wide: 1, 2 or 5 times a power of ten that
    // takes about a fifth of the width. 0 without a scale.
    [[nodiscard]] static double scaleBarMm(double pxPerMm, double imageWidthPx, double shownWidthPx);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    QSize sizeHint() const override { return {420, 380}; }
    QSize minimumSizeHint() const override { return {160, 120}; }

private:
    [[nodiscard]] const Photo *photo() const;
    [[nodiscard]] QRect imageRect(const QImage &img) const;
    void select(int which);

    Photo m_photos[2];
    QString m_key;
    int m_current{-1};
    QToolButton *m_overviewButton;
    QToolButton *m_detailButton;
};
