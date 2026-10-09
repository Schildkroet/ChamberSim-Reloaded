#pragma once

#include "chronograph.h"

#include <QDialog>

class QLabel;
class QTableWidget;

// One series of the file with the charge it was shot with.
struct ImportedCharge
{
    csr::ChronographSeries series;
    double charge_g{};
};

// "Import chronograph file": lists the series of a MagnetoSpeed / Labradar file with their statistics. The files
// do not know the powder charge, so it is entered here (default: the load's charge) and every ticked series
// becomes a charge with its shots in the Measurements tab.
class ChronographImportDialog : public QDialog
{
    Q_OBJECT
public:
    ChronographImportDialog(csr::ChronographFile file, double defaultCharge_g, QWidget *parent = nullptr);

    // The ticked series with their charges (an unreadable charge falls back to the default).
    [[nodiscard]] std::vector<ImportedCharge> selected() const;

    // Test access: tick state and charge text of a row.
    void setChecked(int row, bool on);
    void setChargeText(int row, const QString &text);

private:
    csr::ChronographFile m_file;
    double m_default;
    QTableWidget *m_table;
};
