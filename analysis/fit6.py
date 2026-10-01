import json, numpy as np
d = json.load(open("../reference/results/exp_base.json")); P = d["points"]
x, z, p, v, e, t = (np.array([q[k] for q in P]) for k in "xzpvet")
x_, p_ = x * 1e-3, p * 1e5
A, mc, mp, rho = 51.78e-6, 2.721554219989671e-3, 12.700586359951799e-3, 1580.0
k, Qex, seb = 1.2214, 3790e3, 0.5
mprime = mp + seb * mc; th = k - 1; f = th * Qex
dx = x_ - x_[0]
Ffr = 0.7685 * 1e3                      # N, from MuzzleEfriction scaling (J/mm -> N)
# model: p*(V0 + A dx - mc(1-z)/rho - b mc z) = f mc z - th*(0.5 m' v^2 + s*Ffr*dx)   unknown V0, b, s
y = p_ * (A * dx - mc * (1 - z) / rho) - f * mc * z + th * 0.5 * mprime * v**2
M = np.column_stack([-p_, p_ * z, -th * Ffr * dx])      # coeffs: V0, b*mc, s   (y = -V0 p + b mc p z - th s Ffr dx)
sol, *_ = np.linalg.lstsq(M, y, rcond=None)
r = M @ sol - y
print("V0 = %.4f cm3   b = %.4f cm3/g   s = %.4f   resid = %.2e" % (sol[0]*1e6, sol[1]/mc*1e3, sol[2], np.max(np.abs(r))/np.max(np.abs(y))))
# also: friction in the energy only at coefficient 1 (physical), fit V0,b
M2 = M[:, :2]; y2 = y + (-th * Ffr * dx) * 0  # compare s fixed=1
y2 = y - (-th * Ffr * dx)
sol2, *_ = np.linalg.lstsq(M2, y2, rcond=None)
print("s=1: V0 = %.4f cm3 b = %.4f cm3/g resid %.2e" % (sol2[0]*1e6, sol2[1]/mc*1e3, np.max(np.abs(M2@sol2-y2))/np.max(np.abs(y))))
