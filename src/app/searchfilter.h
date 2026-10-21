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

#include <algorithm>

#include <QHash>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStringList>

// Tolerant text search for the record tables: "8x57", "8 x 57", "8X57" and "8×57" all find "8 x 57 IS", "30 06" finds "30-06 Springfield",
// "9mm" finds "9 mm Luger". Case, accents, spaces, dots, hyphens and other punctuation are ignored, and so is an "x" between two digits.
namespace searchfilter
{

// `dropX`: also remove an "x" between digits ("8 x 57" -> "857"); the plain form keeps it, so that a partial entry like "8x" still matches.
[[nodiscard]] inline QString normalized(const QString &text, bool dropX = true)
{
    QString out;
    for (const QChar c : text.normalized(QString::NormalizationForm_D))
    {
        if (c.category() == QChar::Mark_NonSpacing) continue;  // accents
        if (c.isLetterOrNumber()) out += c.toLower();
        else if (c == QLatin1Char('*')) out += QLatin1Char('x');
    }
    out.replace(QChar(0x00D7), QLatin1Char('x'));  // multiplication sign
    // an "x" between digits is a separator ("8 x 57", "6.5x55"), the table may spell it without ("6.5 - 284")
    for (qsizetype i = out.size() - 2; dropX && i >= 1; --i)
        if (out[i] == QLatin1Char('x') && out[i - 1].isDigit() && out[i + 1].isDigit()) out.remove(i, 1);
    return out;
}

// All words of the query (or the whole query) are found in the cells of one row.
[[nodiscard]] inline bool matches(const QString &query, const QStringList &cells)
{
    if (normalized(query).isEmpty()) return true;
    QStringList dropped, kept;
    for (const QString &c : cells)
    {
        dropped += normalized(c);
        kept += normalized(c, false);
    }
    // the text in either spelling of the "x" ("8x" is found in "8x57is", "857" in "857is")
    const auto found = [&](const QString &text)
    {
        const QString d = normalized(text), k = normalized(text, false);
        return std::ranges::any_of(dropped, [&](const QString &c) { return c.contains(d); }) ||
               std::ranges::any_of(kept, [&](const QString &c) { return c.contains(k); });
    };
    if (found(query)) return true;
    // several words: every word in some cell ("lovex s020", "hornady 115")
    const QStringList words = query.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    // not for numbers only ("8 x 57" is one cartridge name, "8" and "57" would match anywhere)
    const auto isName = [](const QString &w)
    { return std::ranges::any_of(w, [](QChar c) { return c.isLetter() && c.toLower() != QLatin1Char('x'); }); };
    if (words.size() < 2 || !std::ranges::any_of(words, isName)) return false;
    return std::ranges::all_of(words, [&](const QString &w)
                               { return normalized(w).isEmpty() || normalized(w) == QLatin1String("x") || found(w); });
}

// How well a text cell fits the query, for ranking the rows: 4000 = the whole cell, 3000 = its beginning, 2000 = somewhere in it (plus up to 999 for the share
// of the cell the query covers; either spelling of the "x"), 0 = not at all. The number columns of a record (a diameter of 11.63 for ".458") only count as 1 where they contain it.
[[nodiscard]] inline int cellScore(const QString &query, const QString &cell, bool isText)
{
    const QString qd = normalized(query), qk = normalized(query, false);
    if (qd.isEmpty()) return 0;
    const QString cd = normalized(cell), ck = normalized(cell, false);
    int best = 0;
    for (const auto &[q, c] : {std::pair{qd, cd}, std::pair{qk, ck}})
    {
        const int fill = static_cast<int>(999 * q.size() / std::max<qsizetype>(c.size(), 1));  // the shorter cell fits better
        if (c == q) best = std::max(best, 4000 + fill);
        else if (c.startsWith(q)) best = std::max(best, 3000 + fill);
        else if (c.contains(q)) best = std::max(best, 2000 + fill);
    }
    return best == 0 ? 0 : isText ? best : 1;
}

}  // namespace searchfilter

// Proxy model that filters with searchfilter::matches over one column or, with the key column -1, over all columns.
class SearchFilterProxy : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    void setQuery(const QString &query)
    {
        m_query = query.trimmed();
        m_scores.clear();
        invalidate();  // filter and order: the rows that fit best first
    }

    // While a query is entered and the table is sorted by this column (the default order, -1: never), the rows whose text fits the query
    // best come first (a cell that is the query, then one that starts with it, then one that contains it); the column sorts within them.
    void setRankedColumn(int column) { m_rankedColumn = column; }

protected:
    [[nodiscard]] bool filterAcceptsRow(int row, const QModelIndex &parent) const override
    {
        if (m_query.isEmpty() || !sourceModel()) return true;
        QStringList cells;
        int score = 0;
        const int key = filterKeyColumn();
        const int columns = sourceModel()->columnCount(parent);
        for (int c = 0; c < columns; ++c)
        {
            if (key >= 0 && key != c) continue;
            const QVariant v = sourceModel()->index(row, c, parent).data(filterRole());
            cells += v.toString();
            score = std::max(score, searchfilter::cellScore(m_query, cells.back(), v.typeId() == QMetaType::QString));
        }
        const bool ok = searchfilter::matches(m_query, cells);
        if (ok) m_scores.insert(row, score);
        return ok;
    }

    [[nodiscard]] bool lessThan(const QModelIndex &left, const QModelIndex &right) const override
    {
        if (!m_query.isEmpty() && m_rankedColumn >= 0 && sortColumn() == m_rankedColumn)
        {
            const int a = m_scores.value(left.row()), b = m_scores.value(right.row());
            // the better fit first in either sort order (a descending sort turns the result around)
            if (a != b) return sortOrder() == Qt::AscendingOrder ? a > b : a < b;
        }
        return QSortFilterProxyModel::lessThan(left, right);
    }

private:
    QString m_query;
    int m_rankedColumn = -1;
    mutable QHash<int, int> m_scores;  // source row -> how well it fits the query (filled by the filter)
};
