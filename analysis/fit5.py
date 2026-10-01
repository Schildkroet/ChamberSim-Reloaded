import json, numpy as np
for name, (z1, z2, a0, Ba) in {"8x57is": (0.4697, 0.8169, 1.1286, 0.5814)}.items():
    d = json.load(open(f"../reference/results/{name}.json")); P = d["points"]
    x, z, p, v = (np.array([q[k] for q in P]) for k in "xzpv")
    dzdx = np.gradient(z, x * 1e-3)                    # 1/m
    dzdt = v * dzdx                                    # 1/s
    g = dzdt / (p * 1e5) * 1e5                         # 1/(s*bar)
    print(name, "  z1=%.4f z2=%.4f   (g = dz/dt / p_bar)" % (z1, z2))
    for i in (3, 10, 30, 60, 100, 150, 180, 185, 190, 250, 400, 600, 800, 990, 994, 1000, 1200, 1600, 1990):
        print("  i=%4d z=%.4f p=%6.0f  dz/dt=%9.1f /s   g=%.4f   g/Ba=%.4f" % (i, z[i], p[i], dzdt[i], g[i], g[i] / Ba))
