#pragma once

#include "loadfile.h"

#include <QWidget>

#include <vector>

class QTableWidget;
class QTableWidgetItem;

// A two-column "Property | Value" table with one or more titled sections, like the input panels of
// the original program. Edits are reported with the section id and the entry key.
class QResizeEvent;

class PropertyPanel : public QWidget
{
    Q_OBJECT
public:
    struct Section
    {
        int id;
        QString key;  // "caliber", "gun", ... (settings key of the field units)
        QString title;
        const grt::EntryList *entries;
    };

    explicit PropertyPanel(QWidget *parent = nullptr);

    void setSections(const std::vector<Section> &sections);
    // Remembers the widths of the property and value columns under `key` once the user has changed them.
    void rememberColumns(const QString &key);

signals:
    void valueEdited(int sectionId, const QString &key, const QString &text);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    enum Role
    {
        SectionRole = Qt::UserRole,
        KeyRole,
        FieldRole,
        QuantityRole,
        BaseRole,
        ChoiceRole
    };
    friend class UnitDelegate;
    friend class ValueDelegate;
    void onItemChanged(QTableWidgetItem *item);
    void refreshValues();

    QTableWidget *m_table;
    std::vector<Section> m_sections;
    bool m_rebuilding{false};
    bool m_userSizedColumns{false};
    QString m_settingsKey;
    bool m_adjustingColumns{false};
};
