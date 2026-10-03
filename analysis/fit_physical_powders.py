#!/usr/bin/env python3
"""Fits the physical burn values (Ba, a0, z1, z2) of the project's powders for the physical model and writes data/physical/propellants.json.

    fit_physical_powders.py <grt_eval> samples.csv [samples2.csv ...] [--out FILE] [--split train|all] [--min-samples 40]
                            [--pressure-weight 1.0] [--start FILE] [--fit form|ba] [--form a0,z1,z2]

For every powder of data/propellants with at least --min-samples rows in the sample files (tools/loaddata): Ba, a0, z1 and z2 are fitted
to the velocities and, where published, the measured pressures (relative errors, robust loss) with the physical model's constants
(grt_eval --profile physical). Without pressures only Ba is fitted (the velocities hardly determine the form function) and a0, z1, z2
are the medians of the powders fitted with pressures. --fit ba fits only Ba for every powder, with the form function --form. --split train uses only the training cartridges (every cartridge but each 4th of the
sorted list, the split of calibrate_published.py) and reports the error on the held-out ones; --split all (default) fits on everything.
The file keeps the record values the burn values belong to: they apply only to the unchanged record (physicalpowders.h)."""
import argparse, csv, json, os, subprocess, tempfile
import numpy as np
from concurrent.futures import ThreadPoolExecutor
from scipy.optimize import least_squares

ap = argparse.ArgumentParser()
ap.add_argument("eval_exe"); ap.add_argument("samples", nargs="+")
ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "physical", "propellants.json"))
ap.add_argument("--split", choices=["train", "all"], default="all")
ap.add_argument("--min-samples", type=int, default=40)
ap.add_argument("--pressure-weight", type=float, default=1.0)
ap.add_argument("--start", default="", help="earlier output: start values per powder")
ap.add_argument("--fit", choices=["form", "ba"], default="form", help="form: Ba, a0, z1, z2 (powders with pressures); ba: only Ba")
ap.add_argument("--form", default="0.45,0.27,0.62", help="a0,z1,z2 for --fit ba")
args = ap.parse_args()
DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data")

# ---- samples (identical rows and misread pressures dropped, like calibrate_published.py) and the project's powder records
rows, seen = [], set()
for f in args.samples:
    for r in csv.DictReader(open(f)):
        if r.get("pressure_bar") and not 500 <= float(r["pressure_bar"]) <= 9000: r["pressure_bar"] = ""
        key = (r["powder"].upper(), r["caliber"], r["bullet_gr"], r["kind"], r["charge_g"], r["velocity_ms"], r["col_mm"])
        if key not in seen: seen.add(key); rows.append(r)
held_out = set(sorted({r["caliber"] for r in rows})[3::4])
records = {}
for f in sorted(os.listdir(os.path.join(DATA, "propellants"))):
    try: r = json.load(open(os.path.join(DATA, "propellants", f)))
    except (ValueError, OSError): continue
    records[(r["mname"] + " " + r["pname"]).lower()] = r
by_powder = {}
for r in rows:
    if r["powder"].lower() in records: by_powder.setdefault(r["powder"].lower(), []).append(r)
start = {(p["mname"] + " " + p["pname"]).lower(): p for p in json.load(open(args.start))["propellants"]} if args.start else {}
tmp = tempfile.mkdtemp()
header = list(rows[0].keys())

def simulate(name, powder, burn, subset):
    """(velocity, pressure) arrays of the rows of `subset` with the burn values `burn` (Ba, a0, z1, z2); nan where not simulated."""
    path = os.path.join(tmp, f"{name}.csv")
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, header, extrasaction="ignore"); w.writeheader(); w.writerows(subset)
    cmd = [args.eval_exe, path, "--profile", "physical", "--physical", os.path.join(tmp, "none.json"),
           "--phys", f"{powder}=" + ",".join(f"{v:.8g}" for v in burn)]
    out = subprocess.run(cmd, capture_output=True, text=True).stdout
    v = np.full(len(subset), np.nan); p = np.full(len(subset), np.nan)
    for line in out.splitlines():
        i, vv, pp = line.split(","); v[int(i)], p[int(i)] = float(vv), float(pp)
    return v, p

def residuals(name, powder, burn, subset):
    v, p = simulate(name, powder, burn, subset)
    mv = np.array([float(r["velocity_ms"]) for r in subset])
    mp = np.array([float(r["pressure_bar"]) if r["pressure_bar"] else np.nan for r in subset])
    ok = ~np.isnan(v)
    rv = np.where(ok, v / mv - 1, 0.0)
    rp = np.where(ok & ~np.isnan(mp), p / mp - 1, 0.0)
    return rv, rp, ok, ~np.isnan(mp) & ok

def unpack(x, form):
    """x = Ba [, a0, z1, d] with z2 = z1 + d (1 - z1); `form` = the fixed a0, z1, z2 when only Ba is fitted."""
    if len(x) == 1: return (x[0], *form)
    Ba, a0, z1, d = x
    return (Ba, a0, z1, z1 + d * (1 - z1))

def fit(powder, subset, x0, form):
    name = powder.replace(" ", "_").replace("/", "_")
    def fun(x):
        rv, rp, _, _ = residuals(name, powder, unpack(x, form), subset)
        return np.concatenate([rv, args.pressure_weight * rp])
    lo, hi = ([0.05, -3.0, 0.05, 0.05], [20.0, 10.0, 0.8, 0.95]) if len(x0) == 4 else ([0.05], [20.0])
    sol = least_squares(fun, np.clip(x0, np.array(lo) + 1e-6, np.array(hi) - 1e-6), bounds=(lo, hi), loss="soft_l1", f_scale=0.1,
                        diff_step=1e-3, x_scale=np.maximum(np.abs(x0), 0.1), max_nfev=60)
    return unpack(sol.x, form)

def stats(powder, burn, subset):
    if not subset: return {}
    rv, rp, ok, hasp = residuals(powder.replace(" ", "_") + "_eval", powder, burn, subset)
    s = {"n": int(ok.sum()), "v_rms": round(100 * float(np.sqrt(np.mean(rv[ok] ** 2))), 2) if ok.any() else None}
    if hasp.any(): s.update(p_n=int(hasp.sum()), p_rms=round(100 * float(np.sqrt(np.mean(rp[hasp] ** 2))), 2))
    return s

def start_of(powder):
    rec = records[powder]
    if powder in start: s = start[powder]; return [s["Ba"], s["a0"], s["z1"], (s["z2"] - s["z1"]) / (1 - s["z1"])]
    return [rec["Ba"] * 1.5, 1.0, max(rec["z1"], 0.3), 0.5]

def run(powder, with_form, form=(0.0, 0.3, 0.6)):
    data = by_powder[powder]
    train = [r for r in data if args.split == "all" or r["caliber"] not in held_out]
    test = [r for r in data if args.split == "train" and r["caliber"] in held_out]
    x0 = start_of(powder)
    burn = fit(powder, train, x0 if with_form else x0[:1], form)
    return powder, burn, stats(powder, burn, train), stats(powder, burn, test)

candidates = [p for p, rs in by_powder.items() if len(rs) >= args.min_samples]
with_p = [p for p in candidates if sum(1 for r in by_powder[p] if r["pressure_bar"]) >= 20] if args.fit == "form" else []
without_p = [p for p in candidates if p not in with_p]
print(f"{len(candidates)} powders ({len(with_p)} with pressures), held-out cartridges: {len(held_out)}", flush=True)
results = {}
with ThreadPoolExecutor(os.cpu_count()) as pool:
    for powder, burn, tr, te in pool.map(lambda p: run(p, True), with_p):
        results[powder] = (burn, tr, te); print(powder, [round(b, 4) for b in burn], tr, te, flush=True)
    form = tuple(float(np.median([results[p][0][i] for p in with_p])) for i in (1, 2, 3)) if with_p else tuple(map(float, args.form.split(",")))
    print("median form of the powders with pressures (a0, z1, z2):", [round(f, 4) for f in form], flush=True)
    for powder, burn, tr, te in pool.map(lambda p: run(p, False, form), without_p):
        results[powder] = (burn, tr, te); print(powder, [round(b, 4) for b in burn], tr, te, flush=True)

def summary(key):
    for title, ps in (("with pressures", with_p), ("velocities only", without_p)):
        n = sum(results[p][key].get("n", 0) for p in ps)
        if not n: continue
        v = np.sqrt(sum(results[p][key]["n"] * results[p][key]["v_rms"] ** 2 for p in ps if results[p][key].get("n")) / n)
        pn = sum(results[p][key].get("p_n", 0) for p in ps)
        pr = np.sqrt(sum(results[p][key]["p_n"] * results[p][key]["p_rms"] ** 2 for p in ps if results[p][key].get("p_n")) / pn) if pn else float("nan")
        print(f"   {title:16s}: {len(ps)} powders, velocity {v:.2f} % rms ({n} loads), pressure {pr:.2f} % rms ({pn} loads)")
print("\nfitted rows:"); summary(1)
if args.split == "train": print("held-out cartridges:"); summary(2)

out = {"note": "Physical burn values (Ba, a0, z1, z2) for the physical model (CalibrationProfile::Physical), fitted with "
               "analysis/fit_physical_powders.py to the powders' published loads (velocities and measured pressures); they apply only "
               "to the record values in 'record'. Generated - do not edit by hand.",
       "split": args.split,
       "propellants": []}
for powder in sorted(results):
    rec = records[powder]; burn, tr, te = results[powder]
    out["propellants"].append({"mname": rec["mname"], "pname": rec["pname"],
                               "record": {k: rec[k] for k in ("Ba", "a0", "z1", "z2")},
                               "Ba": round(burn[0], 6), "a0": round(burn[1], 6), "z1": round(burn[2], 6), "z2": round(burn[3], 6),
                               "form": "fitted" if powder in with_p else "median", "fit": tr, **({"heldOut": te} if te else {})})
os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
json.dump(out, open(args.out, "w"), indent=1)
print("written", args.out)
