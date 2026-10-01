import json, numpy as np, glob, os
R = "../reference/results/"
runs = ["exp_base", "exp_mc_x0.6", "exp_mc_x0.8", "exp_mc_x1.2", "exp_mp_x0.5", "exp_mp_x2.0", "exp_mp_x4.0", "exp_sebert_0.0", "exp_sebert_1.0", "exp_ps_50", "exp_ps_500", "exp_cv_mp200_mc1.0"]
zq = [0.02, 0.05, 0.1, 0.2, 0.3, 0.4, 0.45, 0.5, 0.55, 0.6, 0.7, 0.75, 0.8, 0.85]
def g_of_z(name):
    P = json.load(open(R + name + ".json"))["points"]
    x, z, p, v = (np.array([q[k] for q in P]) for k in "xzpv")
    dzdt = v * np.gradient(z, x * 1e-3); g = dzdt / p         # 1/(s*bar)
    # strictly increasing part of z only
    zi = np.maximum.accumulate(z); out = {}
    for q in zq:
        if zi[-1] > q:
            i = np.searchsorted(zi, q); out[q] = g[i]
    return out
print("g(z) [1/(s bar)]   rows=z   cols=runs")
print("   z    " + " ".join("%9s" % n[4:14] for n in runs))
G = {n: g_of_z(n) for n in runs}
for q in zq:
    print("%5.2f  " % q + " ".join(("%9.4f" % G[n][q]) if q in G[n] else "%9s" % "-" for n in runs))
