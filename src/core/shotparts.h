#pragma once

#include "loadfile.h"

#include <QString>
#include <QStringList>

#include <array>
#include <vector>

namespace grt
{

// The pressure of published shotshell loads depends on the shell (hull), the wad and the primer of the load (the shell alone: up to +-20 %
// at the same velocity), which the shotshell model does not know. A load names them in the caliber entries shothull, shotwad and shotprimer;
// every part has two factors on the shotshell model (fitted by analysis/fit_shot_factors.py to the Hodgdon, Alliant and Vectan data): the
// powder space (the volume of the hull, the cushion and gas seal of the wad) and the gas energy. The factors of a load multiply.
enum class ShotPart
{
    Shell,
    Wad,
    Primer
};
inline constexpr std::array<ShotPart, 3> kShotParts{ShotPart::Shell, ShotPart::Wad, ShotPart::Primer};

struct ShotPartInfo
{
    QString id;                 // stored in the load file, e.g. "rem-tgt12"
    QString name;               // shown to the user
    int gauge{};                // wads: the gauge they are made for (0: any)
    double capacityScale{1.0};  // x Calibration::shotCapacityFactor
    double energyScale{1.0};    // x Calibration::shotEnergyFactor
    int loads{};                // published loads the factors were fitted to
    // a group of similar parts (low, medium or high pressure) instead of one part: its rank and some of its members
    bool group{false};
    QString rank;         // "low", "medium" or "high" (the pressure the parts give)
    QString examples;     // the members with most loads, e.g. "Federal Gold Medal (plastic), Federal Gold Medal (paper)"
    QStringList members;  // all members (names), most loads first
};

// The caliber entry that holds the part of a load: "shothull", "shotwad" or "shotprimer".
[[nodiscard]] QString shotPartKey(ShotPart part);

// The file the parts are read from (default ":/data/shotshell/parts.json", read once on first use); tools and tests set another one.
void setShotPartsFile(const QString &path);

// All parts of a kind, sorted by gauge and name.
[[nodiscard]] const std::vector<ShotPartInfo> &shotParts(ShotPart part);

// General entries for those who do not know the exact part: groups of parts with a similar effect (id "group-low", "group-medium",
// "group-high": low-pressure parts first). Held-out loads: three groups of every kind are almost as good as all parts (10.6 % against
// 10.3 % pressure error).
[[nodiscard]] const std::vector<ShotPartInfo> &shotPartGroups(ShotPart part);

// The part or group with this id, or null (none chosen, or an id this version does not know: the shotshell model's average is used).
[[nodiscard]] const ShotPartInfo *findShotPart(ShotPart part, const QString &id);

// The factors of the shell, wad and primer a shotshell load names (1 for those it does not name).
struct ShotFactors
{
    double capacity{1.0};
    double energy{1.0};
};
[[nodiscard]] ShotFactors shotFactorsOf(const EntryList &caliber);

// Adds the entries shothull, shotwad and shotprimer (empty: not chosen) to the caliber of a shotshell load that has none; returns whether it did.
bool ensureShotParts(EntryList &caliber);

}  // namespace grt
