"""Judge a gas-pressure closure directly: given reference (x,z,v) predict p; minimise relative p error."""
import json, glob, re, os, numpy as np
from scipy.optimize import least_squares
R = "../reference/results/"; JA = "../tools/GRTRef/jobs_all/"; JD = "../tools/GRTRef/jobs_done/"
BAD = {"casevol_x1.25", "cv_mp200_casevol1.5", "twist_0"}
def inp(s, sec, key):
    blk = re.search(rf"<{sec}>.*?</{sec}>", s, re.S).group(0)
    return float(re.search(rf'<input name="{key}" value="([^"]*)"', blk).group(1))
runs = {}
for f in sorted(glob.glob(R + "*.json")):
    n = os.path.basename(f)[:-5]; nn = n[4:] if n.startswith("exp_") else n
    if nn in BAD or not (nn in ("45-70-Gov","8x57is N130","base","mc_x0.8","mc_x1.2","mp_x0.5","mp_x2.0","xe_200","xe_1000","ps_100","sebert_1.0")): continue
    jp = (JA + n + ".grtload") if os.path.exists(JA + n + ".grtload") else JD + n + ".grtload"
    if not os.path.exists(jp): continue
    P = json.load(open(f))["points"]
    if not P: continue
    s = open(jp, encoding="utf-8").read(); a = lambda k: np.array([q[k] for q in P])
    runs[nn] = dict(x=a("x")*1e-3, z=a("z"), p=a("p")*1e5, v=a("v"), mc=inp(s,"propellant","mc")*1e-3, mp=inp(s,"projectile","mp")*1e-3,
        seb=inp(s,"caliber","sebert"), cv=inp(s,"caliber","casevol")*1e-6, A=inp(s,"caliber","Aeff")*1e-6, k=inp(s,"propellant","k"),
        Qex=inp(s,"propellant","Qex")*1e3, rho=inp(s,"propellant","pc"), eta=inp(s,"propellant","eta"), ps=inp(s,"caliber","ps")*1e5,
        D=inp(s,"projectile","Dbul")*1e-3, depth=inp(s,"projectile","gdepth")*1e-3, th_=inp(s,"projectile","gtailh")*1e-3,
        dB=inp(s,"projectile","gTailDiaB")*1e-3, tt=inp(s,"projectile","gTailType"))
print(len(runs), "runs:", " ".join(runs))
def intrusion(r):
    A_b = np.pi / 4 * r["D"]**2; v = A_b * r["depth"]
    if r["tt"] == 2 and r["depth"] >= r["th_"]:
        h, R, rr = r["th_"], r["D"] / 2, r["dB"] / 2
        v -= A_b * h - np.pi * h / 3 * (R*R + R*rr + rr*rr)
    return v
def pred(q, r):
    """p from reference (x,z,v) with an accumulated wall-heat-loss term:
       p*Vg = th*(lf*Qex*mc*(z+zs) - kap*0.5*m'*v^2 - Qloss),  dQloss/dt = h*Sw(x)*(T/Tex - tw)*Qscale"""
    lf, kap, b, ga, be, hh, zs, tw, c0 = q
    th = r["k"] - 1; m_ = r["mp"] + r["seb"] * r["mc"]; x = r["x"]; dx = x - x[0]
    z, v = r["z"], r["v"]
    Vg = r["cv"] - ga * intrusion(r) + be * 1e-6 + r["A"] * dx - r["mc"] * (1 - z) / r["rho"] - b * 1e-3 * r["mc"] * z
    Sw = c0 * 1e-4 + np.pi * r["D"] * dx
    Eg = lf * th * r["Qex"] * r["mc"] * (z + zs)
    n = len(x); dt = np.zeros(n)
    vv = v[1:] + v[:-1]; dt[1:] = np.where(vv > 0, 2 * (x[1:] - x[:-1]) / np.where(vv > 0, vv, 1), 0)
    if v[1] > 0: dt[1] = 2 * (x[1] - x[0]) / v[1]
    p = np.zeros(n); Q = 0.0; Tex_ratio = 0.0
    for i in range(n):
        if i > 0 and Eg[i] > 0:
            Sm = 0.5 * (Sw[i] + Sw[i-1]); Q += hh * Sm * max(Tex_ratio - tw, 0.0) * dt[i] * r["Qex"] * r["mc"] * 1e-1
        E = Eg[i] - th * kap * 0.5 * m_ * v[i]**2 - th * Q
        p[i] = E / Vg[i]
        Tex_ratio = (p[i] * Vg[i]) / (lf * th * r["Qex"] * r["mc"] * (z[i] + zs) + 1e-9) if z[i] + zs > 0 else 0.0
    return p
def resid(q):
    out = []
    for r in runs.values():
        sel = r["p"] > 5e7                   # > 500 bar: where it matters
        out.append((pred(q, r)[sel] - r["p"][sel]) / r["p"][sel])
    return np.concatenate(out)
q0 = [1.0, 1.0, 0.5, 1.0, 0.0, 50.0, 0.02, 0.1, 20.0]
sol = least_squares(resid, q0, bounds=([0.5, 0.2, -1, 0.0, -3, 0.0, -0.1, 0.0, 0.0], [1.5, 5, 3, 3.0, 4, 5000, 0.2, 0.6, 200.0]))
names = ["lambda_f", "kappa(v2)", "b cm3/g", "gamma(intr)", "beta cm3", "h", "z_s", "Tw/Tex", "c0 cm2"]
print(dict(zip(names, np.round(sol.x, 4))))
e = sol.fun; print("rel p error: rms %.3f%%  max %.2f%%" % (100*np.sqrt(np.mean(e**2)), 100*np.abs(e).max()))
for n, r in runs.items():
    sel = r["p"] > 5e7; ee = (pred(sol.x, r)[sel] - r["p"][sel]) / r["p"][sel]
    print("  %-18s rms %.2f%%  max %.2f%%" % (n, 100*np.sqrt(np.mean(ee**2)), 100*np.abs(ee).max()))
