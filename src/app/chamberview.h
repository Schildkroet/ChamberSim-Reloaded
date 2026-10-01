#pragma once

#include "cartridgeview.h"
#include "hovermeasure.h"
#include "loadfile.h"
#include "solver.h"

#include <QMap>
#include <QWidget>

#include <optional>
#include <vector>

// The "Chamber" tab of the drawing panel: a section through the rifle's chamber and the first part of the
// barrel, from the caliber's chamber dimensions (the c_* values of the caliber record), with the loaded
// round seated in it.
class ChamberView : public QWidget {
    Q_OBJECT
public:
    explicit ChamberView(QWidget *parent = nullptr);

    void setLoad(const grt::LoadFile &file, const grt::SimInput &input);
    void clear();
    void setTableVisible(bool visible);  // for reports: only the measure A below the drawing

    struct Geometry {
        std::vector<QPointF> cavity;  // (radius, y) in mm from the breech face up to the end of the drawn barrel
        double lands{0.0};            // bore diameter over the lands
        double grooves{0.0};          // diameter over the grooves
        double throatY{0.0};          // where the chamber ends and the leade cone starts
        bool valid{false};
    };
    // `topY` limits how far up the barrel is drawn.
    [[nodiscard]] static Geometry buildGeometry(const QMap<QString, double> &dims, double topY);
    [[nodiscard]] const Geometry &geometry() const noexcept { return m_geometry; }

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override { return {420, 520}; }
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    struct RowHit {
        QRectF rect;
        QString key, label, value;
    };
    [[nodiscard]] std::optional<HoverMeasure> hoverMeasure(const QString &key, const QString &label, const QString &valueText) const;

    void drawTable(QPainter &p, const QRectF &area) const;

    QMap<QString, double> m_dims;
    CartridgeView::RoundShape m_round;
    Geometry m_geometry;
    double m_topY{0.0};
    bool m_hasLoad{false};
    bool m_showTable{true};
    mutable std::vector<RowHit> m_rowHits;  // filled while the table is painted
    QString m_hoverKey;
};
