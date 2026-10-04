#pragma once

#include "units.h"

#include <QDialog>

class QComboBox;
class QLineEdit;
class QTableWidget;

// Tools > Unit converter: a value in one unit of a quantity, shown in all units of that quantity (the units of the program, see units.h).
class UnitConverterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UnitConverterDialog(QWidget *parent = nullptr);

    void setQuantity(grt::units::Quantity quantity);
    void setFromUnit(const QString &unitId);
    void setInput(const QString &text);
    [[nodiscard]] grt::units::Quantity quantity() const;
    // The converted values as shown, one row per unit: "symbol" and "value".
    [[nodiscard]] int rowCount() const;
    [[nodiscard]] QString symbolAt(int row) const;
    [[nodiscard]] QString valueAt(int row) const;

private:
    void fillUnits();
    void convert();

    QComboBox *m_quantity;
    QLineEdit *m_input;
    QComboBox *m_from;
    QTableWidget *m_table;
};
