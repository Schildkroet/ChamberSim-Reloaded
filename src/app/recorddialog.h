#pragma once

#include "database.h"
#include "dbfields.h"

#include <QDialog>
#include <QVariantMap>

#include <optional>
#include <vector>

class QWidget;

// Creates or edits a caliber, projectile or propellant record: all columns in groups with readable labels, numbers
// in the user's units. The caller stores the result (Database::insert / update).
class RecordDialog : public QDialog {
    Q_OBJECT
public:
    enum class Mode { New, Copy, Edit };

    // `record` is the record to copy or edit (empty for a new one).
    RecordDialog(const grt::Database &db, grt::Table table, Mode mode, const QVariantMap &record = {}, QWidget *parent = nullptr);

    // The values in database units, without the id (also sets the creation/change date).
    [[nodiscard]] QVariantMap values() const;

    // Text of a column's editor, and setting it (as typed by the user).
    [[nodiscard]] QString text(const QString &column) const;
    void setText(const QString &column, const QString &text);

    // Checks the input; returns the label of the first wrong field (empty: all fine).
    [[nodiscard]] QString firstError() const;

    void accept() override;

private:
    struct Editor {
        grt::dbfields::Field field;
        QWidget *widget;
    };

    [[nodiscard]] const Editor *editor(const QString &column) const;
    [[nodiscard]] std::optional<QVariant> valueOf(const Editor &e) const;

    grt::Table m_table;
    Mode m_mode;
    QVariantMap m_record;
    std::vector<Editor> m_editors;
};
