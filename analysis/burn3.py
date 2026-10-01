import json, re, numpy as np
R = "../reference/results/"; J = "../tools/GRTRef/jobs_done/"
def prm(path):
    s = open(path, encoding="utf-8").read()
    blk = re.search(r"<propellant>.*?</propellant>", s, re.S).group(0)
    g = lambda k: float(re.search(rf'<input name="{k}" value="([^"]*)"', blk).group(1))
    nm = re.search(r'name="pname" value="([^"]*)"', blk).group(1)
    return nm, {k: g(k) for k in ("Ba", "a0", "a1", "z1", "z2", "k", "Qex", "pc", "pcd", "eta")}
for job in ("8x57is", "8x57is N130", "45-70-Gov"):
    nm, q = prm(J + job + ".grtload")
    P = json.load(open(R + job + ".json"))["points"]
    x, z, p, v = (np.array([r[k] for r in P]) for k in "xzpv")
    g = v * np.gradient(z, x * 1e-3) / p / q["Ba"]
    print("== %s  powder %s  %s" % (job, nm, {k: q[k] for k in ("Ba", "a0", "a1", "z1", "z2")}))
    for zq in (0.03, 0.1, 0.2, 0.3, 0.4, q["z1"] - 0.02, q["z1"] + 0.02, (q["z1"] + q["z2"]) / 2, q["z2"] - 0.02, q["z2"] + 0.02, 0.9):
        i = np.searchsorted(np.maximum.accumulate(z), zq)
        if i < len(z) - 1: print("   z=%.3f  g/Ba=%.4f" % (zq, g[i]))
