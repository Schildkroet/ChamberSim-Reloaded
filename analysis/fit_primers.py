#!/usr/bin/env python3
"""Fits the start pressure of every primer type (data/physical/primers.json, src/core/primer.h) to published loads that name their primer.

    fit_primers.py <csr_eval> samples.csv [samples2.csv ...] [--anchor LR=250] [--size none|volume] [--split train|all] [--write]

A load's start pressure is the start pressure of its primer type plus the bullet's additional start pressure (0 here: the rows have no
gpressure). The primer text of a row (column primer, e.g. Hodgdon's "WIN LR Magnum") gives the type (scripts/data/primers.py); rows without
one are left out. The powders keep their physical burn values. --anchor holds one type fixed (Large Rifle at the 250 bar the powders were
fitted with, so their calibration stays valid); the others are fitted relative to it. --size volume adds a dependence on the case:
start pressure = type value * (V / 3 cm3)^-k with one exponent k for all types (a primer's gas fills a small case to a higher pressure).
Residuals: velocity / 3 % and pressure / 13 % (the scatter of the tables), soft-L1. --split train fits on every cartridge but each 4th of
the sorted list and reports the held-out ones; the result is printed per class (rifle; pistol and revolver = C.I.P. table IV).
"""
import argparse, csv, glob, json, os, subprocess, sys, tempfile
import numpy as np
from concurrent.futures import ThreadPoolExecutor
from scipy.optimize import least_squares

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
sys.path.insert(0, os.path.join(ROOT, "scripts", "data"))
from primers import TYPES, classify  # noqa: E402

ap = argparse.ArgumentParser()
ap.add_argument("eval_exe"); ap.add_argument("samples", nargs="+")
ap.add_argument("--anchor", default="LR=250", help="type=bar held fixed")
ap.add_argument("--size", choices=["none", "volume"], default="none")
ap.add_argument("--split", choices=["train", "all"], default="all")
ap.add_argument("--min-loads", type=int, default=50, help="types with fewer loads keep the anchor's value")
ap.add_argument("--jobs", type=int, default=int(os.environ.get("CSR_JOBS", min(14, os.cpu_count() or 1))))
ap.add_argument("--write", action="store_true", help="write data/physical/primers.json")
ap.add_argument("--out", default=os.path.join(ROOT, "data", "physical", "primers.json"))
args = ap.parse_args()

cls = {}
for f in glob.glob(os.path.join(ROOT, "data", "calibers", "*.json")):
    r = json.load(open(f))
    cls[r["cipname"]] = "pistol" if "/tab-iv/" in r.get("cippdf", "") or r.get("ciptype", "").startswith("4") else "rifle"
rows, seen = [], set()
for f in args.samples:
    for r in csv.DictReader(open(f, encoding="utf-8")):
        t = classify(r.get("primer", ""))
        if not t: continue
        if r.get("pressure_bar") and not 500 <= float(r["pressure_bar"]) <= 9000: r["pressure_bar"] = ""
        key = (r["powder"], r["caliber"], r["bullet"], r["bullet_gr"], r["kind"], r["charge_g"], r["velocity_ms"], r["col_mm"])
        if key in seen: continue
        seen.add(key); r["primer_type"] = t; rows.append(r)
held = set(sorted({r["caliber"] for r in rows})[3::4])
train = [r for r in rows if args.split == "all" or r["caliber"] not in held]
test = [r for r in rows if args.split == "train" and r["caliber"] in held]
anchor_type, anchor_bar = args.anchor.split("=")
anchor_bar = float(anchor_bar)
count = {t: sum(1 for r in train if r["primer_type"] == t) for t in TYPES}
free = [t for t in TYPES if t != anchor_type and count[t] >= args.min_loads]
print(f"{len(rows)} loads with a primer ({len(train)} fitted, {len(test)} held out); loads per type: {count}; fitted types: {free}", flush=True)

tmp = tempfile.mkdtemp()
header = list(dict.fromkeys(k for r in rows for k in r))


def chunks(subset, name):
    """The rows split into --jobs CSV files (written once)."""
    parts = []
    for i in range(args.jobs):
        part = subset[i::args.jobs]
        path = os.path.join(tmp, f"{name}_{i}.csv")
        with open(path, "w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, header, extrasaction="ignore"); w.writeheader(); w.writerows(part)
        parts.append((path, part))
    return parts


def primers_file(values, k, tag):
    path = os.path.join(tmp, f"primers_{tag}.json")
    json.dump({"primers": values, "volumeExponent": k, "referenceVolume_cm3": 3.0}, open(path, "w"))
    return path


def simulate(parts, values, k, tag):
    pf = primers_file(values, k, tag)

    def one(i):
        path, part = parts[i]
        out = subprocess.run([args.eval_exe, path, "--profile", "physical", "--primers", pf], capture_output=True, text=True).stdout
        v = np.full(len(part), np.nan); p = np.full(len(part), np.nan)
        for line in out.splitlines():
            j, vv, pp = line.split(","); v[int(j)], p[int(j)] = float(vv), float(pp)
        return part, v, p

    with ThreadPoolExecutor(args.jobs) as pool:
        res = list(pool.map(one, range(len(parts))))
    sub = [r for part, _, _ in res for r in part]
    return sub, np.concatenate([v for _, v, _ in res]), np.concatenate([p for _, _, p in res])


def values_of(x):
    values = {t: anchor_bar for t in TYPES}
    for t, v in zip(free, x[:len(free)]): values[t] = float(v)
    return values, (float(x[len(free)]) if args.size == "volume" else 0.0)


def residuals(sub, v, p):
    mv = np.array([float(r["velocity_ms"]) for r in sub])
    mp = np.array([float(r["pressure_bar"]) if r["pressure_bar"] else np.nan for r in sub])
    rv = np.where(np.isnan(v), 0.0, v / mv - 1)
    rp = np.where(np.isnan(v) | np.isnan(mp), 0.0, p / mp - 1)
    return rv, rp


train_parts = chunks(train, "train")
calls = [0]


def fun(x):
    calls[0] += 1
    values, k = values_of(x)
    sub, v, p = simulate(train_parts, values, k, f"c{calls[0] % 64}")
    rv, rp = residuals(sub, v, p)
    return np.concatenate([rv / 0.03, rp / 0.13])


def report(title, parts, values, k):
    sub, v, p = simulate(parts, values, k, "report")
    rv, rp = residuals(sub, v, p)
    ok = ~np.isnan(v)
    hasp = ok & np.array([bool(r["pressure_bar"]) for r in sub])
    for c in ("rifle", "pistol"):
        m = ok & np.array([cls.get(r["caliber"], "rifle") == c for r in sub]); mp = hasp & m
        if not m.any(): continue
        print(f"  {title:28s} {c:6s}: velocity {100 * np.sqrt(np.mean(rv[m] ** 2)):.2f} % rms ({m.sum()}), "
              f"pressure {100 * np.sqrt(np.mean(rp[mp] ** 2)):.2f} % rms, mean {100 * np.mean(rp[mp]):+.2f} % ({mp.sum()})", flush=True)
    for t in TYPES:
        mt = hasp & np.array([r["primer_type"] == t for r in sub])
        if mt.sum() >= 20:
            print(f"      {t:4s} pressure mean {100 * np.mean(rp[mt]):+.2f} %, velocity mean {100 * np.mean(rv[ok & np.array([r['primer_type'] == t for r in sub])]):+.2f} % "
                  f"({mt.sum()})", flush=True)


base = {t: anchor_bar for t in TYPES}
report("all types at the anchor", train_parts, base, 0.0)
x0 = [anchor_bar] * len(free) + ([0.0] if args.size == "volume" else [])
lo = [50.0] * len(free) + ([-2.0] if args.size == "volume" else [])
hi = [800.0] * len(free) + ([2.0] if args.size == "volume" else [])
sol = least_squares(fun, x0, bounds=(lo, hi), loss="soft_l1", f_scale=1.0, diff_step=0.02, x_scale=[100.0] * len(free) + ([0.2] if args.size == "volume" else []), max_nfev=40)
values, k = values_of(sol.x)
print("fitted:", {t: round(v) for t, v in values.items()}, "volume exponent", round(k, 3), "cost", round(sol.cost, 2), flush=True)
report("fitted", train_parts, values, k)
if test:
    test_parts = chunks(test, "test")
    print("held-out cartridges:")
    report("all types at the anchor", test_parts, base, 0.0)
    report("fitted", test_parts, values, k)
if args.write:
    out = {"note": "Start pressure [bar] of the primer types (src/core/primer.h), fitted with analysis/fit_primers.py to published loads that name their "
                   "primer (velocities and pressures, physical model, powders' burn values kept). A load's start pressure is this plus its additional "
                   f"start pressure. Anchor {args.anchor}. Generated - do not edit by hand.",
           "primers": {t: round(v, 1) for t, v in values.items()}, "volumeExponent": round(k, 4), "referenceVolume_cm3": 3.0}
    json.dump(out, open(args.out, "w"), indent=1)
    print("written", args.out)
