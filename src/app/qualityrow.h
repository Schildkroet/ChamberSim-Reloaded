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
