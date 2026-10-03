#!/usr/bin/env python3
"""Recalibrates solver constants on published load data (tools/loaddata CSVs) and compares them with the current calibration.

    calibrate_published.py <grt_eval> <grt_compare> samples.csv [samples2.csv ...] [--powders "Accurate 4350,..."] [--free name,name,...]
                           [--profile original|published|physical] [--start constants.json] [--evaluate-only]

--profile chooses the model and its starting constants (physical = the project's physical model); --start replaces some of them (a
JSON file as written by --out); --evaluate-only reports the starting constants without fitting. grt_compare (agreement with the original
program) only runs for the original profile: the physical model is not meant to reproduce it.

Training: the given powders (burn rates Ba free per powder, everything else of the records fixed) on the cartridges of the training split.
Test A: the same powders on the held-out cartridges (every 4th cartridge). Test B: other powders, whose Ba is fitted on their training
cartridges under the respective calibration and which are evaluated on the held-out ones. Both calibrations (the current one with Ba fitted,
and the recalibrated one) are compared; grt_compare (the 190+ reference runs of the original program) shows what the new constants do to
the accuracy against the original.
Residuals: relative velocity error and relative pressure error (loads with a PSI pressure), robust loss (soft L1)."""
import argparse, csv, json, os, subprocess, sys, tempfile
import numpy as np
from concurrent.futures import ThreadPoolExecutor
from scipy.optimize import least_squares

ap = argparse.ArgumentParser()
ap.add_argument("eval_exe"); ap.add_argument("compare_exe"); ap.add_argument("samples", nargs="+")
ap.add_argument("--powders", default="Accurate 4350,Ramshot Hunter,Accurate 2700,Ramshot Magnum,Ramshot Big Game,Hodgdon H4350")
ap.add_argument("--free", default="impetusFactor,kineticFactor,covolume_cm3g,covolumeDensityFactor,solidDensityFactor,intrusionFactor,"
                                  "capacityFactor,chamberOffsetLength_mm,burnIntercept,burnExponent")
ap.add_argument("--test-powders", default="IMR 4350,Hodgdon Varget,Hodgdon H4831,Winchester W760,IMR 4064,Accurate 2520,IMR 4831,Hodgdon H4895")
ap.add_argument("--pressure-weight", type=float, default=1.0)
ap.add_argument("--bound-frac", type=float, default=0.0, help="keep the constants within +-this fraction of the current values (0: unbounded)")
ap.add_argument("--bounds", default="", help="name=lo:hi,... explicit limits of constants")
ap.add_argument("--fixed-ba", action="store_true", help="keep the burn rates of the records (add burnRateScale to --free); nothing is refitted per powder")
ap.add_argument("--out", default="calibrated_published.json")
ap.add_argument("--profile", default="original", choices=["original", "published", "physical"])
ap.add_argument("--start", default="", help="JSON file ({\"calibration\": {name: value}}) with starting constants")
ap.add_argument("--evaluate-only", action="store_true", help="only report the starting constants")
args = ap.parse_args()
POOL = ThreadPoolExecutor(os.cpu_count())

# ---- samples: merge, drop identical ones and misread pressures, split by cartridge
rows, seen = [], set()
for f in args.samples:
    for r in csv.DictReader(open(f)):
        if r["pressure_bar"] and not 500 <= float(r["pressure_bar"]) <= 9000: r["pressure_bar"] = ""
        key = (r["powder"].upper(), r["caliber"], r["bullet_gr"], r["kind"], r["charge_g"], r["velocity_ms"], r["col_mm"])
        if key not in seen: seen.add(key); rows.append(r)
cartridges = sorted({r["caliber"] for r in rows})
test_cartridges = set(cartridges[3::4])
names_of = {r["powder"].upper(): r["powder"] for r in rows}
train_p = [names_of[p.upper()] for p in args.powders.split(",")]
test_p = [names_of[p.upper()] for p in args.test_powders.split(",")]

def subset(powders, test):
    return [r for r in rows if r["powder"] in powders and (r["caliber"] in test_cartridges) == test]
sets = {"train": subset(train_p, False), "testA": subset(train_p, True), "B_fit": subset(test_p, False), "B_test": subset(test_p, True)}
tmp = tempfile.mkdtemp()
files = {}
for name, rs in sets.items():
    files[name] = os.path.join(tmp, name + ".csv")
    with open(files[name], "w", newline="") as f:
        w = csv.DictWriter(f, list(rows[0].keys())); w.writeheader(); w.writerows(rs)
print({k: len(v) for k, v in sets.items()}, "test cartridges:", len(test_cartridges), "of", len(cartridges), flush=True)

def simulate(name, cal, ba, chunks=16):
    """(velocity, pressure) arrays of a set (nan where it cannot be simulated)."""
    rs = sets[name]; n = len(rs)
    parts = [list(range(i, n, chunks)) for i in range(chunks)]
    def one(idx):
        if not idx: return []
        path = os.path.join(tmp, f"{name}_{os.getpid()}_{idx[0]}.csv")
        with open(path, "w", newline="") as f:
            w = csv.DictWriter(f, list(rows[0].keys())); w.writeheader(); w.writerows([rs[i] for i in idx])
        cmd = [args.eval_exe, path, "--profile", args.profile] + [a for k, v in cal.items() for a in ("--set", f"{k}={v:.10g}")] + [a for k, v in ba.items() for a in ("--ba", f"{k}={v:.8g}")]
        out = subprocess.run(cmd, capture_output=True, text=True).stdout
        os.remove(path)
        return [(idx[int(l.split(",")[0])], float(l.split(",")[1]), float(l.split(",")[2])) for l in out.splitlines() if l]
    v = np.full(n, np.nan); p = np.full(n, np.nan)
    for res in POOL.map(one, parts):
        for i, vv, pp in res: v[i], p[i] = vv, pp
    return v, p

def measured(name):
    rs = sets[name]
    return (np.array([float(r["velocity_ms"]) for r in rs]), np.array([float(r["pressure_bar"]) if r["pressure_bar"] else np.nan for r in rs]))

def residuals(name, cal, ba):
    v, p = simulate(name, cal, ba); mv, mp = measured(name)
    ok = ~np.isnan(v)
    rv = np.where(ok, v / mv - 1, 0.0)  # loads the solver rejects (charge too high for the case) are left out, like in grt_fitdata
    rp = np.where(~np.isnan(mp) & ok, p / mp - 1, 0.0) * args.pressure_weight
    return rv, rp, mp

def stats(name, cal, ba):
    rv, rp, mp = residuals(name, cal, ba); has = ~np.isnan(mp) & (rp != 0)
    return dict(v_rms=100 * np.sqrt(np.mean(rv ** 2)), p_rms=100 * np.sqrt(np.mean((rp[has] / args.pressure_weight) ** 2)), p_bias=100 * np.mean(rp[has] / args.pressure_weight))

current = {}
for line in subprocess.run([args.eval_exe, "--list-params", "--profile", args.profile, "x"], capture_output=True, text=True).stdout.splitlines():
    k, v = line.split(); current[k] = float(v)
if args.start: current.update(json.load(open(args.start))["calibration"])
free = args.free.split(",")
ba0 = {p.lower(): 0.6 for p in train_p}
records = {}
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools"))
import recordfiles  # noqa: E402  data/propellants/<manufacturer>.json
for _, r in recordfiles.read(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "propellants")):
    records[(r["mname"] + " " + r["pname"]).lower()] = r["Ba"]
ba0 = {p.lower(): records[p.lower()] for p in train_p}

def fit_ba(setname, powders, cal, start):
    """Ba of each powder under `cal`, fitted on the given set (velocity and pressure)."""
    ba = dict(start)
    def fun(x):
        b = {p.lower(): float(v) for p, v in zip(powders, x)}
        rv, rp, _ = residuals(setname, cal, b)
        return np.concatenate([rv, rp])
    x0 = np.array([start[p.lower()] for p in powders])
    sol = least_squares(fun, x0, x_scale=np.maximum(x0, 0.05), loss="soft_l1", f_scale=0.1, diff_step=2e-3, max_nfev=12, bounds=(0.05, 20))
    return {p.lower(): float(v) for p, v in zip(powders, sol.x)}

# ---- comparison
def report(title, cal, ba):
    print(f"\n== {title}")
    for n in free: print("   %-24s %12.6g   (current %.6g)" % (n, cal[n], current[n]))
    s = {k: stats(k, cal, ba) for k in ("train", "testA")}
    print("   training powders, training cartridges : velocity %.2f %%  pressure %.2f %%  (bias %+.1f %%)" % tuple(s["train"].values()))
    print("   training powders, held-out cartridges : velocity %.2f %%  pressure %.2f %%  (bias %+.1f %%)" % tuple(s["testA"].values()))
    bb = {} if args.fixed_ba else fit_ba("B_fit", test_p, cal, {p.lower(): records[p.lower()] for p in test_p})
    sb = stats("B_test", cal, bb)
    print("   other powders (Ba fitted), held-out    : velocity %.2f %%  pressure %.2f %%  (bias %+.1f %%)" % tuple(sb.values()))
    return s, sb
if args.evaluate_only:
    report("starting constants", current, {})
    sys.exit(0)
# ---- baseline: the current calibration, Ba fitted
if args.fixed_ba:
    base_ba = {}  # the records' burn rates are used as they are
else:
    print("baseline: fitting Ba under the current calibration ...", flush=True)
    base_ba = fit_ba("train", train_p, current, ba0)
# ---- joint fit: shared constants and the Ba of the training powders
x0 = np.array([current[n] for n in free] + ([] if args.fixed_ba else [base_ba[p.lower()] for p in train_p]))
scale = np.maximum(np.abs(x0), 0.05)
calls = 0
def unpack(x):
    cal = dict(zip(free, x[:len(free)])); ba = {} if args.fixed_ba else {p.lower(): v for p, v in zip(train_p, x[len(free):])}
    return {**current, **cal}, ba
def fun(x):
    global calls; calls += 1
    cal, ba = unpack(x); rv, rp, _ = residuals("train", cal, ba)
    r = np.concatenate([rv, rp]); print("eval %3d  rms %.5f" % (calls, np.sqrt(np.mean(r ** 2))), flush=True); return r
def jac(x):
    f0 = fun(x); h = 2e-3 * scale
    cols = list(map(lambda i: (fun(x + h[i] * np.eye(len(x))[i]) - f0) / h[i], range(len(x))))  # each evaluation is parallel inside
    return np.array(cols).T
print("joint fit of", len(free), "constants and", len(train_p), "burn rates ...", flush=True)
lo = np.full(len(x0), -np.inf); hi = np.full(len(x0), np.inf)
if args.bound_frac > 0:
    for i, n in enumerate(free):
        a, b = current[n] * (1 - args.bound_frac), current[n] * (1 + args.bound_frac)
        lo[i], hi[i] = (min(a, b), max(a, b)) if current[n] != 0 else (-args.bound_frac, args.bound_frac)
for item in filter(None, args.bounds.split(",")):
    n, r = item.split("="); a, b = r.split(":"); i = free.index(n); lo[i], hi[i] = float(a), float(b)
lo[len(free):], hi[len(free):] = 0.05, 20  # (nothing left with --fixed-ba)
sol = least_squares(fun, np.clip(x0, lo + 1e-9, hi - 1e-9), jac=jac, method="trf", x_scale=scale, loss="soft_l1", f_scale=0.1, max_nfev=12, bounds=(lo, hi))
new_cal, new_ba = unpack(sol.x)
json.dump({"calibration": {n: new_cal[n] for n in free}, "ba": new_ba}, open(args.out, "w"), indent=1)

report("current calibration (Ba refitted)", current, base_ba)
report("recalibrated", new_cal, new_ba)
def compare(cal):
    out = subprocess.run([args.compare_exe] + [a for n in free for a in ("--set", f"{n}={cal[n]:.10g}")], capture_output=True, text=True).stdout
    return [l for l in out.splitlines() if "OBJECTIVE" in l or "mean" in l.lower()][-4:]
if args.profile != "physical":
    print("\ngrt_compare against the original's reference runs:")
    print("  current     :", compare(current)); print("  recalibrated:", compare(new_cal))
