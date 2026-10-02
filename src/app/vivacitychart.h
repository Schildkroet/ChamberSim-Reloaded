#pragma once

#include <QChartView>

#include <vector>

class QLineSeries;
class QValueAxis;

// Small chart of the burn-rate coefficient Ba * phi(z) over the burnt fraction z, with the
// transition limits z1 and z2.
class VivacityChart : public QChartView
{
    Q_OBJECT
public:
    explicit VivacityChart(QWidget *parent = nullptr);

    void setCurve(const std::vector<double> &values, double z1, double z2);
    void clear();

protected:
    void changeEvent(QEvent *event) override;

private:
    void applyTheme();
    void applyFonts();

    QLineSeries *m_curve;
    QLineSeries *m_z1;
    QLineSeries *m_z2;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
};
