#pragma once

#include "powderfit.h"

#include <QDialog>

#include <functional>
#include <map>
#include <optional>

class QCheckBox;
class QLabel;
class QPushButton;
class QTableWidget;

// "Fit to measurements": changes the load's propellant values (Ba, a0, z1, z2) until the simulation agrees with the
// measured charges, shows the difference per charge before and after, and applies the values to the load or saves
// them as an own propellant record.
class PowderFitDialog : public QDialog {
    Q_OBJECT
public:
    PowderFitDialog(std::vector<grt::powderfit::Sample> samples, QStringList warnings, QWidget *parent = nullptr);

    // Called by "Save as own propellant…" with the result (the dialog stays open); none: the button is hidden.
    void setSaveHandler(std::function<bool(const grt::powderfit::Result &)> handler);

    void setParams(const std::vector<grt::powderfit::Param> &params);
    void runFit();
    [[nodiscard]] const std::optional<grt::powderfit::Result> &result() const noexcept { return m_result; }

private:
    void showResult();

    std::vector<grt::powderfit::Sample> m_samples;
    std::optional<grt::powderfit::Result> m_result;
    std::function<bool(const grt::powderfit::Result &)> m_save;
    std::map<grt::powderfit::Param, QCheckBox *> m_params;
    QTableWidget *m_table;
    QLabel *m_summary;
    QPushButton *m_fit;
    QPushButton *m_apply;
    QPushButton *m_saveButton;
};
