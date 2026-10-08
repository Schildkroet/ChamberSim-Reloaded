#pragma once

#include <algorithm>

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

}  // namespace searchfilter

// Proxy model that filters with searchfilter::matches over one column or, with the key column -1, over all columns.
class SearchFilterProxy : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    void setQuery(const QString &query)
    {
        m_query = query.trimmed();
        invalidateFilter();
    }

protected:
    [[nodiscard]] bool filterAcceptsRow(int row, const QModelIndex &parent) const override
    {
        if (m_query.isEmpty() || !sourceModel()) return true;
        QStringList cells;
        const int key = filterKeyColumn();
        const int columns = sourceModel()->columnCount(parent);
        for (int c = 0; c < columns; ++c)
            if (key < 0 || key == c) cells += sourceModel()->index(row, c, parent).data(filterRole()).toString();
        return searchfilter::matches(m_query, cells);
    }

private:
    QString m_query;
};
