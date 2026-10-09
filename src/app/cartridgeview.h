#pragma once

#include "hovermeasure.h"
#include "loadfile.h"
#include "solver.h"

#include <QMap>
#include <QWidget>

#include <functional>
#include <optional>
#include <vector>

// Technical drawing of the loaded round, like the "Cartridge" panel of the original program: the case
// from the caliber's CIP dimensions, the seated bullet and the powder fill, with a table of the
// dimensions. Everything is drawn from the load file; nothing is looked up elsewhere.
class CartridgeView : public QWidget
{
    Q_OBJECT
public:
    explicit CartridgeView(QWidget *parent = nullptr);

    void setLoad(const csr::LoadFile &file, const csr::SimInput &input);
    void clear();
    // For reports the drawing is rendered without the dimension table; only the measures A, B, C stay.
    void setTableVisible(bool visible);

    // Geometry used by the drawing (exposed for tests). Points are (radius, y) in mm, head to mouth.
    struct Geometry
    {
        std::vector<QPointF> outer;
        double wall{0.0};           // wall thickness that makes the interior equal the case capacity
        double cavityVolume_mm3{};  // interior volume up to the mouth with that wall
        double powderHeight_mm{};   // height of the powder fill (bulk volume), measured from the head
        bool valid{false};
    };
    [[nodiscard]] static Geometry buildGeometry(const QMap<QString, double> &dims, double caseVolume_cm3, double powderBulkVolume_cm3);
    [[nodiscard]] const Geometry &geometry() const noexcept { return m_round.geometry; }

    // Everything needed to draw the loaded round: CIP dimensions, the bullet and the derived case geometry.
    struct RoundShape
    {
        QMap<QString, double> dims;
        double bulletDiameter{}, bulletLength{}, depth{}, tailLength{}, tailDiameter{};
        Geometry geometry;
        // a shotshell (no C.I.P. dimensions of a case): brass head, plastic hull, powder, wad and shot column, drawn by paintShotshell
        bool shotshell{false};
        double hullDiameter{}, rimDiameter{}, headHeight{}, floorHeight{}, wadHeight{}, powderTop{};
        [[nodiscard]] double dim(const char *key) const { return dims.value(QLatin1String(key), 0.0); }
        [[nodiscard]] double caseLength() const { return dim("L3"); }
        [[nodiscard]] double bulletBase() const { return caseLength() - depth; }
        [[nodiscard]] double bulletTip() const { return bulletBase() + bulletLength; }
    };
    [[nodiscard]] static RoundShape makeRound(const csr::LoadFile &file, const csr::SimInput &input);
    // Draws case (brass, hollow), powder fill and bullet. X maps a radius [mm] and Y an axial position [mm]
    // (0 = case head) to widget coordinates; the drawing is symmetric about X(0).
    static void paintRound(QPainter &p, const RoundShape &round, const std::function<double(double)> &X,
                           const std::function<double(double)> &Y, bool dark);
    // The same for a shotshell: head with rim and primer, hull, powder, wad (cushion and cup), shot and the crimp.
    static void paintShotshell(QPainter &p, const RoundShape &round, const std::function<double(double)> &X,
                               const std::function<double(double)> &Y, bool dark);
    [[nodiscard]] const RoundShape &round() const noexcept { return m_round; }

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override { return {420, 520}; }
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    struct RowHit
    {
        QRectF rect;
        QString key, label, value;
    };
    // The dimension of the table row `key`, as it is shown in the drawing while the mouse is on that row.
    [[nodiscard]] std::optional<HoverMeasure> hoverMeasure(const QString &key, const QString &label, const QString &valueText) const;

    void drawTable(QPainter &p, const QRectF &area) const;
    // The measures A (overall length), B (case length), C (seating depth); returns the height used.
    double drawMeasures(QPainter &p, const QRectF &area, const QFont &font) const;

    RoundShape m_round;
    csr::SimInput m_input;
    bool m_hasLoad{false};
    bool m_showTable{true};
    mutable std::vector<RowHit> m_rowHits;  // filled while the table is painted
    QString m_hoverKey;
};
