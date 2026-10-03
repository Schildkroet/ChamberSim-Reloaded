#pragma once

#include "loadfile.h"

#include <QString>
#include <QVariantMap>

#include <optional>

namespace grt
{

// Burn values of a powder for the physical model (CalibrationProfile::Physical): the burn rate Ba and the form function (a0, z1, z2)
// fitted with that model to the powder's published loads (velocities and measured pressures; analysis/fit_physical_powders.py). The
// project's records carry the values of the original program's model; the physical ones are kept apart in
// data/physical/propellants.json, together with the record values they were fitted for.
struct PhysicalBurn
{
    double Ba{};
    double a0{};
    double z1{};
    double z2{};
};

// The file the values are read from (default ":/data/physical/propellants.json", read once on first use); tools and tests set the
// source directory's file. Setting it discards what was read.
void setPhysicalBurnFile(const QString &path);

// The physical values of a propellant record (mname, pname, Ba, a0, z1, z2): only when the file has the powder and the record's
// values are the ones the physical values were fitted for (a changed or other record of the same powder gets none).
[[nodiscard]] std::optional<PhysicalBurn> physicalBurnOf(const QVariantMap &propellant);

// In a load file the values are the propellant entries physBa, physA0, physZ1, physZ2 and physRef (the record's "Ba a0 z1 z2" they
// belong to). write() sets them, or removes them for std::nullopt; read() returns them only while the load's Ba, a0, z1 and z2 still
// equal physRef (after an edit of those values the physical ones no longer apply).
void writePhysicalBurn(EntryList &propellant, const std::optional<PhysicalBurn> &burn);
[[nodiscard]] std::optional<PhysicalBurn> readPhysicalBurn(const EntryList &propellant);

}  // namespace grt
