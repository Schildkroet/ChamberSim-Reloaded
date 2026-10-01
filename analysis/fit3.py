import json, numpy as np, itertools
d = json.load(open("../reference/results/8x57is.json")); P = d["points"]
x, z, p, v, e, t = (np.array([q[k] for q in P]) for k in "xzpvet")
x_, p_ = x * 1e-3, p * 1e5
A, mc, mp, rho = 51.78e-6, 2.721554219989671e-3, 12.700586359951799e-3, 1580.0
k, Qex, seb = 1.2214, 3790e3, 0.5
mprime = mp + seb * mc
th = k - 1
x0 = x_[0]
y = p_ * (A * (x_ - x0) - mc * (1 - z) / rho)       # target LHS part
cols = {  # name: (column, meaning of coefficient)
 "z":      z,            # f*mc
 "v2":    -v**2,         # c
 "pz":     p_ * z,       # b*mc
 "p":     -p_,           # V0
 "dx":     (x_ - x0),    # friction-ish work  (N)
 "dx*z":   (x_ - x0)*z,
 "1":      np.ones_like(z),
}
def fit(names):
    M = np.column_stack([cols[n] for n in names])
    s, *_ = np.linalg.lstsq(M, y, rcond=None)
    r = M @ s - y
    return s, np.max(np.abs(r)) / np.max(np.abs(y))
for names in (["z","v2","pz","p"], ["z","v2","pz","p","dx"], ["z","v2","pz","p","dx","1"], ["z","v2","pz","p","dx","dx*z"]):
    s, r = fit(names)
    print(names, "resid %.2e" % r)
    print("    ", dict(zip(names, ["%.5g" % q for q in s])))
print("expected: f*mc=", (k-1)*Qex*mc, " c=", th/2*mprime)
