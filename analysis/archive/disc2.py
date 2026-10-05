import json, numpy as np
d = json.load(open("../../reference/results/exp_base.json")); P = d["points"]
x, z, p, v, e, t = (np.array([q[k] for q in P]) for k in "xzpvet")
dx = (x[1] - x[0]) * 1e-3
dt_tab = np.diff(t) * 1e-3
cands = {
 "dx/v_end":   dx / v[1:],
 "dx/v_start": dx / np.where(v[:-1] > 0, v[:-1], np.nan),
 "2dx/(v0+v1)": 2 * dx / (v[:-1] + v[1:]),
 "dx/sqrt(v0v1)": dx / np.sqrt(v[:-1] * v[1:]),
}
print(" i   dt_table[us]  " + "  ".join("%-14s" % k for k in cands))
for i in (0, 1, 2, 3, 4, 5, 6, 10, 20, 50, 100, 500, 1500):
    print("%4d  %12.5f  " % (i, dt_tab[i] * 1e6) + "  ".join("%14.5f" % (c[i] * 1e6) for c in cands.values()))
print("t[0..5] (ms):", t[:6])
# ratio table/trapz pattern
r = dt_tab / cands["2dx/(v0+v1)"]
print("table/trapz  min %.5f max %.5f mean %.5f" % (np.nanmin(r[3:]), np.nanmax(r[3:]), np.nanmean(r[3:])))
print("t_end table %.6f ms ;  sum trapz %.6f ms ; sum dx/v_end %.6f" % (t[-1], np.nansum(cands["2dx/(v0+v1)"][1:])*1e3 , np.nansum(cands["dx/v_end"])*1e3))
