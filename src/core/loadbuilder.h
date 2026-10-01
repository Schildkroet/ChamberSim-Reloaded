#pragma once

#include "loadfile.h"

#include <QStringList>
#include <QVariantMap>

namespace grt {

inline constexpr double kGrainToGram = 0.06479890999975;  // the exact factor of the original program

// What the user chooses besides the three database records. A value of 0 means "use the default".
struct LoadParams {
    double charge_g{};              // default: 70 % of the bulk-density capacity of the case
    double barrelLength_mm{};       // default: depends on the cartridge type (pistol/rimfire/rifle)
    double seatingDepth_mm{};       // default: overall length = the cartridge's maximum length
    double twistLength_mm{240.0};
    double gunWeight_kg{};
    double powderTemperature_C{21.0};
};

// Case capacity in cm^3 (the database stores grains of water).
[[nodiscard]] double caseVolume_cm3(const QVariantMap &caliber);
[[nodiscard]] double defaultBarrelLength_mm(const QVariantMap &caliber);
[[nodiscard]] double defaultSeatingDepth_mm(const QVariantMap &caliber, const QVariantMap &projectile);
[[nodiscard]] double defaultCharge_g(const QVariantMap &caliber, const QVariantMap &propellant);

// Replaces one component of an existing load by a database record. Everything else the user has set
// (gun data, seating depth, charge, temperature, ...) is kept unless `resetDependent` asks for the
// defaults that belong to the new record: the charge and seating depth for a caliber change, the
// seating depth for a bullet, the charge for a powder. The title and the overall length are updated.
void applyCaliber(LoadFile &load, const QVariantMap &caliber, const QStringList &caliberColumns = {}, bool resetDependent = false,
                  const QVariantMap *projectile = nullptr, const QVariantMap *propellant = nullptr);
void applyProjectile(LoadFile &load, const QVariantMap &projectile, bool resetDependent = false, const QVariantMap *caliber = nullptr);
void applyPropellant(LoadFile &load, const QVariantMap &propellant, bool resetDependent = false, const QVariantMap *caliber = nullptr);

// Builds a complete load from one record of each table. `caliberColumns` (schema order) keeps the
// <caliberfile> block in the order of the original; without it the keys are sorted.
[[nodiscard]] LoadFile buildLoad(const QVariantMap &caliber, const QVariantMap &projectile, const QVariantMap &propellant,
                                 LoadParams params = {}, const QStringList &caliberColumns = {});

}  // namespace grt
