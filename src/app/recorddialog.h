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

#include "database.h"
#include "dbfields.h"

#include <QDialog>
#include <QVariantMap>

#include <array>
#include <optional>
#include <vector>

class QImage;
class QVBoxLayout;
class QLabel;
class QLineEdit;
class QWidget;

// Creates or edits a caliber, projectile or propellant record: all columns in groups with readable labels, numbers
// in the user's units. The caller stores the result (Database::insert / update).
class RecordDialog : public QDialog
{
    Q_OBJECT
public:
    enum class Mode
    {
        New,
        Copy,
        Edit
    };

    // `record` is the record to copy or edit (empty for a new one).
    RecordDialog(const csr::Database &db, csr::Table table, Mode mode, const QVariantMap &record = {}, QWidget *parent = nullptr);

    // The values in database units, without the id (also sets the creation/change date).
    [[nodiscard]] QVariantMap values() const;

    // Text of a column's editor, and setting it (as typed by the user).
    [[nodiscard]] QString text(const QString &column) const;
    void setText(const QString &column, const QString &text);

    // Photos of a propellant (slot 0 overview, 1 detail): a picture chosen by the user is scaled down and kept
    // here, the caller stores it with Database::addImage before the record (see pendingImages). `pxPerMm` 0: no scale.
    void setPhoto(int slot, const QImage &image, double pxPerMm, const QString &name);
    void removePhoto(int slot);
    // The new pictures (with their uuids, which values() refers to); empty without a change.
    [[nodiscard]] std::vector<csr::Database::Image> pendingImages() const;

    // Checks the input; returns the label of the first wrong field (empty: all fine).
    [[nodiscard]] QString firstError() const;

    void accept() override;

private:
    struct Editor
    {
        csr::dbfields::Field field;
        QWidget *widget;
    };

    void calculateBoreCrossSection();
    [[nodiscard]] const Editor *editor(const QString &column) const;
    [[nodiscard]] std::optional<QVariant> valueOf(const Editor &e) const;

    struct PhotoSlot
    {
        QString column;  // imageUuid / imageDetailUuid
        QString uuid;    // the picture the record refers to ("": none)
        std::optional<csr::Database::Image> pending;
        QLabel *preview{};
        QLabel *info{};
        QLineEdit *scale{};
    };
    void addPhotoGroup(QVBoxLayout *layout);
    void showPhoto(int slot);

    const csr::Database *m_db;
    std::array<PhotoSlot, 2> m_photos;
    csr::Table m_table;
    Mode m_mode;
    QVariantMap m_record;
    std::vector<Editor> m_editors;
};
