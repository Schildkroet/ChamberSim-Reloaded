#!/usr/bin/env python3
"""Fits the physical burn values (Ba, a0, z1, z2) of the project's powders for the physical model and writes data/physical/propellants.json.

    fit_physical_powders.py <csr_eval> samples.csv [samples2.csv ...] [--out FILE] [--split train|all] [--min-samples 40]
                            [--pressure-weight 1.0] [--slope-weight 1.0] [--only N110,N120 --merge] [--start FILE]
                            [--fit form|ba] [--form a0,z1,z2] [--limit-pressure [--limit-margin 50]]

For every powder of data/propellants with at least --min-samples rows in the sample files (scripts/data/loaddata): Ba, a0, z1 and z2 are fitted
to the velocities and, where published, the measured pressures (relative errors, robust loss) with the physical model's constants
(csr_eval --profile physical). Without pressures only Ba is fitted (the velocities hardly determine the form function) and a0, z1, z2
are the medians of the powders fitted with pressures. --fit ba fits only Ba for every powder, with the form function --form. --split train uses only the training cartridges (every cartridge but each 4th of the
sorted list, the split of calibrate_published.py) and reports the error on the held-out ones; --split all (default) fits on everything.
Besides the errors of the single loads, the fit minimises the error of the velocity gain from the start to the maximum load of every pair of
loads (same cartridge, bullet, length and barrel; ln(v_max / v_start) simulated minus published), weighted with --slope-weight: the model must
follow how the velocity rises with the charge, not only its level. (2026-10-09: with Ba fitted per powder the model already follows
the published gain at 0.92; weights of 1-10 change nothing. A per-powder burn-law exponent and a loading-density dependent flame spread were
tried and dropped: they trade velocity against pressure and leave the elasticity of own ladders, 1.1 against the model's 0.84, as it is.)
--limit-pressure: for samples without measured pressures (Vihtavuori publishes only the C.I.P. maximum of the cartridge, column pmax_bar) the
maximum loads count as loads at that limit, so the form function is determined as well (2026-10-14: the 16 Vihtavuori rifle powders; the maximum loads
come out at 0.98 of the limit instead of 1.07 (with --limit-margin 50, i.e. the limit minus 50 bar, as in the data file: 0.98 over all 3,996 max loads of the 16 rifle powders; the 10 pistol powders 1.24 -> 0.93, velocity 12.9 -> 8.0 % rms), loads more than 20 % over it 166 instead of 670 of 3,755, velocity error 3.2 % as before).
An optional column weight makes a row count as that many rows (sources/samples/deva_samples.csv: a measured series of 10 shots has weight 10).
The file keeps the record values the burn values belong to: they apply only to the unchanged record (physicalpowders.h)."""
import argparse, csv, glob, json, os, subprocess, sys, tempfile
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
ap.add_argument("--keep-form", action="store_true", help="with --fit ba: keep each powder's a0, z1, z2 from --start (or the record)")
ap.add_argument("--only", help="comma separated powders (product name or \"maker name\"): fit only these")
ap.add_argument("--slope-weight", type=float, default=1.0, help="weight of the start-to-maximum velocity gain of the load pairs (0: off)")
ap.add_argument("--merge", action="store_true", help="keep the other powders of the existing --out file (with --only: refit some, e.g. new records)")
ap.add_argument("--limit-pressure", action="store_true",
                help="rows without a measured pressure: the maximum loads get the caliber's C.I.P. limit (column pmax_bar) as pressure (Vihtavuori)")
ap.add_argument("--limit-margin", type=float, default=0.0, help="with --limit-pressure: the maximum loads count as this many bar below the limit")
ap.add_argument("--eval-arg", action="append", default=[], help="extra option for csr_eval (repeatable), e.g. --eval-arg=--ps-handgun=150")
args = ap.parse_args()
DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data")

# ---- samples (identical rows and misread pressures dropped, like calibrate_published.py) and the project's powder records
records = {}
sys.path.insert(0, os.path.join(DATA, "..", "scripts", "data"))
import recordfiles  # noqa: E402  data/propellants/<manufacturer>.json
for _, r in recordfiles.read(os.path.join(DATA, "propellants")):
    records[(r["mname"] + " " + r["pname"]).lower()] = r
# a powder column without the maker ("N150" of vv_extract.py) is matched by the product name when that is unique among the records
by_name = {}
for k, r in records.items(): by_name.setdefault(r["pname"].lower(), []).append(r["mname"] + " " + r["pname"])
rows, seen = [], set()
for f in args.samples:
    for r in csv.DictReader(open(f)):
        r.setdefault("pressure_bar", "")  # files without pressures (Vihtavuori)
        if args.limit_pressure and not r["pressure_bar"] and r["kind"] == "max" and r.get("pmax_bar"): r["pressure_bar"] = str(float(r["pmax_bar"]) - args.limit_margin)
        if r["powder"].lower() not in records and len(by_name.get(r["powder"].lower(), [])) == 1: r["powder"] = by_name[r["powder"].lower()][0]
        if r.get("pressure_bar") and not 500 <= float(r["pressure_bar"]) <= 9000: r["pressure_bar"] = ""
        key = (r["powder"].upper(), r["caliber"], r["bullet_gr"], r["kind"], r["charge_g"], r["velocity_ms"], r["col_mm"])
        if key not in seen: seen.add(key); rows.append(r)
held_out = set(sorted({r["caliber"] for r in rows})[3::4])
by_powder = {}
for r in rows:
    if r["powder"].lower() in records: by_powder.setdefault(r["powder"].lower(), []).append(r)
start = {(p["mname"] + " " + p["pname"]).lower(): p for p in json.load(open(args.start))["propellants"]} if args.start else {}
# The grain geometry of a powder is independent of the Ba fit: csr_eval takes it from this file when --phys replaces Ba, a0, z1, z2.
PHYS_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "physical", "propellants.json")
# rifle / pistol class of a caliber (C.I.P. table IV is pistol and revolver)
CAL_CLASS = {}
for _f in glob.glob(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "calibers", "*.json")):
    _r = json.load(open(_f))
    _pistol = "/tab-iv/" in _r.get("cippdf", "") or "pistol" in _r.get("ciptype", "").lower()
    CAL_CLASS[_r.get("cipname", "").lower()] = "pistol" if _pistol else "rifle"
tmp = tempfile.mkdtemp()
header = list(dict.fromkeys(k for r in rows for k in r))

def simulate(name, powder, burn, subset):
    """(velocity, pressure) arrays of the rows of `subset` with the burn values `burn` (Ba, a0, z1, z2); nan where not simulated."""
    path = os.path.join(tmp, f"{name.replace('/', '_')}.csv")
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, header, extrasaction="ignore"); w.writeheader(); w.writerows(subset)
    cmd = [args.eval_exe, path, "--profile", "physical", "--physical", PHYS_FILE,
           "--phys", f"{powder}=" + ",".join(f"{v:.8g}" for v in burn)] + args.eval_arg
    out = subprocess.run(cmd, capture_output=True, text=True).stdout
    v = np.full(len(subset), np.nan); p = np.full(len(subset), np.nan)
    for line in out.splitlines():
        i, vv, pp = line.split(","); v[int(i)], p[int(i)] = float(vv), float(pp)
    return v, p

def pairs_of(subset):
    """Index pairs (start load, maximum load) of the subset: the same cartridge, bullet, length and barrel."""
    groups = {}
    for i, r in enumerate(subset):
        groups.setdefault((r["caliber"], r["bullet"], r["bullet_gr"], r["col_mm"], r["barrel_mm"]), {})[r["kind"]] = i
    return [(g["start"], g["max"]) for g in groups.values() if "start" in g and "max" in g
            and float(subset[g["max"]]["charge_g"]) > float(subset[g["start"]]["charge_g"]) * 1.02
            and float(subset[g["max"]]["velocity_ms"]) > float(subset[g["start"]]["velocity_ms"])]

def residuals(name, powder, burn, subset):
    v, p = simulate(name, powder, burn, subset)
    mv = np.array([float(r["velocity_ms"]) for r in subset])
    mp = np.array([float(r["pressure_bar"]) if r["pressure_bar"] else np.nan for r in subset])
    ok = ~np.isnan(v)
    rv = np.where(ok, v / mv - 1, 0.0)
    rp = np.where(ok & ~np.isnan(mp), p / mp - 1, 0.0)
    return rv, rp, ok, ~np.isnan(mp) & ok, v, mv

def gains(subset, v, mv):
    """(simulated, published) ln(v_max / v_start) of the pairs of the subset where both loads were simulated."""
    pr = [(a, b) for a, b in pairs_of(subset) if not (np.isnan(v[a]) or np.isnan(v[b]))]
    return (np.array([np.log(v[b] / v[a]) for a, b in pr]), np.array([np.log(mv[b] / mv[a]) for a, b in pr]))

def unpack(x, form):
    """x = Ba [, a0, z1, d] with z2 = z1 + d (1 - z1); `form` = the fixed a0, z1, z2 when only Ba is fitted."""
    if len(x) == 1: return (x[0], *form)
    Ba, a0, z1, d = x
    return (Ba, a0, z1, z1 + d * (1 - z1))

def fit(powder, subset, x0, form):
    name = powder.replace(" ", "_").replace("/", "_")
    w = np.sqrt([float(r.get("weight") or 1) for r in subset])  # column weight: a row counts as that many rows (own measured series)
    def fun(x):
        rv, rp, _, _, v, mv = residuals(name, powder, unpack(x, form), subset)
        parts = [w * rv, args.pressure_weight * w * rp]
        if args.slope_weight > 0:
            sg, pg = gains(subset, v, mv)
            # fixed length: a pair that fails to simulate at some trial values must not change the residual vector's size
            full = np.zeros(len(pairs_of(subset)))
            full[:len(sg)] = sg - pg
            parts.append(args.slope_weight * full)
        return np.concatenate(parts)
    lo, hi = ([0.05, -3.0, 0.05, 0.05], [20.0, 10.0, 0.8, 0.95]) if len(x0) == 4 else ([0.05], [20.0])
    sol = least_squares(fun, np.clip(x0, np.array(lo) + 1e-6, np.array(hi) - 1e-6), bounds=(lo, hi), loss="soft_l1", f_scale=0.1,
                        diff_step=1e-3, x_scale=np.maximum(np.abs(x0), 0.1), max_nfev=60)
    print(f"cost {powder}: {sol.cost:.6g}", flush=True)  # compare fits of one powder from different start values
    return unpack(sol.x, form)

def stats(powder, burn, subset):
    if not subset: return {}
    rv, rp, ok, hasp, v, mv = residuals(powder.replace(" ", "_") + "_eval", powder, burn, subset)
    s = {"n": int(ok.sum()), "v_rms": round(100 * float(np.sqrt(np.mean(rv[ok] ** 2))), 2) if ok.any() else None}
    if hasp.any(): s.update(p_n=int(hasp.sum()), p_rms=round(100 * float(np.sqrt(np.mean(rp[hasp] ** 2))), 2))
    sg, pg = gains(subset, v, mv)
    big = pg > 0.03
    if big.any(): s.update(gain_n=int(big.sum()), gain=round(float(np.median(sg[big] / pg[big])), 3))  # 1: the model follows the data
    classes = {}
    for cls in ("rifle", "pistol"):
        mask = np.array([CAL_CLASS.get(r["caliber"].lower(), "rifle") == cls for r in subset], dtype=bool)
        if not mask.any(): continue
        m = ok & mask
        c = {"n": int(m.sum())}
        if m.any(): c["v_rms"] = round(100 * float(np.sqrt(np.mean(rv[m] ** 2))), 2)
        mp = hasp & mask
        if mp.any(): c.update(p_n=int(mp.sum()), p_rms=round(100 * float(np.sqrt(np.mean(rp[mp] ** 2))), 2))
        classes[cls] = c
    if classes: s["classes"] = classes
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
    if not with_form and args.keep_form:
        form = (x0[1], x0[2], x0[2] + x0[3] * (1 - x0[2]))
    burn = fit(powder, train, x0 if with_form else x0[:1], form)
    return powder, burn, stats(powder, burn, train), stats(powder, burn, test)

only = {o.strip().lower() for o in args.only.split(",")} if args.only else None
candidates = [p for p, rs in by_powder.items() if len(rs) >= args.min_samples and (only is None or p in only or records[p]["pname"].lower() in only)]
with_p = [p for p in candidates if sum(1 for r in by_powder[p] if r["pressure_bar"]) >= 20] if args.fit == "form" else []
without_p = [p for p in candidates if p not in with_p]
print(f"{len(candidates)} powders ({len(with_p)} with pressures), held-out cartridges: {len(held_out)}", flush=True)
results = {}
with ThreadPoolExecutor(int(os.environ.get("CSR_JOBS", min(14, os.cpu_count() or 1)))) as pool:
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
        gn = sum(results[p][key].get("gain_n", 0) for p in ps)
        gr = sum(results[p][key]["gain_n"] * results[p][key]["gain"] for p in ps if results[p][key].get("gain_n")) / gn if gn else float("nan")
        print(f"   {title:16s}: {len(ps)} powders, velocity {v:.2f} % rms ({n} loads), pressure {pr:.2f} % rms ({pn} loads), "
              f"velocity gain start -> max {gr:.3f} of the published ({gn} pairs)")
        for cls in ("rifle", "pistol"):
            cn = sum(results[p][key].get("classes", {}).get(cls, {}).get("n", 0) for p in ps)
            if not cn: continue
            cv = np.sqrt(sum(results[p][key]["classes"][cls]["n"] * results[p][key]["classes"][cls]["v_rms"] ** 2
                             for p in ps if results[p][key].get("classes", {}).get(cls, {}).get("n")) / cn)
            cpn = sum(results[p][key].get("classes", {}).get(cls, {}).get("p_n", 0) for p in ps)
            cpr = np.sqrt(sum(results[p][key]["classes"][cls]["p_n"] * results[p][key]["classes"][cls]["p_rms"] ** 2
                              for p in ps if results[p][key].get("classes", {}).get(cls, {}).get("p_n")) / cpn) if cpn else float("nan")
            print(f"     {cls:8s}: velocity {cv:.2f} % rms ({cn} loads), pressure {cpr:.2f} % rms ({cpn} loads)")
print("\nfitted rows:"); summary(1)
if args.split == "train": print("held-out cartridges:"); summary(2)

out = {"note": "Physical burn values (Ba, a0, z1, z2, optional effective grain geometry) for the physical model "
               "(CalibrationProfile::Physical), fitted with analysis/fit_physical_powders.py and analysis/fit_grain_geometry.py to the "
               "powders' published loads (velocities and measured pressures); they apply only to the record values in 'record'. "
               "Generated - do not edit by hand.",
       "split": args.split,
       "propellants": []}
old_grain = {}
if os.path.exists(args.out):
    try:
        old_grain = {(e["mname"] + " " + e["pname"]).lower(): e["grain"]
                     for e in json.load(open(args.out))["propellants"] if "grain" in e}
    except (OSError, ValueError, KeyError):
        old_grain = {}
for powder in sorted(results):
    rec = records[powder]; burn, tr, te = results[powder]
    entry = {"mname": rec["mname"], "pname": rec["pname"],
             "record": {k: rec[k] for k in ("Ba", "a0", "z1", "z2")},
             "Ba": round(burn[0], 6), "a0": round(burn[1], 6), "z1": round(burn[2], 6), "z2": round(burn[3], 6),
             "form": "kept" if args.fit == "ba" and args.keep_form else ("fitted" if powder in with_p else "median"),
             "fit": tr, **({"heldOut": te} if te else {})}
    key = (rec["mname"] + " " + rec["pname"]).lower()
    if key in old_grain: entry["grain"] = old_grain[key]  # keep the effective grain geometry (fit_grain_geometry.py)
    out["propellants"].append(entry)
if args.merge and os.path.exists(args.out):
    kept = {(p["mname"] + " " + p["pname"]).lower(): p for p in json.load(open(args.out))["propellants"]}
    kept.update({(p["mname"] + " " + p["pname"]).lower(): p for p in out["propellants"]})
    out["propellants"] = [kept[k] for k in sorted(kept)]
os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
json.dump(out, open(args.out, "w"), indent=1)
print("written", args.out)
