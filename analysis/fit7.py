"""Joint fit across experiments of
   p*(A dx - mc(1-z)/rho) = f*mc*z  - cv*(m' v^2/2) + b*mc*p*z - alpha*casevol*p - beta*p - F*dx + E0
 (unknown: f, cv, b, alpha, beta, F, E0)"""
import json, glob, re, os, numpy as np
J = "../tools/GRTRef/jobs_done/"; JE = "../tools/GRTRef/jobs/"
R = "../reference/results/"
def inp(path, sec, key):
    s = open(path, encoding="utf-8").read()
    blk = re.search(rf"<{sec}>.*?</{sec}>", s, re.S).group(0)
    return float(re.search(rf'<input name="{key}" value="([^"]*)"', blk).group(1))
runs = {}
for f in sorted(glob.glob(R + "exp_*.json")):
    n = os.path.basename(f)[4:-5]
    if n.startswith(("pt_", "friction", "twist")): continue
    jp = JE + f"exp_{n}.grtload"
    d = json.load(open(f)); P = d["points"]
    a = lambda k: np.array([q[k] for q in P])
    runs[n] = dict(x=a("x")*1e-3, z=a("z"), p=a("p")*1e5, v=a("v"),
        mc=inp(jp,"propellant","mc")*1e-3, mp=inp(jp,"projectile","mp")*1e-3, seb=inp(jp,"caliber","sebert"),
        cv=inp(jp,"caliber","casevol")*1e-6, A=inp(jp,"caliber","Aeff")*1e-6, ps=inp(jp,"caliber","ps")*1e5)
rho = 1580.0; k=1.2214; Qex=3790e3
rows=[]; ys=[]
for n, r in runs.items():
    m_ = r["mp"] + r["seb"] * r["mc"]; dx = r["x"] - r["x"][0]
    y = r["p"] * (r["A"] * dx - r["mc"] * (1 - r["z"]) / rho)
    M = np.column_stack([r["mc"]*r["z"], -m_*r["v"]**2/2, r["mc"]*r["p"]*r["z"], -r["cv"]*r["p"], -r["p"], -dx, np.ones_like(dx)])
    w = 1.0 / np.max(np.abs(y))
    rows.append(M*w); ys.append(y*w)
M = np.vstack(rows); y = np.concatenate(ys)
sol, *_ = np.linalg.lstsq(M, y, rcond=None)
names = ["f(J/kg)", "cv(theta)", "b(m3/kg)", "alpha", "beta(m3)", "F(N)", "E0(J)"]
for n_, s_ in zip(names, sol): print("%-10s %.6g" % (n_, s_))
print("expected: f=%.0f theta=%.4f" % ((k-1)*Qex, k-1))
res = M @ sol - y; print("rms rel resid %.2e max %.2e" % (np.sqrt(np.mean(res**2)), np.max(np.abs(res))))
for n, r in runs.items():
    pass
