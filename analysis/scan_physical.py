#!/usr/bin/env python3
"""Tests physical-model constants on published load data, powder by powder (fast: the powders are independent).

    scan_physical.py <csr_eval> samples.csv [samples2.csv ...] [--set name=value ...] [--min-samples 60] [--powders "ADI AR2208,..."]

For every powder with physical burn values (data/physical/propellants.json) the burn rate Ba is fitted on the training cartridges (all but
every 4th cartridge of the sorted list) under the given constants, the form function stays. Reported: velocity / pressure error on the
training and held-out cartridges and the trends of the error with loading density and bullet mass (slope of ln(simulated / published) over
ln(charge / case volume) and ln(bullet mass): zero for a model that follows the data). The powders run in parallel (one csr_eval per
powder and evaluation, `--jobs` threads).
"""
import argparse, csv, glob, json, os, subprocess, sys, tempfile
import numpy as np
from concurrent.futures import ThreadPoolExecutor
from scipy.optimize import minimize_scalar, least_squares

ap = argparse.ArgumentParser()
ap.add_argument("eval_exe"); ap.add_argument("samples", nargs="+")
ap.add_argument("--set", action="append", default=[], help="calibration constant name=value (repeatable)")
ap.add_argument("--min-samples", type=int, default=60)
ap.add_argument("--powders", default="")
ap.add_argument("--jobs", type=int, default=int(os.environ.get("CSR_JOBS", min(14, os.cpu_count() or 1))), help="threads (default: at most 14 cores, or CSR_JOBS)")
ap.add_argument("--pressure-weight", type=float, default=1.0)
ap.add_argument("--quiet", action="store_true")
ap.add_argument("--fit-form", action="store_true", help="fit the form function (a0, z1, z2) as well as Ba, with the pressures (slower)")
args = ap.parse_args()
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
PHYS = {(e["mname"] + " " + e["pname"]).lower(): e for e in json.load(open(os.path.join(ROOT, "data", "physical", "propellants.json")))["propellants"]}
case = {}
for f in glob.glob(os.path.join(ROOT, "data", "calibers", "*.json")):
    r = json.load(open(f)); case[r["cipname"]] = r

by_name = {}
for k, e in PHYS.items(): by_name.setdefault(e["pname"].lower(), []).append(k)
rows, seen = [], set()
for f in args.samples:
    for r in csv.DictReader(open(f)):
        r["_src"] = os.path.basename(f).split("_")[0]
        if r["powder"].lower() not in PHYS and len(by_name.get(r["powder"].lower(), [])) == 1:  # "N150" of the Vihtavuori files
            k = by_name[r["powder"].lower()][0]; r["powder"] = PHYS[k]["mname"] + " " + PHYS[k]["pname"]
        r.setdefault("pressure_bar", "")  # files without pressures (Vihtavuori)
        if r["pressure_bar"] and not 500 <= float(r["pressure_bar"]) <= 9000: r["pressure_bar"] = ""
        key = (r["powder"].upper(), r["caliber"], r["bullet_gr"], r["kind"], r["charge_g"], r["velocity_ms"], r["col_mm"])
        if key not in seen: seen.add(key); rows.append(r)
held = set(sorted({r["caliber"] for r in rows})[3::4])
by = {}
for r in rows:
    if r["powder"].lower() in PHYS: by.setdefault(r["powder"], []).append(r)
want = [p.strip() for p in args.powders.split(",") if p.strip()]
powders = [p for p in sorted(by) if len(by[p]) >= args.min_samples and (not want or p in want)]
tmp = tempfile.mkdtemp()
header = [h for h in rows[0].keys() if h != "_src"]

class Powder:
    def __init__(self, name):
        self.name, self.rows = name, by[name]
        self.path = os.path.join(tmp, name.replace(" ", "_") + ".csv")
        with open(self.path, "w", newline="") as f:
            w = csv.DictWriter(f, header, extrasaction="ignore"); w.writeheader(); w.writerows(self.rows)
        self.mv = np.array([float(r["velocity_ms"]) for r in self.rows])
        self.mp = np.array([float(r["pressure_bar"]) if r["pressure_bar"] else np.nan for r in self.rows])
        self.test = np.array([r["caliber"] in held for r in self.rows])
        e = PHYS[name.lower()]; self.e = e
    def sim(self, ba, form=None):
        e = self.e
        a0, z1, z2 = form if form else (e["a0"], e["z1"], e["z2"])
        cmd = [args.eval_exe, self.path, "--profile", "physical", "--phys", f"{self.name.lower()}={ba:.8g},{a0:.8g},{z1:.8g},{z2:.8g}"]
        cmd += [a for kv in args.set for a in ("--set", kv)]
        out = subprocess.run(cmd, capture_output=True, text=True).stdout
        v = np.full(len(self.rows), np.nan); p = np.full(len(self.rows), np.nan)
        for l in out.splitlines():
            a = l.split(",")
            if len(a) == 3: v[int(a[0])], p[int(a[0])] = float(a[1]), float(a[2])
        return v, p
    def resid(self, ba, form=None):
        v, p = self.sim(ba, form)
        return np.log(v / self.mv), np.log(p / self.mp)
    def fit_form(self):
        """Ba, a0, z1 and z2 (= z1 + d (1 - z1)) on the training cartridges, robust least squares on the relative errors."""
        e = self.e
        def unpack(x): return float(np.exp(x[0])), (x[1], x[2], x[2] + x[3] * (1 - x[2]))
        def fun(x):
            ba, form = unpack(x); ev, ep = self.resid(ba, form); m = ~self.test
            return np.concatenate([np.nan_to_num(ev[m]), args.pressure_weight * np.nan_to_num(ep[m])])
        d0 = (e["z2"] - e["z1"]) / (1 - e["z1"])
        x0 = np.array([np.log(e["Ba"]), e["a0"], e["z1"], d0])
        lo, hi = np.array([np.log(0.05), -3.0, 0.05, 0.05]), np.array([np.log(20.0), 10.0, 0.8, 0.95])  # as fit_physical_powders.py
        sol = least_squares(fun, np.clip(x0, lo + 1e-6, hi - 1e-6), loss="soft_l1", f_scale=0.1, diff_step=2e-3, x_scale=np.maximum(np.abs(x0), 0.1), max_nfev=40, bounds=(lo, hi))
        self.ba, self.form = unpack(sol.x)
        self.ev, self.ep = self.resid(self.ba, self.form)
        return self

    def fit(self):
        if args.fit_form and np.sum(~np.isnan(self.mp) & ~self.test) >= 20: return self.fit_form()  # with pressures: Ba and the form function
        def cost(lb):
            ev, ep = self.resid(np.exp(lb)); m = ~self.test & ~np.isnan(ev)
            c = np.concatenate([ev[m], args.pressure_weight * ep[m & ~np.isnan(ep)]])
            return float(np.mean(np.minimum(c ** 2, 0.04))) if len(c) else 1e9  # robust against rejected loads
        start = np.log(self.e["Ba"])
        sol = minimize_scalar(cost, bracket=(start - 0.15, start + 0.15), tol=1e-3, options={"maxiter": 25})
        self.ba = float(np.exp(sol.x)); self.ev, self.ep = self.resid(self.ba)
        return self

def covariates(P):
    X = []
    for r in P.rows:
        c = case.get(r["caliber"], {}); V = c.get("V", 0) * 0.0648
        X.append([np.log(float(r["charge_g"]) / V) if V > 0 else np.nan, np.log(float(r["bullet_gr"]) * 0.0648)])
    return np.array(X)

with ThreadPoolExecutor(args.jobs) as pool:
    fits = list(pool.map(lambda n: Powder(n).fit(), powders))
def rms(x): x = x[~np.isnan(x)]; return 100 * np.sqrt(np.mean(x ** 2)) if len(x) else float("nan")
def stat(sel):
    ev = np.concatenate([f.ev[sel(f)] for f in fits]); ep = np.concatenate([f.ep[sel(f)] for f in fits])
    return rms(ev), rms(ep), 100 * np.nanmean(ep)
tr, te = stat(lambda f: ~f.test), stat(lambda f: f.test)
allev = np.concatenate([f.ev for f in fits]); allep = np.concatenate([f.ep for f in fits]); X = np.vstack([covariates(f) for f in fits])
def slope(y, j):
    m = ~np.isnan(y) & ~np.isnan(X[:, j]); return np.polyfit(X[m, j], y[m], 1)[0] if m.sum() > 3 else float("nan")
kind = np.array([r["kind"] for f in fits for r in f.rows])
if not args.quiet:
    for f in fits: print(f"  {f.name:22} Ba {f.ba:.4f} (file {f.e['Ba']:.4f})  v {rms(f.ev[~f.test]):.2f}/{rms(f.ev[f.test]):.2f} %  p {rms(f.ep[~f.test]):.2f}/{rms(f.ep[f.test]):.2f} %")
print(f"train      : velocity {tr[0]:.2f} %  pressure {tr[1]:.2f} %  (bias {tr[2]:+.1f} %)")
def med(x): x = np.abs(x[~np.isnan(x)]); return 100 * np.median(x) if len(x) else float("nan")
hev = np.concatenate([f.ev[f.test] for f in fits]); hep = np.concatenate([f.ep[f.test] for f in fits])
print(f"held-out median |error|: velocity {med(hev):.2f} %  pressure {med(hep):.2f} %")
print(f"held-out   : velocity {te[0]:.2f} %  pressure {te[1]:.2f} %  (bias {te[2]:+.1f} %)")
src = np.array([r["_src"] for f in fits for r in f.rows]); tst = np.concatenate([f.test for f in fits])
for name in sorted(set(src)):
    m = (src == name) & tst
    print(f"  {name:8} held-out: velocity {rms(allev[m]):.2f} %  pressure {rms(allep[m]):.2f} %  (bias {100*np.nanmean(allep[m]) if np.any(~np.isnan(allep[m])) else float('nan'):+.1f} %)  n={int(m.sum())}")
print(f"trends (slope of ln sim/published): loading density  v {slope(allev,0):+.3f} p {slope(allep,0):+.3f} | bullet mass  v {slope(allev,1):+.3f} p {slope(allep,1):+.3f}")
print(f"start/max velocity bias: {100*np.nanmean(allev[kind=='start']):+.2f} / {100*np.nanmean(allev[kind=='max']):+.2f} %  pressure {100*np.nanmean(allep[kind=='start']):+.2f} / {100*np.nanmean(allep[kind=='max']):+.2f} %")
