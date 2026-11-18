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

#include "log.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include <mutex>
#include <optional>
#include <utility>

namespace csr
{

// The top-level object of a JSON file; empty (and logged, as `what`) when the file cannot be read.
[[nodiscard]] inline QJsonObject readJsonObject(const QString &path, const char *what)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        LOG_Info() << what << "- cannot read" << path;
        return {};
    }
    return QJsonDocument::fromJson(f.readAll()).object();
}

// Data read from a file on first use (thread-safe); setPath() discards it.
template <class Data>
class LazyFile
{
public:
    using Loader = Data (*)(const QString &path);

    LazyFile(QString path, Loader load)
        : m_path(std::move(path))
        , m_load(load)
    {
    }

    // f(const Data &) under the lock; references into the data stay valid until the next setPath()
    template <class F>
    decltype(auto) use(F &&f)
    {
        const std::scoped_lock lock(m_mutex);
        if (!m_data) m_data = m_load(m_path);
        return std::forward<F>(f)(*m_data);
    }

    void setPath(QString path)
    {
        const std::scoped_lock lock(m_mutex);
        m_path = std::move(path);
        m_data.reset();
    }

private:
    std::mutex m_mutex;
    QString m_path;
    Loader m_load;
    std::optional<Data> m_data;
};

}  // namespace csr
