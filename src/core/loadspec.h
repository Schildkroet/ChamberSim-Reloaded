// Generated from a sample load of the original program: the input fields of a load file with their
// units, types and descriptions, in file order. Used to create new loads in the same format.
#pragma once

namespace grt::spec
{

struct Field
{
    const char *name;
    const char *unit;
    const char *type;
    const char *descr;
};

inline constexpr Field kCaliber[] = {
    {"CaliberName", "", "string", "caliber name"},
    {"oal", "mm", "decimal", "cartridge overall length"},
    {"caselen", "mm", "decimal", "case length"},
    {"casevol", "cm3", "decimal", "case volume"},
    {"Dz", "mm", "decimal", "groove caliber"},
    {"Aeff", "mm2", "decimal", "effektive area"},
    {"pMaxZul", "bar", "decimal", "maximum allowed pressure"},
    {"method", "", "string", "cartridge test method"},
    {"sebert", "", "decimal", "Sebert factor"},
    {"ps", "bar", "decimal", "initial gas pressure"},
    {"shotshell", "", "boolean", "true if shotshell"},
};

inline constexpr Field kGun[] = {
    {"GunName", "", "string", "gun name"},
    {"xe", "mm", "decimal", "barrel length"},
    {"twistlen", "mm", "decimal", "twist length"},
    {"mgun", "kg", "decimal", "gun weight"},
    {"gjump", "mm", "decimal", "bullet jump distance"},
    {"gjumpUnlock", "", "boolean", "unlocked bullet jump"},
};

inline constexpr Field kProjectile[] = {
    {"ProjectileName", "", "string", "projectile name"},
    {"mname", "", "string", "projectile manufacturer"},
    {"pname", "", "string", "projectile product name"},
    {"lotid", "", "string", "projectile lot-id"},
    {"Dbul", "mm", "decimal", "projectile diameter"},
    {"mp", "g", "decimal", "projectile weight"},
    {"gdepth", "mm", "decimal", "bullet seating depth"},
    {"glen", "mm", "decimal", "projectile overall length"},
    {"gpressure", "bar", "decimal", "initial gas pressure"},
    {"gtailh", "mm", "decimal", "projectile tail length"},
    {"gTailDiaA", "mm", "decimal", "projectile tail diameter A"},
    {"gTailDiaB", "mm", "decimal", "projectile tail diameter B"},
    {"gTailType", "", "decimal", "projectile base type"},
    {"g1bc", "", "decimal", "G1 ballistic coefficient"},
    {"g7bc", "", "decimal", "G7 ballistic coefficient"},
    {"gUBCS", "", "string", "UBCS-Code (Universal Bullet Classification Scheme)"},
    {"gfriction", "bar", "decimal", "special bullet resistance inside the barrel"},
    {"gfrictionUnlock", "", "boolean", "unlocked friction"},
    {"mode", "", "string", ""},
};

inline constexpr Field kPropellant[] = {
    {"mname", "", "string", "propellant manufacturer"},
    {"pname", "", "string", "propellant product name"},
    {"lotid", "", "string", "propellant lot-id"},
    {"Ba", "", "decimal", "vivacity coefficient"},
    {"Qex", "kJ/kg", "decimal", "specific explosive heat"},
    {"k", "", "decimal", "ratio of the specific heat of the powder gases"},
    {"a0", "", "decimal", "prog/deg coefficient"},
    {"a1", "", "decimal", "prog/deg coefficient"},
    {"z1", "", "decimal", "burn-up limit z1"},
    {"z2", "", "decimal", "burn-up limit z2"},
    {"eta", "cm3/kg", "decimal", "covolume of the powder gases"},
    {"pc", "kg/m3", "decimal", "material density"},
    {"pcd", "kg/m3", "decimal", "bulk density"},
    {"pt", "Cel", "decimal", "temperature normal"},
    {"tcc", "", "decimal", "cold temperature coefficient"},
    {"tch", "", "decimal", "hot temperature coefficient"},
    {"mc", "g", "decimal", "propellant mass"},
    {"laddercnt", "", "decimal", "ladder steps (1 to 10, below 2: disabled)"},
    {"laddermc", "g", "decimal", "ladder step propellant mass"},
    {"imageUuid", "", "string", "propellant picture uuid"},
    {"imageDetailUuid", "", "string", "propellant detail picture uuid"},
};

}  // namespace grt::spec
