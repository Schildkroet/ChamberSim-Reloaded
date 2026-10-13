/*
 * This file is part of ChamberSim Reloaded.
 *
 * Copyright (C) 2026 Schildkroet
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
        const csr::EntryList *entries;
    };

    explicit PropertyPanel(QWidget *parent = nullptr);

    void setSections(const std::vector<Section> &sections);
    // Remembers the widths of the property and value columns under `key` once the user has changed them.
    void rememberColumns(const QString &key);
    // Shows the values the entries have now (after the program changed some of them, e.g. a derived length).
    void syncValues();

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
