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

#include "fieldnames.h"

#include <QCoreApplication>

#include <algorithm>
#include <string_view>

namespace csr
{

namespace
{

// the descriptions the original program writes into its files (for lupdate; translated at run time)
[[maybe_unused]] constexpr const char *kDescriptions[] = {
    QT_TRANSLATE_NOOP("csr::fields", "G1 ballistic coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "G7 ballistic coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "Sebert factor"),
    QT_TRANSLATE_NOOP("csr::fields", "UBCS-Code (Universal Bullet Classification Scheme)"),
    QT_TRANSLATE_NOOP("csr::fields", "barrel length"),
    QT_TRANSLATE_NOOP("csr::fields", "bulk density"),
    QT_TRANSLATE_NOOP("csr::fields", "bullet jump distance"),
    QT_TRANSLATE_NOOP("csr::fields", "bullet seating depth"),
    QT_TRANSLATE_NOOP("csr::fields", "burn-up limit z1"),
    QT_TRANSLATE_NOOP("csr::fields", "burn-up limit z2"),
    QT_TRANSLATE_NOOP("csr::fields", "caliber name"),
    QT_TRANSLATE_NOOP("csr::fields", "cartridge overall length"),
    QT_TRANSLATE_NOOP("csr::fields", "cartridge test method"),
    QT_TRANSLATE_NOOP("csr::fields", "case length"),
    QT_TRANSLATE_NOOP("csr::fields", "case volume"),
    QT_TRANSLATE_NOOP("csr::fields", "cold temperature coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "covolume of the powder gases"),
    QT_TRANSLATE_NOOP("csr::fields", "effektive area"),
    QT_TRANSLATE_NOOP("csr::fields", "groove caliber"),
    QT_TRANSLATE_NOOP("csr::fields", "gun name"),
    QT_TRANSLATE_NOOP("csr::fields", "gun weight"),
    QT_TRANSLATE_NOOP("csr::fields", "hot temperature coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "initial gas pressure"),
    QT_TRANSLATE_NOOP("csr::fields", "ladder step propellant mass"),
    QT_TRANSLATE_NOOP("csr::fields", "ladder steps (1 to 10, below 2: disabled)"),
    QT_TRANSLATE_NOOP("csr::fields", "material density"),
    QT_TRANSLATE_NOOP("csr::fields", "maximum allowed pressure"),
    QT_TRANSLATE_NOOP("csr::fields", "prog/deg coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile base type"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile diameter"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile lot-id"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile manufacturer"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile material and gas sealing/losses"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile name"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile overall length"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile product name"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile tail diameter A"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile tail diameter B"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile tail length"),
    QT_TRANSLATE_NOOP("csr::fields", "projectile weight"),
    QT_TRANSLATE_NOOP("csr::fields", "propellant detail picture uuid"),
    QT_TRANSLATE_NOOP("csr::fields", "propellant lot-id"),
    QT_TRANSLATE_NOOP("csr::fields", "propellant manufacturer"),
    QT_TRANSLATE_NOOP("csr::fields", "propellant mass"),
    QT_TRANSLATE_NOOP("csr::fields", "propellant picture uuid"),
    QT_TRANSLATE_NOOP("csr::fields", "propellant product name"),
    QT_TRANSLATE_NOOP("csr::fields", "ratio of the specific heat of the powder gases"),
    QT_TRANSLATE_NOOP("csr::fields", "selected ladder curve for display"),
    QT_TRANSLATE_NOOP("csr::fields", "shotshell hull"),
    QT_TRANSLATE_NOOP("csr::fields", "shotshell primer"),
    QT_TRANSLATE_NOOP("csr::fields", "primer"),
    QT_TRANSLATE_NOOP("csr::fields", "additional start pressure (bullet, crimp, case)"),
    QT_TRANSLATE_NOOP("csr::fields", "shotshell wad"),
    QT_TRANSLATE_NOOP("csr::fields", "special bullet resistance inside the barrel"),
    QT_TRANSLATE_NOOP("csr::fields", "bullet material (jacketed or lead: cast, swaged)"),
    QT_TRANSLATE_NOOP("csr::fields", "specific explosive heat"),
    QT_TRANSLATE_NOOP("csr::fields", "technical atmosphere pressure"),
    QT_TRANSLATE_NOOP("csr::fields", "temperature normal"),
    QT_TRANSLATE_NOOP("csr::fields", "true if shotshell"),
    QT_TRANSLATE_NOOP("csr::fields", "twist length"),
    QT_TRANSLATE_NOOP("csr::fields", "unlocked bullet jump"),
    QT_TRANSLATE_NOOP("csr::fields", "unlocked friction"),
    QT_TRANSLATE_NOOP("csr::fields", "vivacity coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "physical model: vivacity coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "physical model: prog/deg coefficient"),
    QT_TRANSLATE_NOOP("csr::fields", "physical model: burn-up limit z1"),
    QT_TRANSLATE_NOOP("csr::fields", "physical model: burn-up limit z2"),
    QT_TRANSLATE_NOOP("csr::fields", "physical model: grain surface shape"),
    QT_TRANSLATE_NOOP("csr::fields", "physical model: grain length / diameter"),
    QT_TRANSLATE_NOOP("csr::fields", "physical model: grain hole / outer radius"),
    QT_TRANSLATE_NOOP("csr::fields", "physical model: geometric surface amplitude"),
};

// What a field of the input panels means (its tooltip). `section` empty: the same in every section.
struct FieldHelp
{
    std::u16string_view section;
    std::u16string_view key;
    const char *text;
};

constexpr FieldHelp kHelp[] = {
    // caliber / cartridge
    {u"caliber", u"CaliberName", QT_TRANSLATE_NOOP("csr::fieldhelp", "Name of the cartridge.")},
    {u"caliber", u"oal",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Maximum overall length of the loaded cartridge, from the case head to the bullet tip (C.I.P. L6).")},
    {u"caliber", u"caselen", QT_TRANSLATE_NOOP("csr::fieldhelp", "Length of the case from the head to the mouth (C.I.P. L3).")},
    {u"caliber", u"casevol",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Capacity of the empty case up to the mouth, best measured with water in a fired case of your own brass. One of the most "
         "important inputs: a smaller case gives a noticeably higher pressure.")},
    {u"caliber", u"Dz", QT_TRANSLATE_NOOP("csr::fieldhelp", "Groove diameter of the barrel (C.I.P. Z), usually the bullet diameter.")},
    {u"caliber", u"Aeff",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Cross-section of the bore the gas pushes on (C.I.P. Q), from the bore and groove diameters and the number and width of the "
         "grooves.")},
    {u"caliber", u"pMaxZul",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Permitted maximum pressure of the cartridge (C.I.P. Pmax or SAAMI MAP). The results warn when the simulated peak pressure "
         "is above it.")},
    {u"caliber", u"method",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "How the maximum pressure of the cartridge is measured (piezo C.I.P., SAAMI, copper crusher); for information.")},
    {u"caliber", u"sebert",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Share of the powder charge that is accelerated together with the bullet (the moving gas column). 0.5 is the usual value.")},
    {u"caliber", u"ps",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Pressure at which the bullet starts to move: neck tension, crimp and the engraving of the bullet into the rifling.")},
    {u"caliber", u"primer",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Primer size. Its ignition pressure, with the additional start pressure below, gives the start pressure of the load.")},
    {u"caliber", u"psadd",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Start pressure on top of the primer's: positive for a heavy crimp or a tight neck, negative for a light hold of the bullet. "
         "0 is the usual value.")},
    {u"caliber", u"shotshell",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Yes for a shotgun shell: the wad and the shot column are the projectile, and hull, wad and primer are chosen below.")},
    {u"caliber", u"shothull",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Hull of the shotshell. Hull, wad and primer change the usable capacity and the energy in the simulation.")},
    {u"caliber", u"shotwad",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Wad of the shotshell. Hull, wad and primer change the usable capacity and the energy in the simulation.")},
    {u"caliber", u"shotprimer",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Primer of the shotshell. Hull, wad and primer change the usable capacity and the energy in the simulation.")},
    // gun
    {u"gun", u"GunName", QT_TRANSLATE_NOOP("csr::fieldhelp", "Name of the gun, for your records.")},
    {u"gun", u"xe",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "Length of the barrel from the bolt face to the muzzle. A longer barrel gives more velocity.")},
    {u"gun", u"twistlen",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Barrel length per full turn of the rifling. Gives the spin of the bullet (revolutions per minute).")},
    {u"gun", u"mgun", QT_TRANSLATE_NOOP("csr::fieldhelp", "Weight of the gun, for the recoil energy and the recoil velocity.")},
    {u"gun", u"gjump",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Distance the bullet travels freely before it touches the rifling (jump to the lands). Usually 0; a longer jump lowers the "
         "peak pressure.")},
    // projectile
    {u"projectile", u"ProjectileName", QT_TRANSLATE_NOOP("csr::fieldhelp", "Name of the bullet.")},
    {u"projectile", u"mname", QT_TRANSLATE_NOOP("csr::fieldhelp", "Manufacturer of the bullet.")},
    {u"projectile", u"pname", QT_TRANSLATE_NOOP("csr::fieldhelp", "Product name of the bullet.")},
    {u"projectile", u"lotid", QT_TRANSLATE_NOOP("csr::fieldhelp", "Lot number of the bullets, for your records.")},
    {u"projectile", u"Dbul", QT_TRANSLATE_NOOP("csr::fieldhelp", "Diameter of the bullet.")},
    {u"projectile", u"mp", QT_TRANSLATE_NOOP("csr::fieldhelp", "Weight of the bullet. For a shotshell: shot and wad together.")},
    {u"projectile", u"glen",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Overall length of the bullet. With the seating depth it gives the space the bullet takes in the case. For a shotshell: the "
         "length of the wad and shot column.")},
    {u"projectile", u"gdepth",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "How deep the bullet sits in the case, from the case mouth to the bullet base. A deeper bullet leaves less room for the "
         "powder and raises the pressure.")},
    {u"projectile", u"gTailType",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Shape of the bullet base: flat base, hollow base or boat tail. Changes the room the base takes in the case.")},
    {u"projectile", u"gtailh", QT_TRANSLATE_NOOP("csr::fieldhelp", "Length of the boat tail, or depth of the hollow base.")},
    {u"projectile", u"gTailDiaA",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "Diameter where the boat tail or the cavity of the hollow base begins.")},
    {u"projectile", u"gTailDiaB", QT_TRANSLATE_NOOP("csr::fieldhelp", "Diameter at the base of the bullet (the end of the boat tail).")},
    {u"projectile", u"gfriction",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Extra resistance of the bullet in the barrel, as a pressure. Positive for bullets that are harder to push (e.g. solid "
         "copper), negative for easier ones. 0 is the usual value.")},
    {u"projectile", u"gmaterial",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Jacketed, or lead (cast or swaged): a lead bullet slides through the barrel with less friction.")},
    {u"projectile", u"gpressure",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Start pressure stored with the bullet; the simulation uses the start pressure of the cartridge.")},
    {u"projectile", u"g1bc",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "Ballistic coefficient for the G1 drag model (exterior ballistics; not used here).")},
    {u"projectile", u"g7bc",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "Ballistic coefficient for the G7 drag model (exterior ballistics; not used here).")},
    {u"projectile", u"gUBCS", QT_TRANSLATE_NOOP("csr::fieldhelp", "Code of the bullet's type and shape; for information.")},
    {u"projectile", u"mode", QT_TRANSLATE_NOOP("csr::fieldhelp", "Field of the original program's record; not used by the simulation.")},
    // propellant
    {u"propellant", u"mname", QT_TRANSLATE_NOOP("csr::fieldhelp", "Manufacturer of the powder.")},
    {u"propellant", u"pname", QT_TRANSLATE_NOOP("csr::fieldhelp", "Product name of the powder.")},
    {u"propellant", u"lotid", QT_TRANSLATE_NOOP("csr::fieldhelp", "Lot number of the powder, for your records.")},
    {u"propellant", u"mc", QT_TRANSLATE_NOOP("csr::fieldhelp", "Weight of the powder charge.")},
    {u"propellant", u"laddercnt",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Number of charges of the load development ladder. The charge above is the highest; every further step has one ladder step "
         "less powder.")},
    {u"propellant", u"laddermc", QT_TRANSLATE_NOOP("csr::fieldhelp", "Difference in powder weight between two steps of the ladder.")},
    {u"propellant", u"Ba",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Burn rate of the powder in the GRT model: higher is a faster powder. Fitted to measured loads; to match your own chronograph "
         "data use Fit to measurements on the Measurements tab.")},
    {u"propellant", u"a0",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Progressivity of the burn in the GRT model: how the burning surface of the grains grows or shrinks while they burn.")},
    {u"propellant", u"a1", QT_TRANSLATE_NOOP("csr::fieldhelp", "Second coefficient of the progressivity in the GRT model.")},
    {u"propellant", u"z1",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Burnt fraction of the powder at which the burn law of the GRT model passes into its middle part.")},
    {u"propellant", u"z2",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "Burnt fraction of the powder at which the burn law of the GRT model passes into its last part.")},
    {u"propellant", u"Qex",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Heat of explosion: the energy one kilogram of the powder releases. More energy gives more velocity and pressure.")},
    {u"propellant", u"k", QT_TRANSLATE_NOOP("csr::fieldhelp", "Ratio of the specific heats (cp/cv) of the powder gas, about 1.2 to 1.25.")},
    {u"propellant", u"eta",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Covolume of the powder gas: the volume the gas molecules themselves take. It matters at high pressure.")},
    {u"propellant", u"pc",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Density of the powder material itself (not of the loose powder): the room the unburnt grains take.")},
    {u"propellant", u"pcd",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "Bulk density of the loose powder: tells whether the charge fits into the case (load ratio).")},
    {u"propellant", u"pt",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "Temperature of the powder when fired. Warm powder burns faster: higher pressure and velocity.")},
    {u"propellant", u"tcc",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Change of the burn rate per degree below the reference temperature. 0 for both coefficients: the program's usual "
                       "temperature behaviour.")},
    {u"propellant", u"tch",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Change of the burn rate per degree above the reference temperature. 0 for both coefficients: the program's usual "
                       "temperature behaviour.")},
    {u"propellant", u"po",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "Air pressure of the original program's files; not used by the simulation.")},
    {u"propellant", u"imageUuid", QT_TRANSLATE_NOOP("csr::fieldhelp", "Reference to the photo of the powder in the database.")},
    {u"propellant", u"imageDetailUuid",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "Reference to the close-up photo of the powder in the database.")},
    {u"propellant", u"physBa",
     QT_TRANSLATE_NOOP(
         "csr::fieldhelp",
         "Burn rate of this powder in the Physical Model, fitted to published load data. It applies while the GRT values (Ba, a0, z1, "
         "z2) are unchanged.")},
    {u"propellant", u"physA0",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "Progressivity of the burn in the Physical Model, fitted to published load data.")},
    {u"propellant", u"physZ1",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "Burnt fraction at which the burn law of the Physical Model passes into its middle part.")},
    {u"propellant", u"physZ2",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "Burnt fraction at which the burn law of the Physical Model passes into its last part.")},
    {u"propellant", u"physGrain",
     QT_TRANSLATE_NOOP("csr::fieldhelp",
                       "Grain shape in the Physical Model: ball, cylinder (extruded) or fitted (the burn law of the fit).")},
    {u"propellant", u"physGrainLd",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "Length of the grains divided by their diameter (Physical Model).")},
    {u"propellant", u"physGrainHole",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "Radius of the hole of perforated grains divided by their outer radius (Physical Model).")},
    {u"propellant", u"physGrainAmp",
     QT_TRANSLATE_NOOP("csr::fieldhelp", "How strongly the burning surface follows the grain geometry (Physical Model); 1: exactly.")},
};

}  // namespace

QString fieldHelp(QStringView section, QStringView key)
{
    const auto it = std::ranges::find_if(kHelp, [&](const FieldHelp &h)
                                         { return QStringView(h.key) == key && (h.section.empty() || QStringView(h.section) == section); });
    return it == std::end(kHelp) ? QString() : QCoreApplication::translate("csr::fieldhelp", it->text);
}

QString translatedDescription(const QString &descr)
{
    if (descr.isEmpty()) return descr;
    return QCoreApplication::translate("csr::fields", descr.toUtf8().constData());
}

}  // namespace csr
