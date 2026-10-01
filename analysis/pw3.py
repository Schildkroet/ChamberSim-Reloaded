import json, numpy as np, sys
R = "../reference/results/"; meta = json.load(open("../reference/powders.json"))
def prof(pid):
    d = json.load(open(R + f"exp_pw_{pid}.json")); m = meta[pid]; P = d["points"]
    x, z, p, v = (np.array([q[k] for q in P]) for k in "xzpv")
    g = v * np.gradient(z, x * 1e-3) / np.maximum(p, 1) / m["Ba"]
    return m, z, p, g
for pid in sys.argv[1:]:
    m, z, p, g = prof(pid); zi = np.maximum.accumulate(z)
    print("== pid %s  %s %s  a0=%.2f z1=%.3f z2=%.3f   (phase-1 prediction 1+0.5*a0*z)" % (pid, m["mname"], m["pname"], m["a0"], m["z1"], m["z2"]))
    line = []
    for zq in np.arange(0.05, 0.96, 0.05):
        i = np.searchsorted(zi, zq)
        if 3 < i < len(z) - 3 and p[i] > 150: line.append("%.2f:%.3f" % (zq, g[i]))
    print("   " + "  ".join(line))
