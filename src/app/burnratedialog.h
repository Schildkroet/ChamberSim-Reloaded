#pragma once

#include "database.h"

#include <QDialog>

class QButtonGroup;
class QCheckBox;
class QLabel;
class QLineEdit;
class SearchFilterProxy;
class QStandardItemModel;
class QTableView;

// The burn rate table: all propellants with their burn factors, sorted by progressivity (Bp), brisance (Br) or both (Brp).
// A rough overview of how the powders of the different manufacturers compare, not a substitute table.
class BurnRateDialog : public QDialog
{
    Q_OBJECT
public:
    explicit BurnRateDialog(const csr::Database &db, QWidget *parent = nullptr);
    ~BurnRateDialog() override;

    // Column of the factor the table is sorted by: Br, Bp or Brp.
    enum Factor
    {
        Brisance,
        Progressivity,
        Combined
    };
    void setFactor(Factor factor);
    [[nodiscard]] Factor factor() const noexcept { return m_factor; }
    [[nodiscard]] int rowCount() const;
    // Row `row` (in the order shown): "Manufacturer Name".
    [[nodiscard]] QString powderAt(int row) const;
    [[nodiscard]] double valueAt(int row, Factor factor) const;
    // Filters the table by manufacturer or name (empty: all).
    void setSearch(const QString &text);

private:
    void load(const csr::Database &db);
    void applyFilter();

    QStandardItemModel *m_model;
    SearchFilterProxy *m_proxy;
    QTableView *m_view;
    QLineEdit *m_search;
    QCheckBox *m_allLots;
    QButtonGroup *m_buttons;
    QLabel *m_count;
    const csr::Database &m_db;
    Factor m_factor{Progressivity};
};
