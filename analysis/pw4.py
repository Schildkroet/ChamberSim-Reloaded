"""Is phi(z) a universal shape per phase once rescaled by (a0, z1, z2)?"""
import json, glob, numpy as np
R = "../reference/results/"; meta = json.load(open("../reference/powders.json"))
P1, P2, P3 = [], [], []      # (feature..., value)
per = {}
for f in sorted(glob.glob(R + "exp_pw_*.json")):
    pid = f.split("_")[-1][:-5]; d = json.load(open(f)); m = meta[pid]
    if not d["points"] or d.get("stale"): continue
    Pt = d["points"]; x, z, p, v = (np.array([q[k] for q in Pt]) for k in "xzpv")
    g = v * np.gradient(z, x * 1e-3) / np.maximum(p, 1) / m["Ba"]
    z1, z2, a0 = m["z1"], m["z2"], m["a0"]
    ok = (p > 200) & (np.arange(len(z)) > 8)
    # phase 1 linear fit -> phi1 at z1
    s1 = ok & (z > 0.08) & (z < 0.9 * z1)
    if s1.sum() < 10: continue
    B, A = np.polyfit(z[s1], g[s1], 1); phi1 = A + B * z1
    per[pid] = dict(A=A, B=B, phi1=phi1, a0=a0, z1=z1, z2=z2)
    s2 = ok & (z > z1 + 0.02) & (z < z2 - 0.01)
    for zz, gg in zip(z[s2], g[s2]): P2.append(((zz - z1) / (z2 - z1), gg / phi1, pid))
    s3 = ok & (z > z2 + 0.01) & (z < 0.97)
    for zz, gg in zip(z[s3], g[s3]): P3.append(((zz - z2) / (1 - z2), gg / phi1, pid))
print("powders used:", len(per))
a = np.array([[v["a0"], v["B"], v["A"], v["phi1"], v["z1"], v["z2"]] for v in per.values()])
print("B vs a0: B = %.4f*a0 + %.4f   (A mean %.3f)" % (*np.polyfit(a[:, 0], a[:, 1], 1), a[:, 2].mean()))
s2 = np.array([[q[0], q[1]] for q in P2]); s3 = np.array([[q[0], q[1]] for q in P3])
for name, S, deg in (("phase 2: phi/phi1 vs s", s2, 3), ("phase 3: phi/phi1 vs t", s3, 3)):
    c = np.polyfit(S[:, 0], S[:, 1], deg); res = S[:, 1] - np.polyval(c, S[:, 0])
    print("%s  poly coeffs (high->low): %s   rms %.4f  (n=%d)" % (name, np.round(c, 4), res.std(), len(S)))
    for q in (0.0, 0.1, 0.25, 0.5, 0.75, 0.9, 1.0): print("     s=%.2f -> %.4f" % (q, np.polyval(c, q)))
