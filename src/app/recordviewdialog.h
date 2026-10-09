#pragma once

#include "database.h"

#include <QDialog>
#include <QVariantMap>

// Read-only view of one caliber, projectile or propellant record: all columns in groups, numbers in the user's units
// (opened by a double click on a record). The buttons offer to edit the record (own records) or to copy it.
class RecordViewDialog : public QDialog
{
    Q_OBJECT
public:
    enum Result
    {
        Closed = QDialog::Rejected,
        EditRequested = 2,
        CopyRequested = 3
    };

    RecordViewDialog(const csr::Database &db, csr::Table table, int id, const QVariantMap &record, QWidget *parent = nullptr);

    // Number of fields shown, and the text shown for a column ("" without it).
    [[nodiscard]] int fieldCount() const noexcept { return m_fieldCount; }
    [[nodiscard]] QString valueText(const QString &column) const;

private:
    int m_fieldCount{0};
};
