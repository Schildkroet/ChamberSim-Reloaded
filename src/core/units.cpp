#include "units.h"

#include "numeric.h"

#include <QCoreApplication>
#include <QLocale>

#include <array>

namespace csr::units
{

namespace
{

constexpr double kGrain = kGrainToGram;
constexpr double kInch = 25.4;
constexpr double kLbf = 4.4482216152605;
constexpr double kFtLbf = 1.3558179483314004;

constexpr Unit kLength[] = {{"mm", "mm", 1.0, 0, 2}, {"cm", "cm", 10.0, 0, 3}, {"in", "in", kInch, 0, 3}};
constexpr Unit kBulletMass[] = {{"g", "g", 1.0, 0, 3}, {"grain", "grain", kGrain, 0, 2}, {"oz", "oz", 28.349523125, 0, 4}};
constexpr Unit kGunMass[] = {{"kg", "kg", 1.0, 0, 2}, {"lb", "lb", 0.45359237, 0, 2}, {"g", "g", 0.001, 0, 0}};
constexpr Unit kPressure[] = {{"bar", "bar", 1.0, 0, 0},
                              {"psi", "psi", 0.0689475729316836, 0, 0},
                              {"mpa", "MPa", 10.0, 0, 1},
                              {"kpsi", "kpsi", 68.9475729316836, 0, 2}};
constexpr Unit kVelocity[] = {{"ms", "m/s", 1.0, 0, 1}, {"fps", "ft/s", 0.3048, 0, 0}, {"kmh", "km/h", 1.0 / 3.6, 0, 1}};
constexpr Unit kArea[] = {{"mm2", "mm²", 1.0, 0, 3}, {"cm2", "cm²", 100.0, 0, 4}, {"in2", "in²", 645.16, 0, 5}};
constexpr Unit kVolume[] = {
    {"cm3", "cm³", 1.0, 0, 3}, {"grh2o", "grain H₂O", kGrain, 0, 2}, {"in3", "in³", 16.387064, 0, 4}, {"mm3", "mm³", 0.001, 0, 0}};
constexpr Unit kDensity[] = {{"kgm3", "kg/m³", 1.0, 0, 0}, {"gcm3", "g/cm³", 1000.0, 0, 3}, {"lbft3", "lb/ft³", 16.01846337396, 0, 1}};
// Fahrenheit: base(Cel) = (F - 32) / 1.8  ->  factor 1/1.8, offset -32/1.8 + ... handled by offset/factor below
constexpr Unit kTemperature[] = {{"cel", "°C", 1.0, 0.0, 0}, {"fahr", "°F", 1.0 / 1.8, -32.0 / 1.8, 0}};
constexpr Unit kEnergy[] = {{"j", "J", 1.0, 0, 0}, {"ftlbf", "ft·lbf", kFtLbf, 0, 0}, {"kj", "kJ", 1000.0, 0, 3}};
constexpr Unit kImpulse[] = {{"ns", "Ns", 1.0, 0, 2}, {"lbfs", "lbf·s", kLbf, 0, 3}};
constexpr Unit kTime[] = {{"ms", "ms", 1.0, 0, 4}, {"us", "µs", 0.001, 0, 1}, {"s", "s", 1000.0, 0, 6}};
constexpr Unit kForce[] = {{"n", "N", 1.0, 0, 0}, {"lbf", "lbf", kLbf, 0, 1}, {"kn", "kN", 1000.0, 0, 3}};

}  // namespace

std::span<const Unit> unitsOf(Quantity q)
{
    switch (q)
    {
    case Quantity::Length: return kLength;
    case Quantity::BulletMass: return kBulletMass;
    case Quantity::GunMass: return kGunMass;
    case Quantity::Pressure: return kPressure;
    case Quantity::Velocity: return kVelocity;
    case Quantity::Area: return kArea;
    case Quantity::Volume: return kVolume;
    case Quantity::Density: return kDensity;
    case Quantity::Temperature: return kTemperature;
    case Quantity::Energy: return kEnergy;
    case Quantity::Impulse: return kImpulse;
    case Quantity::Force: return kForce;
    case Quantity::Time: return kTime;
    case Quantity::None: break;
    }
    return {};
}

const Unit &systemUnit(Quantity q, System s)
{
    const auto list = unitsOf(q);
    const auto pick = [&](const char *id) -> const Unit &
    {
        const Unit *u = findUnit(q, QLatin1String(id));
        return u ? *u : list.front();
    };
    switch (s)
    {
    case System::Metric: return list.front();
    case System::Imperial:
        switch (q)
        {
        case Quantity::Length: return pick("in");
        case Quantity::BulletMass: return pick("grain");
        case Quantity::GunMass: return pick("lb");
        case Quantity::Pressure: return pick("psi");
        case Quantity::Velocity: return pick("fps");
        case Quantity::Area: return pick("in2");
        case Quantity::Volume: return pick("grh2o");  // case capacity is given in grains of water
        case Quantity::Density: return pick("lbft3");
        case Quantity::Temperature: return pick("fahr");
        case Quantity::Energy: return pick("ftlbf");
        case Quantity::Impulse: return pick("lbfs");
        case Quantity::Force: return pick("lbf");
        case Quantity::Time: return list.front();
        case Quantity::None: break;
        }
        break;
    case System::Original:  // what the original program shows by default: mm, grains, bar, m/s, grain H2O
        if (q == Quantity::BulletMass) return pick("grain");
        if (q == Quantity::Volume) return pick("grh2o");
        break;
    }
    return list.front();
}

const Unit *findUnit(Quantity q, const QString &id)
{
    for (const Unit &u : unitsOf(q))
        if (id == QLatin1String(u.id)) return &u;
    return nullptr;
}

Quantity quantityOfFileUnit(const QString &unit)
{
    static const std::array<std::pair<const char *, Quantity>, 15> map{{{"mm", Quantity::Length},
                                                                        {"g", Quantity::BulletMass},
                                                                        {"kg", Quantity::GunMass},
                                                                        {"bar", Quantity::Pressure},
                                                                        {"m/s", Quantity::Velocity},
                                                                        {"mm2", Quantity::Area},
                                                                        {"cm3", Quantity::Volume},
                                                                        {"kg/m3", Quantity::Density},
                                                                        {"Cel", Quantity::Temperature},
                                                                        {"J", Quantity::Energy},
                                                                        {"joule", Quantity::Energy},
                                                                        {"Ns", Quantity::Impulse},
                                                                        {"N", Quantity::Force},
                                                                        {"m", Quantity::Length},
                                                                        {"bar ", Quantity::Pressure}}};
    for (const auto &[name, q] : map)
        if (unit == QLatin1String(name)) return q;
    return Quantity::None;
}

double toDisplay(double base, const Unit &u) noexcept
{
    return (base - u.offset) / u.factor;
}

double fromDisplay(double shown, const Unit &u) noexcept
{
    return shown * u.factor + u.offset;
}

QString format(double base, const Unit &u, int decimals)
{
    return QLocale().toString(toDisplay(base, u), 'f', decimals >= 0 ? decimals : u.decimals);
}

std::optional<double> parseToBase(const QString &text, const Unit &u)
{
    const auto v = parseFiniteLocale(text);
    if (!v) return std::nullopt;
    return fromDisplay(*v, u);
}

QString systemName(System s)
{
    switch (s)
    {
    case System::Metric: return QCoreApplication::translate("csr::units", "Metric (mm, g, bar, m/s)");
    case System::Imperial: return QCoreApplication::translate("csr::units", "Imperial (in, grain, psi, ft/s)");
    case System::Original: return QCoreApplication::translate("csr::units", "Original (mm, grain, bar, m/s)");
    }
    return {};
}

}  // namespace csr::units
