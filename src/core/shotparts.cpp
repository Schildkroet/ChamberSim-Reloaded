#include "shotparts.h"

#include "log.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <mutex>

using namespace Qt::StringLiterals;

namespace grt
{

namespace
{

constexpr std::size_t index(ShotPart part) noexcept
{
    return static_cast<std::size_t>(part);
}

struct Store
{
    std::mutex mutex;
    QString path{u":/data/shotshell/parts.json"_s};
    bool loaded{};
    std::array<std::vector<ShotPartInfo>, 3> parts;
};

Store &store()
{
    static Store s;
    return s;
}

void loadLocked(Store &s)
{
    s.loaded = true;
    for (auto &list : s.parts) list.clear();
    QFile f(s.path);
    if (!f.open(QIODevice::ReadOnly))
    {
        LOG_Info() << "Shotshell parts: cannot read" << s.path;
        return;
    }
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    constexpr std::array<const char16_t *, 3> lists{u"shells", u"wads", u"primers"};
    for (const ShotPart part : kShotParts)
    {
        auto &list = s.parts[index(part)];
        for (const auto &item : root.value(QString::fromUtf16(lists[index(part)])).toArray())
        {
            const QJsonObject o = item.toObject();
            ShotPartInfo info{.id = o.value(u"id"_s).toString(),
                              .name = o.value(u"name"_s).toString(),
                              .gauge = o.value(u"gauge"_s).toInt(),
                              .capacityScale = o.value(u"capacity"_s).toDouble(1.0),
                              .energyScale = o.value(u"energy"_s).toDouble(1.0),
                              .loads = o.value(u"loads"_s).toInt()};
            if (!info.id.isEmpty()) list.push_back(std::move(info));
        }
        std::ranges::sort(list, [](const ShotPartInfo &a, const ShotPartInfo &b)
                          { return a.gauge != b.gauge ? a.gauge < b.gauge : a.name < b.name; });
    }
    LOG_Debug() << "Shotshell parts:" << s.parts[0].size() << "shells," << s.parts[1].size() << "wads," << s.parts[2].size()
                << "primers from" << s.path;
}

}  // namespace

QString shotPartKey(ShotPart part)
{
    constexpr std::array<const char16_t *, 3> keys{u"shothull", u"shotwad", u"shotprimer"};
    return QString::fromUtf16(keys[index(part)]);
}

void setShotPartsFile(const QString &path)
{
    Store &s = store();
    const std::scoped_lock lock(s.mutex);
    s.path = path;
    s.loaded = false;
}

const std::vector<ShotPartInfo> &shotParts(ShotPart part)
{
    Store &s = store();
    const std::scoped_lock lock(s.mutex);
    if (!s.loaded) loadLocked(s);
    return s.parts[index(part)];  // not changed after loading (tools replace the file before they use it)
}

const ShotPartInfo *findShotPart(ShotPart part, const QString &id)
{
    if (id.isEmpty()) return nullptr;
    const auto &list = shotParts(part);
    const auto it = std::ranges::find(list, id, &ShotPartInfo::id);
    return it == list.end() ? nullptr : &*it;
}

ShotFactors shotFactorsOf(const EntryList &caliber)
{
    ShotFactors f;
    for (const ShotPart part : kShotParts)
        if (const ShotPartInfo *info = findShotPart(part, caliber.text(shotPartKey(part))))
        {
            f.capacity *= info->capacityScale;
            f.energy *= info->energyScale;
        }
    return f;
}

bool ensureShotParts(EntryList &caliber)
{
    if (!caliber.flag(u"shotshell")) return false;
    bool added = false;
    // like the original's entries: a type and an English description (the property panel's label)
    constexpr std::array<const char16_t *, 3> descriptions{u"shotshell hull", u"shotshell wad", u"shotshell primer"};
    for (const ShotPart part : kShotParts)
    {
        const QString key = shotPartKey(part);
        if (caliber.find(key)) continue;
        Entry e;
        e.name = key;
        e.type = u"string"_s;
        e.descr = QString::fromUtf16(descriptions[index(part)]);
        caliber.entries().push_back(std::move(e));
        added = true;
    }
    return added;
}

}  // namespace grt
