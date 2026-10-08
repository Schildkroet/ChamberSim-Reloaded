#pragma once

#include "database.h"

#include <QWidget>

#include <memory>

class QLabel;
class QLineEdit;
class SearchFilterProxy;
class QSqlTableModel;
class QTableView;

// Sortable, filterable view of one database table (caliber, projectile, propellant).
class TableBrowser : public QWidget
{
    Q_OBJECT
public:
    explicit TableBrowser(grt::Table table, QWidget *parent = nullptr);
    ~TableBrowser() override;

    // Binds the view to an open database (or detaches it when `db` is null).
    void setDatabase(const grt::Database *db);

    // Restricts the rows with an extra SQL condition (e.g. "gdia BETWEEN 8 AND 8.5"); empty = no restriction.
    void setRowFilter(const QString &sqlCondition);

    // Selects the row with this id; returns false if it is not shown.
    bool selectId(int id);

    // Row id (primary key) of the selected row, or -1.
    [[nodiscard]] int currentId() const;

    // A numeric column of the selected row (0 when nothing is selected or the table has no such column).
    [[nodiscard]] double currentNumber(const QString &column) const;
    // Reads the table again (after records were added or changed), keeping the selection if possible.
    void refresh();

signals:
    void currentIdChanged(int id);
    void idActivated(int id);  // double click / Enter on a row

private:
    void updateCount();
    // Reads all rows of the model (the tables are small enough to load completely); logs a failed query.
    void selectAll();
    [[nodiscard]] QString baseFilter() const;

    grt::Table m_table;
    QString m_rowFilter;
    QTableView *m_view;
    QLineEdit *m_filter;
    QLabel *m_count;
    QSqlTableModel *m_model{nullptr};
    SearchFilterProxy *m_proxy;
};
