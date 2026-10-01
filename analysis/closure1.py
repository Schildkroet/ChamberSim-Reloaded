"""Judge a gas-pressure closure directly: given reference (x,z,v) predict p; minimise relative p error."""
import json, glob, re, os, numpy as np
from scipy.optimize import least_squares
R = "../reference/results/"; JA = "../tools/GRTRef/jobs_all/"; JD = "../tools/GRTRef/jobs_done/"
BAD = {"casevol_x1.25", "cv_mp200_casevol1.5", "twist_0", "45-70-Gov"}
def inp(s, sec, key):
    blk = re.search(rf"<{sec}>.*?</{sec}>", s, re.S).group(0)
    return float(re.search(rf'<input name="{key}" value="([^"]*)"', blk).group(1))
runs = {}
for f in sorted(glob.glob(R + "*.json")):
    n = os.path.basename(f)[:-5]; nn = n[4:] if n.startswith("exp_") else n
    if nn in BAD or nn.startswith(("friction", "twist", "cv_mp200_ps", "cv_mp1000", "cv_mp50", "pt_")): continue
    jp = (JA + n + ".grtload") if os.path.exists(JA + n + ".grtload") else JD + n + ".grtload"
    if not os.path.exists(jp): continue
    P = json.load(open(f))["points"]
    if not P: continue
    s = open(jp, encoding="utf-8").read(); a = lambda k: np.array([q[k] for q in P])
    runs[nn] = dict(x=a("x")*1e-3, z=a("z"), p=a("p")*1e5, v=a("v"), mc=inp(s,"propellant","mc")*1e-3, mp=inp(s,"projectile","mp")*1e-3,
        seb=inp(s,"caliber","sebert"), cv=inp(s,"caliber","casevol")*1e-6, A=inp(s,"caliber","Aeff")*1e-6, k=inp(s,"propellant","k"),
        Qex=inp(s,"propellant","Qex")*1e3, rho=inp(s,"propellant","pc"), eta=inp(s,"propellant","eta"), ps=inp(s,"caliber","ps")*1e5)
print(len(runs), "runs:", " ".join(runs))
def pred(q, r):
    lf, kap, b, al, be, F, zs = q
    th = r["k"] - 1; m_ = r["mp"] + r["seb"] * r["mc"]; dx = r["x"] - r["x"][0]
    V = al * r["cv"] + be * 1e-6 + r["A"] * dx - r["mc"] * (1 - r["z"]) / r["rho"] - b * 1e-3 * r["mc"] * r["z"]
    E = lf * th * r["Qex"] * r["mc"] * (r["z"] + zs) - th * kap * 0.5 * m_ * r["v"]**2 - F * th * dx
    return E / V
def resid(q):
    out = []
    for r in runs.values():
        sel = r["p"] > 5e7                   # > 500 bar: where it matters
        out.append((pred(q, r)[sel] - r["p"][sel]) / r["p"][sel])
    return np.concatenate(out)
q0 = [1.0, 1.0, 0.5, 0.7, 0.5, 0.0, 0.0]
sol = least_squares(resid, q0, bounds=([0.5, 0.2, -1, 0.3, -3, -2000, -0.1], [1.5, 5, 3, 1.2, 4, 3000, 0.2]))
names = ["lambda_f", "kappa(v2)", "b cm3/g", "alpha", "beta cm3", "F N", "z_s"]
print(dict(zip(names, np.round(sol.x, 4))))
e = sol.fun; print("rel p error: rms %.3f%%  max %.2f%%" % (100*np.sqrt(np.mean(e**2)), 100*np.abs(e).max()))
for n, r in runs.items():
    sel = r["p"] > 5e7; ee = (pred(sol.x, r)[sel] - r["p"][sel]) / r["p"][sel]
    print("  %-18s rms %.2f%%  max %.2f%%" % (n, 100*np.sqrt(np.mean(ee**2)), 100*np.abs(ee).max()))
