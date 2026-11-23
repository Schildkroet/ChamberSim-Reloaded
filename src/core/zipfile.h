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

#include <QByteArray>
#include <QString>

#include <optional>

namespace csr
{

// True if the data starts like a ZIP archive (local file header).
[[nodiscard]] bool isZip(const QByteArray &data);

// The content of one file of a ZIP archive held in memory (stored or deflated entries; no encryption, no ZIP64), found through the
// central directory. nullopt if the archive is damaged or has no such entry. Small and self-contained, for the .xlsx files of chronograph
// apps; `maxSize` bounds the unpacked size.
[[nodiscard]] std::optional<QByteArray> zipEntry(const QByteArray &zip, const QString &name, qint64 maxSize = 64 * 1024 * 1024);

}  // namespace csr
