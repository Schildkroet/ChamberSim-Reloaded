#include "language.h"

#include <algorithm>

namespace grt {

QString normalizeLanguage(const QString &tag)
{
    QString t = tag.section(u'.', 0, 0).section(u'@', 0, 0);  // strip ".UTF-8" and "@euro"
    t.replace(u'-', u'_');
    const qsizetype us = t.indexOf(u'_');
    if (us < 0) return t.toLower();
    return t.left(us).toLower() + u'_' + t.mid(us + 1).toUpper();
}

QString resolveLanguage(const QStringList &requested, const QStringList &available)
{
    const auto languageOf = [](const QString &l) { return l.section(u'_', 0, 0); };
    QStringList avail;
    avail.reserve(available.size());
    std::ranges::transform(available, std::back_inserter(avail), normalizeLanguage);

    for (const QString &raw : requested) {
        const QString want = normalizeLanguage(raw);
        if (want.isEmpty()) continue;
        if (avail.contains(want)) return want;
        if (want == kDefaultLanguage) return kDefaultLanguage;
        const auto sameLanguage = std::ranges::find_if(avail, [&](const QString &a) { return languageOf(a) == languageOf(want); });
        if (sameLanguage != avail.end()) return *sameLanguage;
        if (languageOf(want) == languageOf(kDefaultLanguage)) return kDefaultLanguage;  // any English -> default
    }
    return kDefaultLanguage;
}

}  // namespace grt
