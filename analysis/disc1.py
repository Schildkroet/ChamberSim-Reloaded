import json, numpy as np
d = json.load(open("../reference/results/exp_base.json")); P = d["points"]
x, z, p, v, e, t = (np.array([q[k] for q in P]) for k in "xzpvet")
A, mc, mp = 51.78e-6, 2.721554219989671e-3, 12.700586359951799e-3
m_ = mp + 0.5 * mc
dx = (x[1] - x[0]) * 1e-3
dK = 0.5 * m_ * (v[1:]**2 - v[:-1]**2)
W_new = A * dx * p[1:] * 1e5                 # pressure at end of step
W_old = A * dx * p[:-1] * 1e5                # pressure at start of step
W_avg = 0.5 * (W_new + W_old)
for nm, W in (("p_end", W_new), ("p_start", W_old), ("p_avg", W_avg)):
    r = dK[1:1900] / W[1:1900]
    print("%-8s dK/W: mean %.8f  min %.8f max %.8f" % (nm, r.mean(), r.min(), r.max()))
r = dK / W_new
for i in (0, 1, 2, 3, 5, 10, 50, 100, 190, 500, 1000, 1998):
    print(i, "dK/W_end = %.8f   friction-ish (W_end-dK)/A/dx = %.4f bar" % (r[i], (W_new[i]-dK[i])/(A*dx)/1e5))
