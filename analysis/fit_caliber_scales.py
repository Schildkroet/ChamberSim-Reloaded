#!/usr/bin/env python3
"""Fits the per-caliber factor on the physical burn rates of pistol and revolver cartridges (with --rifles also of rifle cartridges) and
writes data/physical/calibers.json.

    fit_caliber_scales.py <csr_eval> samples.csv [samples2.csv ...] [--out FILE] [--min-samples 100] [--holdout] [--rifles] [--keep-existing]

--rifles: rifle cartridges get a factor as well (2026-10-19: calibers such as .30 Super Carry, .327 Federal, 300 Norma, .33 Nosler, 338 Lapua
run 5 - 8 % too slow, 45-70, 9.3 x 62, 257 Weatherby 5 - 8 % too fast; fitted on half of the powders, the velocity rms of the other half
falls 3.79 -> 3.29 % and the pressure rms 13.1 -> 12.6 %; the same factor on the case volume instead did not help the pressure, so it is not a
volume error). --keep-existing: entries already in the file stay as they are, only the other calibers are fitted.

The published loads of the pistol and revolver cartridges (C.I.P. table IV) are not met by one burn rate per powder: auto-loading cartridges
(small rim) come out too slow, revolver cartridges (rimmed) a little too fast. For every caliber with at least --min-samples loads the factor
that minimises the velocity error (robust, burn rates of the powders as in data/physical/propellants.json) is fitted, and for the calibers
without an entry the factor of their class (auto: rim / head diameter below 1.045, rimmed: above), fitted on the loads of the class.
--holdout fits on half of the powders and reports the error on the other half (the factors are not written).
Loads that disagree with their own table (sample_filter.py) are left out."""
import argparse, csv, glob, json, os, random, subprocess, sys, tempfile
import numpy as np
from concurrent.futures import ThreadPoolExecutor
from scipy.optimize import minimize_scalar

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sample_filter import inconsistent

ap = argparse.ArgumentParser()
ap.add_argument("eval_exe"); ap.add_argument("samples", nargs="+")
ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "physical", "calibers.json"))
ap.add_argument("--min-samples", type=int, default=100)
ap.add_argument("--rimmed-from", type=float, default=1.045)
ap.add_argument("--holdout", action="store_true")
ap.add_argument("--rifles", action="store_true", help="fit rifle cartridges too")
ap.add_argument("--keep-existing", action="store_true", help="keep the entries of the existing file, fit only the other calibers")
ap.add_argument("--shrink", type=float, default=0.75, help="part of the fitted deviation from 1 that is kept (the factors of half the powders do not transfer fully)")
ap.add_argument("--class-fallback", action="store_true", help="also give calibers without an entry the factor of their class (on held-out powders it made auto cartridges without data worse: 5.6 -> 9.2 %%)")
ap.add_argument("--jobs", type=int, default=int(os.environ.get("CSR_JOBS", min(14, os.cpu_count() or 1))))
args = ap.parse_args()
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
case = {}
for f in glob.glob(os.path.join(ROOT, "data", "calibers", "*.json")):
    r = json.load(open(f)); case[r["cipname"]] = r
def pistol(c): return c in case and ("/tab-iv/" in case[c].get("cippdf", "") or (args.rifles and "shot" not in case[c].get("cippdf", "")))
def rim(c): r = case[c]; return r["R1"] / r["P1"] if r.get("P1") else 0.0

rows, seen = [], set()
for f in args.samples:
    for r in csv.DictReader(open(f)):
        key = (r["powder"].upper(), r["caliber"], r["bullet_gr"], r["kind"], r["charge_g"], r["velocity_ms"], r["col_mm"])
        if key not in seen and pistol(r["caliber"]): seen.add(key); rows.append(r)
gone = inconsistent(rows); rows = [r for i, r in enumerate(rows) if i not in gone]
print(f"{len(rows)} pistol loads ({len(gone)} inconsistent ones dropped)")
header = sorted(set().union(*[set(r) for r in rows]))

def errors(sub, scale):
    """ln(simulated / published) velocity of every load of `sub` for the burn rate factor (NaN: not simulated)."""
    with tempfile.NamedTemporaryFile("w", suffix=".csv", newline="", delete=False) as f:
        w = csv.DictWriter(f, header + ["ba_scale"], restval=""); w.writeheader()
        w.writerows(dict(r, ba_scale=scale) for r in sub)
    out = subprocess.run([args.eval_exe, f.name, "--profile", "physical", "--set", "caliberBurnScaling=0"], capture_output=True, text=True).stdout
    os.unlink(f.name)
    e = np.full(len(sub), np.nan)
    for l in out.splitlines():
        a = l.split(","); e[int(a[0])] = np.log(float(a[1]) / float(sub[int(a[0])]["velocity_ms"]))
    return e

def fit(sub):
    cost = lambda s: float(np.nanmean(np.minimum(errors(sub, s) ** 2, 0.04)))
    return float(minimize_scalar(cost, bounds=(0.7, 1.5), method="bounded", options={"xatol": 0.005}).x)

def rms(e): e = e[~np.isnan(e)]; return 100 * float(np.sqrt(np.mean(e ** 2))) if len(e) else float("nan")

if args.holdout:
    random.seed(5); powders = sorted({r["powder"] for r in rows}); random.shuffle(powders); half = set(powders[::2])
    train = [r for r in rows if r["powder"] in half]; test = [r for r in rows if r["powder"] not in half]
else:
    train = test = rows
byc = {}
for r in train: byc.setdefault(r["caliber"], []).append(r)
existing = {}
if args.keep_existing and os.path.exists(args.out):
    existing = {e["cipname"]: e for e in json.load(open(args.out))["calibers"]}
names = [c for c, v in byc.items() if len(v) >= args.min_samples and c not in existing]
with ThreadPoolExecutor(args.jobs) as pool:
    scales = {c: 1.0 + args.shrink * (v - 1.0) for c, v in zip(names, pool.map(lambda c: fit(byc[c]), names))}
cls = {"auto": lambda c: rim(c) < args.rimmed_from, "rimmed": lambda c: rim(c) >= args.rimmed_from}
class_scale = {k: (1.0 + args.shrink * (fit([r for r in train if sel(r["caliber"])]) - 1.0) if args.class_fallback else 1.0) for k, sel in cls.items()}
print("class factors", {k: round(v, 3) for k, v in class_scale.items()})
entries, before, after = list(existing.values()), [], []
scales.update({c: e["burnScale"] for c, e in existing.items()})
for c in sorted(scales):
    if c in existing: continue
    sub = [r for r in test if r["caliber"] == c]; e0, e1 = errors(sub, 1.0), errors(sub, scales[c])
    entries.append({"cipname": c, "burnScale": round(scales[c], 4), "n": len(byc[c]), "v_rms_before": round(rms(e0), 2), "v_rms_after": round(rms(e1), 2)})
    print(f"  {c:20} x{scales[c]:.3f}  n={len(sub):5}  velocity rms {rms(e0):5.2f} -> {rms(e1):5.2f} %")
for k, sel in cls.items():
    sub = [r for r in test if sel(r["caliber"]) and r["caliber"] not in scales]
    if sub: print(f"  class {k} without entry: n={len(sub)}  velocity rms {rms(errors(sub, 1.0)):.2f} -> {rms(errors(sub, class_scale[k])):.2f} %")
allt = [r for r in test]; e0 = errors(allt, 1.0)
e1 = np.concatenate([errors([r for r in allt if r["caliber"] == c], scales[c] if c in scales else class_scale["auto" if cls["auto"](c) else "rimmed"]) for c in sorted({r["caliber"] for r in allt})])
print(f"all pistol loads: velocity rms {rms(e0):.2f} -> {rms(e1):.2f} %" + ("  (held-out powders)" if args.holdout else "  (fitted data)"))
if not args.holdout:
    json.dump({"note": "Factors on the physical burn rates of cartridges with at least 60 (pistol and revolver: 100) published loads, fitted with analysis/fit_caliber_scales.py to the published loads (velocities). Generated - do not edit by hand.",
               "autoScale": round(class_scale["auto"], 4), "rimmedScale": round(class_scale["rimmed"], 4), "rimmedFrom": args.rimmed_from,
               "calibers": sorted(entries, key=lambda e: e["cipname"])}, open(args.out, "w"), indent=1)
    print("written", args.out)
