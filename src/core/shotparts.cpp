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
    std::array<std::vector<ShotPartInfo>, 3> groups;
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
    for (auto &list : s.groups) list.clear();
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
                              .loads = o.value(u"loads"_s).toInt(),
                              .groupId = o.value(u"group"_s).toString(),
                              .group = false,
                              .rank = {},
                              .members = {}};
            if (!info.id.isEmpty()) list.push_back(std::move(info));
        }
        std::ranges::sort(list, [](const ShotPartInfo &a, const ShotPartInfo &b)
                          { return a.gauge != b.gauge ? a.gauge < b.gauge : a.name < b.name; });
        // the groups of similar parts: low, medium and high pressure, in this order
        constexpr std::array<const char16_t *, 3> groupLists{u"shell_groups", u"wad_groups", u"primer_groups"};
        for (const auto &item : root.value(QString::fromUtf16(groupLists[index(part)])).toArray())
        {
            const QJsonObject o = item.toObject();
            ShotPartInfo info{.id = o.value(u"id"_s).toString(),
                              .name = {},
                              .gauge = 0,
                              .capacityScale = o.value(u"capacity"_s).toDouble(1.0),
                              .energyScale = o.value(u"energy"_s).toDouble(1.0),
                              .loads = o.value(u"loads"_s).toInt(),
                              .groupId = {},
                              .group = true,
                              .rank = o.value(u"rank"_s).toString(),
                              .members = {}};
            for (const auto &m : o.value(u"members"_s).toArray()) info.members << m.toString();
            info.name = info.rank;
            if (!info.id.isEmpty()) s.groups[index(part)].push_back(std::move(info));
        }
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

const std::vector<ShotPartInfo> &shotPartGroups(ShotPart part)
{
    Store &s = store();
    const std::scoped_lock lock(s.mutex);
    if (!s.loaded) loadLocked(s);
    return s.groups[index(part)];
}

const ShotPartInfo *findShotPart(ShotPart part, const QString &id)
{
    if (id.isEmpty()) return nullptr;
    for (const auto *list : {&shotParts(part), &shotPartGroups(part)})
        if (const auto it = std::ranges::find(*list, id, &ShotPartInfo::id); it != list->end()) return &*it;
    return nullptr;
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
    bool changed = false;
    // like the original's entries: a type and an English description (the property panel's label)
    constexpr std::array<const char16_t *, 3> descriptions{u"shotshell hull", u"shotshell wad", u"shotshell primer"};
    for (const ShotPart part : kShotParts)
    {
        const QString key = shotPartKey(part);
        if (const Entry *existing = caliber.find(key))
        {
            // an individual part (a file of an earlier version, or a tool): the group it belongs to is offered instead
            const auto &list = shotParts(part);
            const auto it = std::ranges::find(list, existing->value, &ShotPartInfo::id);
            if (it != list.end() && !it->groupId.isEmpty())
            {
                caliber.set(key, it->groupId);
                changed = true;
            }
            continue;
        }
        Entry e;
        e.name = key;
        e.type = u"string"_s;
        e.descr = QString::fromUtf16(descriptions[index(part)]);
        caliber.entries().push_back(std::move(e));
        changed = true;
    }
    return changed;
}

}  // namespace grt
