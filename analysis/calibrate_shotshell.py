#!/usr/bin/env python3
"""Fits the shotshell constants of the solver and the burn rates of shotgun powders to published shotgun loads.

    calibrate_shotshell.py <grt_eval> <data dir with propellants/*.json of the shotgun powders> shot_samples.csv [more.csv ...]

The CSVs come from tools/loaddata/hodgdon_shot_extract.py (columns powder, gauge, hull_mm, shot_g, charge_g, velocity_ms, pressure_bar, primer).
The shot column and 2.5 g of wad are the projectile (README: shotgun loads), the barrel is 28" (711 mm). The velocities are measured about 3 ft
(0.91 m) in front of the muzzle: they are converted to the muzzle with the air drag of 2.4 mm lead pellets.
Training: all loads except those with the held-out primer (Win. 209); the held-out ones test the fit on other primers.
First the burn rates alone with the current constants (baseline), then constants and burn rates together."""
import csv, json, math, os, subprocess, sys, tempfile, numpy as np
from concurrent.futures import ThreadPoolExecutor
from scipy.optimize import least_squares

EXE, DATA = sys.argv[1], sys.argv[2]
HELD_OUT_PRIMER = os.environ.get("HELD_OUT_PRIMER", "Win. 209")
rows, seen = [], set()
for f in sys.argv[3:]:
    for r in csv.DictReader(open(f)):
        key = tuple(r[k] for k in ("powder", "gauge", "hull_mm", "shot_g", "charge_g", "velocity_ms", "wad", "primer"))
        if key not in seen: seen.add(key); rows.append(r)
fields = list(rows[0].keys())
powders = sorted({r["powder"] for r in rows})
held = [r for r in rows if r["primer"].replace(" ", "") == HELD_OUT_PRIMER.replace(" ", "")]
train = [r for r in rows if r not in held]
tmp = tempfile.mkdtemp()
POOL = ThreadPoolExecutor(os.cpu_count())
K = 1.2 * 0.47 * 3.0 / (8 * 11340.0 * 0.0012)  # air drag of a 2.4 mm lead pellet per metre
CORR = math.exp(K * 0.914)

def simulate(rs, cal, ba, chunks=16):
    n = len(rs)
    def one(idx):
        if not idx: return []
        path = os.path.join(tmp, f"c{idx[0]}_{os.getpid()}.csv")
        with open(path, "w", newline="") as f:
            w = csv.DictWriter(f, fields); w.writeheader(); w.writerows([rs[i] for i in idx])
        cmd = [EXE, path, "--data", DATA] + [a for k, v in cal.items() for a in ("--set", f"{k}={v:.8g}")] + [a for k, v in ba.items() for a in ("--ba", f"{k.lower()}={v:.8g}")]
        out = subprocess.run(cmd, capture_output=True, text=True).stdout
        os.remove(path)
        return [(idx[int(l.split(",")[0])], float(l.split(",")[1]), float(l.split(",")[2])) for l in out.splitlines() if l]
    v = np.full(n, np.nan); p = np.full(n, np.nan)
    for res in POOL.map(one, [list(range(i, n, chunks)) for i in range(chunks)]):
        for i, vv, pp in res: v[i], p[i] = vv, pp
    return v, p

def residuals(rs, cal, ba):
    v, p = simulate(rs, cal, ba)
    mv = np.array([float(r["velocity_ms"]) for r in rs]) * CORR
    mp = np.array([float(r["pressure_bar"]) if r["pressure_bar"] else np.nan for r in rs])
    ok = ~np.isnan(v)
    rv = np.where(ok, v / mv - 1, 0.0)
    rp = np.where(ok & ~np.isnan(mp), p / mp - 1, 0.0)
    return rv, rp, ~np.isnan(mp) & ok

def stats(rs, cal, ba):
    rv, rp, has = residuals(rs, cal, ba)
    return "velocity %.2f %% (bias %+.1f %%)  pressure %.2f %% (bias %+.1f %%)" % (100 * np.sqrt(np.mean(rv ** 2)), 100 * rv.mean(), 100 * np.sqrt(np.mean(rp[has] ** 2)), 100 * rp[has].mean())

NAMES = ["shotCapacityFactor", "shotFrictionShare", "shotEnergyFactor"]
cur = {}
for line in subprocess.run([EXE, "--list-params", "x"], capture_output=True, text=True).stdout.splitlines():
    k, v = line.split(); cur[k] = float(v)
ba0 = np.full(len(powders), 1.5)
def fit_ba(cal):
    f = lambda x: np.concatenate(residuals(train, cal, dict(zip(powders, x)))[:2])
    return least_squares(f, ba0, loss="soft_l1", f_scale=0.1, bounds=(0.05, 30), diff_step=2e-3, max_nfev=15, x_scale=np.maximum(ba0, 0.05)).x
print(len(train), "training loads,", len(held), "held out (primer %s),"% HELD_OUT_PRIMER, len(powders), "powders", flush=True)
ba_cur = fit_ba(cur)
print("current constants, Ba fitted:", {p: round(b, 3) for p, b in zip(powders, ba_cur)}, flush=True)
x0 = np.concatenate([[cur[n] for n in NAMES], ba_cur])
lo = np.concatenate([[0.5, 0.0, 0.5], np.full(len(powders), 0.05)]); hi = np.concatenate([[4.0, 0.8, float(os.environ.get("ENERGY_MAX", "1.0"))], np.full(len(powders), 30.0)])
x0 = np.clip(x0, lo + 1e-6, hi - 1e-6)
def fun(x):
    cal = {**dict(zip(NAMES, x[:3]))}; return np.concatenate(residuals(train, cal, dict(zip(powders, x[3:])))[:2])
print("joint fit ...", flush=True)
sol = least_squares(fun, x0, loss="soft_l1", f_scale=0.1, bounds=(lo, hi), diff_step=2e-3, max_nfev=15, x_scale=np.maximum(np.abs(x0), 0.05))
new = dict(zip(NAMES, sol.x[:3])); ba_new = dict(zip(powders, sol.x[3:]))
json.dump({"calibration": new, "ba": ba_new}, open(os.environ.get("OUT", "cal_shot.json"), "w"), indent=1)
for title, cal, ba in (("current constants (Ba fitted)", {n: cur[n] for n in NAMES}, dict(zip(powders, ba_cur))), ("recalibrated", new, ba_new)):
    print("\n==", title); print("   " + "  ".join(f"{n}={cal[n]:.4g}" for n in NAMES))
    print("   training  :", stats(train, cal, ba)); print("   held out  :", stats(held, cal, ba))
    for p in powders:
        sub = [r for r in rows if r["powder"] == p]; print("     %-26s n=%4d Ba %.3f  %s" % (p, len(sub), ba[p], stats(sub, cal, ba)))
