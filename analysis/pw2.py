import json, glob, numpy as np
R = "../reference/results/"; meta = json.load(open("../reference/powders.json"))
out = []
for f in sorted(glob.glob(R + "exp_pw_*.json"), key=lambda x: int(x.split("_")[-1][:-5])):
    pid = f.split("_")[-1][:-5]; d = json.load(open(f)); m = meta[pid]
    if not d["points"] or d.get("stale"): continue
    P = d["points"]; x, z, p, v = (np.array([q[k] for q in P]) for k in "xzpv")
    g = v * np.gradient(z, x * 1e-3) / np.maximum(p, 1) / m["Ba"]
    # phase 1 window: z from 0.06 to 0.9*z1, only where pressure is meaningful
    sel = (z > 0.06) & (z < 0.9 * m["z1"]) & (p > 300)
    if sel.sum() < 12: continue
    B, A = np.polyfit(z[sel], g[sel], 1)
    C = np.polyfit(z[sel], g[sel], 2)
    res = np.std(g[sel] - np.polyval([B, A], z[sel]))
    out.append((m["a0"], A, B, res, m["z1"], pid, C))
out.sort()
print(" a0      A(intercept)  B(slope)   rms    z1     B/a0    A-1")
for a0, A, B, res, z1, pid, C in out:
    print("%6.2f   %8.3f   %8.3f  %.3f  %.3f  %7.3f  %7.3f   (pid %s)" % (a0, A, B, res, z1, B / a0 if abs(a0) > 0.2 else float('nan'), A - 1, pid))
a = np.array([o[0] for o in out]); A = np.array([o[1] for o in out]); B = np.array([o[2] for o in out])
ok = (a > 0.1) & (a < 8)
print("fit B = k1*a0 + k0 :", np.polyfit(a[ok], B[ok], 1), "   A = c1*a0 + c0 :", np.polyfit(a[ok], A[ok], 1))
