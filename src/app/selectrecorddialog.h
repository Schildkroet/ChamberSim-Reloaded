#pragma once

#include "database.h"
#include "tablebrowser.h"

#include <QDialog>

class QCheckBox;
class QDialogButtonBox;

// Pick one record of a database table (caliber, projectile or propellant).
class SelectRecordDialog : public QDialog {
    Q_OBJECT
public:
    SelectRecordDialog(const grt::Database &db, grt::Table table, const QString &title, QWidget *parent = nullptr);

    // Optional restriction, e.g. bullets that fit the caliber. The check box switches it on and off.
    void setRestriction(const QString &sqlCondition, const QString &checkBoxText);
    // Optional "also reset dependent values" choice shown below the table.
    void setResetOption(const QString &text, bool checked);

    bool select(int id);
    [[nodiscard]] int selectedId() const { return m_browser->currentId(); }
    [[nodiscard]] bool resetChecked() const;

private:
    void updateOk();

    TableBrowser *m_browser;
    QCheckBox *m_restrict{nullptr};
    QCheckBox *m_reset{nullptr};
    QDialogButtonBox *m_buttons;
    QString m_condition;
};
