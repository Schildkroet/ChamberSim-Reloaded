import json, glob, re, os, numpy as np
exec(open("fit7.py").read().split("rho = 1580.0")[0])     # reuse loader -> runs
rho=1580.0; k=1.2214; Qex=3790e3
def design(r, cols):
    m_ = r["mp"] + r["seb"] * r["mc"]; dx = r["x"] - r["x"][0]
    y = r["p"] * (r["A"] * dx - r["mc"] * (1 - r["z"]) / rho)
    C = {"f": r["mc"]*r["z"]*1e6, "cv": -m_*r["v"]**2/2, "b": r["mc"]*r["p"]*r["z"]*1e-3, "V0": -r["p"]*1e-6, "F": -dx, "E0": np.ones_like(dx)}
    return np.column_stack([C[c] for c in cols]), y
def fit(sel, cols):
    Ms, ys = [], []
    for n in sel:
        M, y = design(runs[n], cols); w = 1/np.max(np.abs(y)); Ms.append(M*w); ys.append(y*w)
    M = np.vstack(Ms); y = np.concatenate(ys)
    s, *_ = np.linalg.lstsq(M, y, rcond=None)
    per = {n: np.max(np.abs(design(runs[n], cols)[0] @ s - design(runs[n], cols)[1])) / np.max(np.abs(design(runs[n], cols)[1])) for n in sel}
    return s, per
sel = ["base", "mc_x0.8", "mc_x1.2", "mp_x0.5", "mp_x2.0", "sebert_0.0", "sebert_1.0"]
for cols in (["f","cv","b","V0"], ["f","cv","b","V0","F"], ["f","cv","b","V0","F","E0"]):
    s, per = fit(sel, cols)
    print(cols, " ".join("%s=%.4g" % (c, v) for c, v in zip(cols, s)))
    print("   per-run max rel resid:", {n: "%.1e" % q for n, q in per.items()})
print("expect f=%.3f (x1e6)  cv=%.4f" % ((k-1)*Qex/1e6, k-1))
