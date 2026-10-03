#pragma once

#include "solver.h"
#include "units.h"

#include <QChartView>

#include <optional>
#include <span>
#include <vector>

class QAreaSeries;
class QLineSeries;
class QGraphicsSimpleTextItem;
class QValueAxis;

// Pressure, bullet energy and velocity over time (or position) with the markers of the original
// program: z1, Pmax, z2, 95 % energy and burnout, plus warning bands below the permitted pressure.
// A reference run (the original's curves) can be drawn dashed underneath. The mouse over the plot shows a cursor line with
// the values of the curves at that point.
class CurveChart : public QChartView
{
    Q_OBJECT
public:
    enum class XAxis
    {
        Time,
        Position
    };

    explicit CurveChart(QWidget *parent = nullptr);

    // `permittedPressure_bar` draws the bands "Pmax", "Pmax-15 %" and "Pmax-25 %" (0 = none).
    void setResult(const grt::SimResult &result, double permittedPressure_bar);
    void clearResult();
    void setReference(std::span<const grt::SimPoint> points);  // empty span clears the overlay
    // The other curves of a load development ladder, drawn faded behind the selected one.
    void setLadder(std::vector<grt::SimResult> others);
    // Units shown on the axes (the data stays in bar, J, m/s, mm).
    void setUnits(const grt::units::Unit &pressure, const grt::units::Unit &energy, const grt::units::Unit &velocity,
                  const grt::units::Unit &length);
    void setXAxis(XAxis axis);
    [[nodiscard]] XAxis xAxis() const noexcept { return m_xAxis; }

    // The curves' values at `x` (time or position, as on the x axis), interpolated; none outside the curves.
    [[nodiscard]] std::optional<grt::SimPoint> pointAt(double x) const;
    [[nodiscard]] std::optional<double> cursorX() const noexcept { return m_cursorX; }

protected:
    void changeEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;

private:
    struct Marker
    {
        QLineSeries *line;
        QGraphicsSimpleTextItem *label;
        double x{0.0};
        bool visible{false};
    };

    [[nodiscard]] double xOf(const grt::SimPoint &p) const noexcept;
    void rebuild();
    void updateMarkerLabels();
    void applyTheme();
    void applyFonts();
    void placeMarker(Marker &m, bool visible, double x, double yTop, const QString &text);
    QAreaSeries *makeBand(const QColor &color);

    XAxis m_xAxis{XAxis::Time};
    const grt::units::Unit *m_pressureUnit;
    const grt::units::Unit *m_energyUnit;
    const grt::units::Unit *m_velocityUnit;
    const grt::units::Unit *m_lengthUnit;
    grt::SimResult m_result;
    double m_permitted{0.0};
    std::vector<grt::SimPoint> m_reference;
    std::vector<grt::SimResult> m_ladder;
    std::vector<QLineSeries *> m_ladderSeries;

    QLineSeries *m_pressure;
    QLineSeries *m_velocity;
    QLineSeries *m_energy;
    QLineSeries *m_refPressure;
    QLineSeries *m_refVelocity;
    QLineSeries *m_refEnergy;
    QLineSeries *m_permittedLine;
    QAreaSeries *m_bandRed{nullptr};
    QAreaSeries *m_bandBlue{nullptr};
    QLineSeries *m_redLo{nullptr}, *m_redHi{nullptr}, *m_blueLo{nullptr}, *m_blueHi{nullptr};
    std::vector<Marker> m_markers;    // z1, Pmax, z2, 95 %, burnout
    std::optional<double> m_cursorX;  // mouse cursor, in x axis units
    QValueAxis *m_axisX;
    QValueAxis *m_axisP;
    QValueAxis *m_axisE;
    QValueAxis *m_axisV;
};
