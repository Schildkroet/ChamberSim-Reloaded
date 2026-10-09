#include "physicalpowders.h"

#include "log.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <mutex>

using namespace Qt::StringLiterals;

namespace csr
{

namespace
{

struct PowderEntry
{
    std::array<double, 4> record;  // Ba, a0, z1, z2 of the record the values were fitted for
    PhysicalBurn burn;
};

struct Store
{
    std::mutex mutex;
    QString path{u":/data/physical/propellants.json"_s};
    bool loaded{};
    std::map<QString, PowderEntry> entries;  // key: "maker name", lower case
};

Store &store()
{
    static Store s;
    return s;
}

QString keyOf(const QString &maker, const QString &name)
{
    return (maker.trimmed() + u' ' + name.trimmed()).toLower();
}

[[nodiscard]] bool same(double a, double b) noexcept
{
    return std::abs(a - b) <= 1e-6 * std::max({1.0, std::abs(a), std::abs(b)});
}

void loadLocked(Store &s)
{
    s.loaded = true;
    s.entries.clear();
    QFile f(s.path);
    if (!f.open(QIODevice::ReadOnly))
    {
        LOG_Info() << "Physical burn values: cannot read" << s.path;
        return;
    }
    const QJsonArray list = QJsonDocument::fromJson(f.readAll()).object().value(u"propellants"_s).toArray();
    for (const auto &item : list)
    {
        const QJsonObject o = item.toObject();
        const QJsonObject r = o.value(u"record"_s).toObject();
        s.entries[keyOf(o.value(u"mname"_s).toString(), o.value(u"pname"_s).toString())] = {
            .record = {r.value(u"Ba"_s).toDouble(), r.value(u"a0"_s).toDouble(), r.value(u"z1"_s).toDouble(), r.value(u"z2"_s).toDouble()},
            .burn = {.Ba = o.value(u"Ba"_s).toDouble(),
                     .a0 = o.value(u"a0"_s).toDouble(),
                     .z1 = o.value(u"z1"_s).toDouble(),
                     .z2 = o.value(u"z2"_s).toDouble()}};
    }
    LOG_Debug() << "Physical burn values:" << s.entries.size() << "powders from" << s.path;
}

QString refText(double Ba, double a0, double z1, double z2)
{
    return QStringList{QString::number(Ba, 'g', 10), QString::number(a0, 'g', 10), QString::number(z1, 'g', 10),
                       QString::number(z2, 'g', 10)}
        .join(u' ');
}

}  // namespace

void setPhysicalBurnFile(const QString &path)
{
    Store &s = store();
    const std::scoped_lock lock(s.mutex);
    s.path = path;
    s.loaded = false;
}

std::optional<PhysicalBurn> physicalBurnOf(const QVariantMap &propellant)
{
    Store &s = store();
    const std::scoped_lock lock(s.mutex);
    if (!s.loaded) loadLocked(s);
    const auto it = s.entries.find(keyOf(propellant.value(u"mname"_s).toString(), propellant.value(u"pname"_s).toString()));
    if (it == s.entries.end()) return std::nullopt;
    const auto &r = it->second.record;
    const std::array<double, 4> values{propellant.value(u"Ba"_s).toDouble(), propellant.value(u"a0"_s).toDouble(),
                                       propellant.value(u"z1"_s).toDouble(), propellant.value(u"z2"_s).toDouble()};
    for (std::size_t i = 0; i < r.size(); ++i)
        if (!same(r[i], values[i])) return std::nullopt;
    return it->second.burn;
}

std::optional<PhysicalBurn> findPhysicalBurn(const QString &maker, const QString &name)
{
    Store &s = store();
    const std::scoped_lock lock(s.mutex);
    if (!s.loaded) loadLocked(s);
    const auto it = s.entries.find(keyOf(maker, name));
    if (it == s.entries.end()) return std::nullopt;
    return it->second.burn;
}

void writePhysicalBurn(EntryList &propellant, const std::optional<PhysicalBurn> &burn)
{
    if (!burn)
    {
        std::erase_if(propellant.entries(), [](const csr::Entry &e) { return e.name.startsWith(u"phys"); });
        return;
    }
    // like the original's entries: a type and an English description (the property panel's label)
    const auto put = [&](QStringView name, const QString &value, const QString &type, const QString &descr)
    {
        propellant.set(name, value);
        for (csr::Entry &e : propellant.entries())
            if (e.name == name)
            {
                e.type = type;
                e.descr = descr;
            }
    };
    const auto number = [](double v) { return QString::number(v, 'g', 10); };
    put(u"physBa", number(burn->Ba), u"decimal"_s, u"physical model: vivacity coefficient"_s);
    put(u"physA0", number(burn->a0), u"decimal"_s, u"physical model: prog/deg coefficient"_s);
    put(u"physZ1", number(burn->z1), u"decimal"_s, u"physical model: burn-up limit z1"_s);
    put(u"physZ2", number(burn->z2), u"decimal"_s, u"physical model: burn-up limit z2"_s);
    put(u"physRef", refText(propellant.number(u"Ba"), propellant.number(u"a0"), propellant.number(u"z1"), propellant.number(u"z2")),
        u"string"_s, u"burn values the physical ones belong to"_s);
}

std::optional<PhysicalBurn> readPhysicalBurn(const EntryList &propellant)
{
    const PhysicalBurn b{.Ba = propellant.number(u"physBa"),
                         .a0 = propellant.number(u"physA0"),
                         .z1 = propellant.number(u"physZ1"),
                         .z2 = propellant.number(u"physZ2")};
    if (b.Ba <= 0.0 || b.z2 <= b.z1) return std::nullopt;
    const QStringList ref = propellant.text(u"physRef").split(u' ', Qt::SkipEmptyParts);
    if (ref.size() != 4) return std::nullopt;
    const std::array<double, 4> now{propellant.number(u"Ba"), propellant.number(u"a0"), propellant.number(u"z1"), propellant.number(u"z2")};
    for (std::size_t i = 0; i < now.size(); ++i)
        if (!same(ref[static_cast<qsizetype>(i)].toDouble(), now[i])) return std::nullopt;
    return b;
}

bool attachPhysicalBurn(EntryList &propellant)
{
    if (readPhysicalBurn(propellant)) return false;
    QVariantMap record;
    for (const char16_t *key : {u"mname", u"pname"}) record.insert(QString::fromUtf16(key), propellant.text(key));
    for (const char16_t *key : {u"Ba", u"a0", u"z1", u"z2"}) record.insert(QString::fromUtf16(key), propellant.number(key));
    const auto burn = physicalBurnOf(record);
    if (!burn) return false;
    writePhysicalBurn(propellant, burn);
    return true;
}

}  // namespace csr
