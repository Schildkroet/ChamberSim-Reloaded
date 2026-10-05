import json, numpy as np
d = json.load(open("../../reference/results/8x57is.json")); P = d["points"]
x, z, p, v, e, t = (np.array([q[k] for q in P]) for k in "xzpvet")
x_, p_ = x * 1e-3, p * 1e5
A, mc, mp, rho = 51.78e-6, 2.721554219989671e-3, 12.700586359951799e-3, 1580.0
k, Qex, seb = 1.2214, 3790e3, 0.5
mprime = mp + seb * mc; th = k - 1; f = th * Qex
x0 = x_[0]
# "Needed" gas volume if the textbook model held with no losses:
Vneed = (f * mc * z - th / 2 * mprime * v**2) / p_          # = V0 + A dx - mc(1-z)/rho - b mc z
geo   = A * (x_ - x0) - mc * (1 - z) / rho
print(" i     x     z      p     v   Vneed[cm3]   geo[cm3]   Vneed-geo[cm3]   (Vneed-geo)/(mc z)[cm3/g]")
for i in (1, 2, 5, 10, 20, 50, 100, 150, 200, 300, 500, 800, 1200, 1600, 1999):
    print("%4d %6.1f %.3f %6.0f %5.0f %9.3f %10.3f %12.3f %12.3f" % (i, x[i], z[i], p[i], v[i], Vneed[i]*1e6, geo[i]*1e6, (Vneed[i]-geo[i])*1e6, (Vneed[i]-geo[i])/(mc*z[i])*1e6/1e3))
