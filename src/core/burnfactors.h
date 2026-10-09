#pragma once

#include <QVariantMap>

namespace csr
{

// The burn-rate factors of the original program (burn rate table, sorting by progressivity / brisance):
//   Br  = sqrt(2 (k - 1) (Ba Qex)^2) / 10000                      brisance (initial burst)
//   Bp  = sqrt(2 (k - 1) (Ba phi(z1) Qex)^2) / 10000 = Br phi(z1)  progressivity
//   Brp = sqrt((Br^2 + Bp^2) / 2)                                  both combined
// They are derived from the model values and are not used by the solver.
struct BurnFactors
{
    double Br{}, Bp{}, Brp{};
};

// phi(z1) of the original's progressivity factor. Its definition is not published; this is a cubic polynomial in a0, z1, z2 fitted to the
// 638 distinct models of the original's database (5-fold cross-validation: 0.5 % rms, at most 4 %).
[[nodiscard]] double progressivityShape(double a0, double z1, double z2);

[[nodiscard]] BurnFactors burnFactors(double Ba, double Qex_kJkg, double k, double a0, double z1, double z2);

// The same for a propellant record (columns Ba, Qex, k, a0, z1, z2); all zero if Ba, Qex or k is missing.
[[nodiscard]] BurnFactors burnFactors(const QVariantMap &propellant);

// Sets Br, Bp and Brp of a propellant record from its model values.
void fillBurnFactors(QVariantMap &propellant);

}  // namespace csr
