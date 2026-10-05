"""Exploratory: does the reference data obey a Noble-Abel energy balance?
   p*(V0 + A*(x-x0) - mc*(1-z)/rho - b*mc*z) = f*mc*z - c*v^2   (c = theta*m'/2)"""
import json, numpy as np
R = "../../reference/results/"
d = json.load(open(R + "8x57is.json")); P = d["points"]
x, z, p, v, e, t = (np.array([q[k] for q in P]) for k in "xzpvet")
x, p = x * 1e-3, p * 1e5                      # m, Pa
A, mc, mp, rho = 51.78e-6, 2.721554219989671e-3, 12.700586359951799e-3, 1580.0
k, Qex = 1.2214, 3790e3
x0 = x[0]
# unknowns: f', c, b', V0   :  p*(A(x-x0) - mc(1-z)/rho) = f' z - c v^2 + b' p z - V0 p
M = np.column_stack([z, -v**2, p * z, -p])
y = p * (A * (x - x0) - mc * (1 - z) / rho)
sol, res, *_ = np.linalg.lstsq(M, y, rcond=None)
fp, c, bp, V0 = sol
pred = M @ sol
print("f' =", fp, " -> f =", fp / mc, " (k-1)*Qex =", (k - 1) * Qex)
print("c  =", c, " -> m'*theta/2 ; theta=k-1 -> m' =", c * 2 / (k - 1), " mp =", mp, " (m'-mp)/mc =", (c * 2 / (k - 1) - mp) / mc)
print("b' =", bp, " -> b =", bp / mc, "m3/kg")
print("V0 =", V0 * 1e6, "cm3  (casevol 4.110 cm3)")
print("max rel residual:", np.max(np.abs(pred - y)) / np.max(np.abs(y)))
