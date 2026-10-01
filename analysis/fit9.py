"""Early-phase (low-velocity) joint fit of the gas equation:
   p*(V0(casevol) + A dx - mc(1-z)/rho - b mc z) = f mc z + E0 - theta*0.5 m' v^2 (theta fixed) """
import json, glob, re, os, numpy as np
R = "../reference/results/"; JE = ["../tools/GRTRef/jobs/", "../tools/GRTRef/jobs_done/", "/home/patrick/Vorlagen/GRTRef/results_batch1/../jobs/"]
BAD = {"casevol_x1.25", "cv_mp200_casevol1.5", "pt_-20"}  # stale / not used here
def find_job(n):
    for d in ("../tools/GRTRef/jobs_all/",):
        p = d + f"exp_{n}.grtload"
        if os.path.exists(p): return p
def inp(s, sec, key):
    blk = re.search(rf"<{sec}>.*?</{sec}>", s, re.S).group(0)
    return float(re.search(rf'<input name="{key}" value="([^"]*)"', blk).group(1))
runs = {}
for f in sorted(glob.glob(R + "exp_*.json")):
    n = os.path.basename(f)[4:-5]
    if n in BAD or n.startswith(("friction", "twist")): continue
    jp = find_job(n)
    if not jp: continue
    d = json.load(open(f)); P = d["points"]
    if not P: continue
    s = open(jp, encoding="utf-8").read()
    a = lambda k: np.array([q[k] for q in P])
    runs[n] = dict(x=a("x")*1e-3, z=a("z"), p=a("p")*1e5, v=a("v"), mc=inp(s,"propellant","mc")*1e-3, mp=inp(s,"projectile","mp")*1e-3,
        seb=inp(s,"caliber","sebert"), cv=inp(s,"caliber","casevol")*1e-6, A=inp(s,"caliber","Aeff")*1e-6, ps=inp(s,"caliber","ps")*1e5)
print(len(runs), "runs usable:", list(runs))
rho, k, Qex = 1580.0, 1.2214, 3790e3; th = k - 1
VMAX = 60.0
rows, ys = [], []
for n, r in runs.items():
    m_ = r["mp"] + r["seb"] * r["mc"]; dx = r["x"] - r["x"][0]
    sel = (r["v"] < VMAX) & (r["z"] > 0)
    y = (r["p"] * (r["A"] * dx - r["mc"] * (1 - r["z"]) / rho) + th * 0.5 * m_ * r["v"]**2)[sel]
    M = np.column_stack([r["mc"]*r["z"], r["mc"]*r["p"]*r["z"], -r["cv"]*r["p"], -r["p"], np.ones_like(r["z"])])[sel]
    w = 1.0 / (r['p'][sel] * 1e-5 * 3e-6 + 50.0)      # ~ energy scale of the point [J]
    rows.append(M * w[:, None]); ys.append(y * w)
M = np.vstack(rows); y = np.concatenate(ys)
cn = np.linalg.norm(M, axis=0); sol, *_ = np.linalg.lstsq(M / cn, y, rcond=None); sol = sol / cn
for nm, s_ in zip(["f(J/kg)", "b(m3/kg)", "alpha(casevol)", "beta(m3)", "E0(J)"], sol): print("%-16s %.6g" % (nm, s_))
res = M @ sol - y; print("rms rel resid %.2e  max %.2e   (expected f=%.0f)" % (np.sqrt(np.mean(res**2)), np.max(np.abs(res)), th*Qex))

print("\nper-run residual (J) over the low-velocity points, with the joint solution:")
for n, r in runs.items():
    m_ = r["mp"] + r["seb"] * r["mc"]; dx = r["x"] - r["x"][0]
    sel = (r["v"] < VMAX) & (r["z"] > 0)
    y = (r["p"] * (r["A"] * dx - r["mc"] * (1 - r["z"]) / rho) + th * 0.5 * m_ * r["v"]**2)[sel]
    M = np.column_stack([r["mc"]*r["z"], r["mc"]*r["p"]*r["z"], -r["cv"]*r["p"], -r["p"], np.ones_like(r["z"])])[sel]
    e = M @ sol - y
    print("  %-18s n=%4d  pmax=%7.0f bar  rms=%8.2f J  max=%8.2f J   y_range=[%.0f, %.0f]" % (n, sel.sum(), r["p"].max()/1e5, np.sqrt(np.mean(e**2)), np.abs(e).max(), y.min(), y.max()))
