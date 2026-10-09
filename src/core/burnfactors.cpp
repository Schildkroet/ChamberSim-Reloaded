#include "burnfactors.h"

#include <array>
#include <cmath>

namespace csr
{

namespace
{
// monomials of a0, z1, z2 up to degree 3 in the order 1; a0 z1 z2; a0a0 a0z1 a0z2 z1z1 z1z2 z2z2; (the ten cubic terms, same scheme)
constexpr std::array<double, 20> kShape{1.56077,    0.123187, -0.509118,  -2.00729,    -0.00771699, 0.428304,   -0.281728,
                                        -0.542881,  1.54258,  2.34569,    0.000267063, -0.0146042,  0.00620305, -0.159374,
                                        -0.0548813, 0.167131, -0.0660242, 0.843872,    -1.20759,    -0.883327};
}  // namespace

double progressivityShape(double a0, double z1, double z2)
{
    const std::array v{a0, z1, z2};
    double sum = kShape[0];
    std::size_t n = 1;
    for (std::size_t i = 0; i < 3; ++i) sum += kShape[n++] * v[i];
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = i; j < 3; ++j) sum += kShape[n++] * v[i] * v[j];
    for (std::size_t i = 0; i < 3; ++i)
        for (std::size_t j = i; j < 3; ++j)
            for (std::size_t l = j; l < 3; ++l) sum += kShape[n++] * v[i] * v[j] * v[l];
    return sum;
}

BurnFactors burnFactors(double Ba, double Qex_kJkg, double k, double a0, double z1, double z2)
{
    if (!(Ba > 0.0) || !(Qex_kJkg > 0.0) || !(k > 1.0)) return {};
    const double Br = std::sqrt(2.0 * (k - 1.0)) * Ba * Qex_kJkg / 10000.0;
    const double Bp = Br * progressivityShape(a0, z1, z2);
    return {.Br = Br, .Bp = Bp, .Brp = std::sqrt((Br * Br + Bp * Bp) / 2.0)};
}

BurnFactors burnFactors(const QVariantMap &p)
{
    const auto num = [&p](const char *key) { return p.value(QLatin1String(key)).toDouble(); };
    return burnFactors(num("Ba"), num("Qex"), num("k"), num("a0"), num("z1"), num("z2"));
}

void fillBurnFactors(QVariantMap &p)
{
    const BurnFactors f = burnFactors(p);
    p.insert(QStringLiteral("Br"), f.Br);
    p.insert(QStringLiteral("Bp"), f.Bp);
    p.insert(QStringLiteral("Brp"), f.Brp);
}

}  // namespace csr
