import json, glob, numpy as np
R = "../../reference/results/"; meta = json.load(open("../../reference/powders.json"))
rows = []
for f in sorted(glob.glob(R + "exp_pw_*.json"), key=lambda x: int(x.split("_")[-1][:-5])):
    pid = f.split("_")[-1][:-5]; d = json.load(open(f))
    if not d["points"] or d.get("stale"): continue
    m = meta[pid]; P = d["points"]
    x, z, p, v = (np.array([q[k] for q in P]) for k in "xzpv")
    dzdt = v * np.gradient(z, x * 1e-3); g = dzdt / np.maximum(p, 1.0) / m["Ba"]
    zi = np.maximum.accumulate(z)
    def G(zq, g=g, zi=zi, z=z):
        i = np.searchsorted(zi, zq)
        return g[i] if 5 < i < len(z) - 3 else np.nan
    rows.append((pid, m, G))
print(" pid  Ba    a0     z1    z2  |  g/Ba at: 0.05   0.10   z1/2   z1-.02  z1+.02  mid   z2-.02  z2+.02  0.95")
for pid, m, G in rows:
    z1, z2 = m["z1"], m["z2"]
    vals = [G(q) for q in (0.05, 0.10, z1/2, z1-0.02, z1+0.02, (z1+z2)/2, z2-0.02, z2+0.02, 0.95)]
    print("%4s %5.2f %6.2f %.3f %.3f |  " % (pid, m["Ba"], m["a0"], z1, z2) + " ".join("%6.3f" % q for q in vals))
