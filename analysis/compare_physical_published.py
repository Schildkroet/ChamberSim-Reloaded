#!/usr/bin/env python3
"""Accuracy of the project's shipped powder records against published loads, for one build or two builds side by side (before / after a change).

    compare_physical_published.py <csr_eval> [--baseline <csr_eval of the older build>] [--profile physical] [--min-samples 60] [--top 8]
                                  [--drop-inconsistent 0.15] [samples.csv ...]

Every published load (scripts/data/loaddata/sources/samples/by_maker, made by by_maker.py: Hodgdon, Lovex, Reload Swiss, ADI, Vihtavuori, ... default: all that exist) is simulated
with the powder record of the build's own data (csr_eval reads the data of the tree it was built from; --profile physical = the default model, the
physical burn values are NOT refitted here) and compared with the published velocity and peak pressure. Reported per class (rifle, pistol and
revolver = C.I.P. table IV) and publisher: velocity / pressure rms and mean error of all loads and of the held-out cartridges (every 4th cartridge
of the sorted list, like analysis/scan_physical.py; powders fitted on the other cartridges). With --baseline the second build is listed next to
the first and the difference is printed. Shotgun loads are not part of the rifle/pistol sample files.

To get a baseline build of an older commit without touching the working tree:
    git archive <commit> | tar -x -C /tmp/base && cmake -S /tmp/base -B /tmp/base/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCSR_FORMAT_ON_BUILD=OFF && ninja -C /tmp/base/build csr_eval
"""
import argparse, csv, glob, json, os, subprocess, sys, tempfile
import numpy as np
from concurrent.futures import ThreadPoolExecutor

ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
ap.add_argument("eval_exe"); ap.add_argument("samples", nargs="*")
ap.add_argument("--baseline", help="csr_eval of the older build to compare with")
ap.add_argument("--profile", default="physical")
ap.add_argument("--eval-arg", action="append", default=[], help="extra option for the csr_eval of the first build (repeatable), e.g. --eval-arg=--physical=other.json")
ap.add_argument("--baseline-projectiles", default="", help="projectile records for the baseline run (default: --projectiles; \"none\": the solver's defaults)")
ap.add_argument("--baseline-arg", action="append", default=[], help="extra option for the baseline csr_eval; the baseline may be the same executable with other options")
ap.add_argument("--min-samples", type=int, default=60)
ap.add_argument("--drop-inconsistent", type=float, default=0.15)
ap.add_argument("--jobs", type=int, default=min(14, os.cpu_count() or 1))
ap.add_argument("--max-pressure-dev", type=float, default=0.0, help="leave out loads whose simulated pressure is more than this far (percent) off the published one: probably invalid data")
ap.add_argument("--exclude-by", choices=["baseline", "both", "either", "current"], default="baseline", help="which build decides (the same loads are left out of both)")
ap.add_argument("--projectiles", default="", help="directory of projectile record files (data/projectiles): every load gets the start pressure gpressure and the extra friction "
                "gfriction of the bullet record it matches, as in the program (default: none, the solver's default start pressure of 250 bar and no extra friction)")
ap.add_argument("--worst", type=int, default=0, help="also list the loads whose pressure changed most between the builds")
args = ap.parse_args()
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
SDIR = os.path.join(ROOT, "scripts", "data", "loaddata", "sources", "samples", "by_maker")  # by_maker.py: one file per powder maker, loads once
files = args.samples or sorted(f for f in glob.glob(os.path.join(SDIR, "*.csv")) if "shot" not in os.path.basename(f) and "vectan_ocr" not in f)
cal = {}
for f in glob.glob(os.path.join(ROOT, "data", "calibers", "*.json")):
    r = json.load(open(f)); cal[r["cipname"]] = r
names = {}
for f in glob.glob(os.path.join(ROOT, "data", "propellants", "*.json")):
    for e in json.load(open(f)) if isinstance(json.load(open(f)), list) else []:
        names.setdefault(e["pname"].lower(), []).append(e["mname"] + " " + e["pname"])
rows, seen = [], set()
for f in files:
    for r in csv.DictReader(open(f)):
        r["_src"] = os.path.basename(f).split("_")[0]
        full = r["powder"]
        if full.lower() not in {x.lower() for v in names.values() for x in v} and len(names.get(full.lower(), [])) == 1: r["powder"] = names[full.lower()][0]
        r.setdefault("pressure_bar", "")
        if r["pressure_bar"] and not 500 <= float(r["pressure_bar"]) <= 9000: r["pressure_bar"] = ""
        key = (r["powder"].upper(), r["caliber"], r["bullet_gr"], r["kind"], r["charge_g"], r["velocity_ms"], r["col_mm"])
        if key not in seen: seen.add(key); rows.append(r)
if args.drop_inconsistent > 0:
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from sample_filter import inconsistent
    gone = inconsistent(rows, args.drop_inconsistent)
    rows = [r for i, r in enumerate(rows) if i not in gone]
def bullet_columns(directory):
    """(gpressure, gfriction) per row from the bullet records in `directory` (empty strings without a directory or a matching record)."""
    if not directory: return [("", "")] * len(rows)
    import re
    sys.path.insert(0, os.path.join(ROOT, "scripts", "data"))
    import recordfiles
    recs = [r for _, r in recordfiles.read(directory)]
    def match(label, gr, dia):  # the record of the load's bullet (maker prefix, mass, diameter; best name overlap), like fit_bullet_start_values.py
        low = label.lower()
        c = [p for p in recs if low.startswith(p["mname"].lower()) and abs(p["gmass"] - gr) < 0.8 and abs(p["gdia"] - dia) < 0.06]
        return max(c, key=lambda p: sum(w in p["pname"].lower() for w in re.findall(r"[a-z0-9]+", low))) if c else None
    out = []
    for r in rows:
        rec = match(r["bullet"], float(r["bullet_gr"]), float(r["bullet_dia_mm"]))
        out.append((str(rec.get("gpressure") or "") if rec else "", str(rec.get("gfriction") or "") if rec else ""))
    return out


held = set(sorted({r["caliber"] for r in rows})[3::4])
count = {}
for r in rows: count[r["powder"]] = count.get(r["powder"], 0) + 1
rows = [r for r in rows if count[r["powder"]] >= args.min_samples]
tmp = tempfile.mkdtemp(); header = [h for h in rows[0].keys() if h != "_src"] + ["gpressure", "gfriction"]
by = {}
for i, r in enumerate(rows): by.setdefault(r["powder"], []).append(i)


def simulate(exe, extra, columns):
    v = np.full(len(rows), np.nan); p = np.full(len(rows), np.nan)
    def one(name):
        path = os.path.join(tmp, f"{os.getpid()}_{abs(hash((exe, name)))}.csv"); idx = by[name]
        with open(path, "w", newline="") as f:
            w = csv.DictWriter(f, header, extrasaction="ignore"); w.writeheader(); w.writerows(dict(rows[i], gpressure=columns[i][0], gfriction=columns[i][1]) for i in idx)
        out = subprocess.run([exe, path, "--profile", args.profile] + extra, capture_output=True, text=True).stdout
        for l in out.splitlines():
            a = l.split(",")
            if len(a) == 3: v[idx[int(a[0])]], p[idx[int(a[0])]] = float(a[1]), float(a[2])
    with ThreadPoolExecutor(args.jobs) as pool: list(pool.map(one, by))
    return v, p


mv = np.array([float(r["velocity_ms"]) for r in rows]); mp = np.array([float(r["pressure_bar"]) if r["pressure_bar"] else np.nan for r in rows])
cls = np.array(["pistol" if "/tab-iv/" in cal.get(r["caliber"], {}).get("cippdf", "") else "rifle" for r in rows])
src = np.array([r["_src"] for r in rows]); test = np.array([r["caliber"] in held for r in rows])
runs = {"current": simulate(args.eval_exe, args.eval_arg, bullet_columns(args.projectiles))}
if args.baseline: runs["baseline"] = simulate(args.baseline, args.baseline_arg, bullet_columns("" if args.baseline_projectiles == "none" else (args.baseline_projectiles or args.projectiles)))
rms = lambda x: 100 * np.sqrt(np.mean(x[~np.isnan(x)] ** 2)) if np.any(~np.isnan(x)) else float("nan")
bias = lambda x: 100 * np.nanmean(x) if np.any(~np.isnan(x)) else float("nan")
ev = {k: np.log(v / mv) for k, (v, p) in runs.items()}; ep = {k: np.log(p / mp) for k, (v, p) in runs.items()}
if args.max_pressure_dev > 0:
    far = {k: np.abs(runs[k][1] / mp - 1) * 100 > args.max_pressure_dev for k in runs}
    far = {k: np.where(np.isnan(runs[k][1] / mp), False, v) for k, v in far.items()}
    pick = {"baseline": far.get("baseline", far["current"]), "current": far["current"], "both": np.logical_and.reduce(list(far.values())),
            "either": np.logical_or.reduce(list(far.values()))}[args.exclude_by if len(runs) > 1 else "current"]
    print(f"{int(pick.sum())} loads with a pressure more than {args.max_pressure_dev:g} % off the published one left out ({args.exclude_by})")
    for k in runs: runs[k][0][pick] = np.nan; runs[k][1][pick] = np.nan
for k in runs: print(f"{k}: {int(np.sum(~np.isnan(runs[k][0])))} of {len(rows)} loads simulated")


def table(title, sel):
    print(f"\n{title}\n{'':22} {'n':>6} {'n(p)':>6} | " + " | ".join(f"{k:^34}" for k in runs) + (" |  change" if len(runs) > 1 else ""))
    print(f"{'':22} {'':>6} {'':>6} | " + " | ".join(f"{'v rms':>7} {'v bias':>7} {'p rms':>7} {'p bias':>7}  " for k in runs))
    for label, m in sel:
        m = m & ~np.isnan(runs["current"][0])
        if not m.any(): continue
        cells, vals = [], {}
        for k in runs:
            vals[k] = (rms(ev[k][m]), bias(ev[k][m]), rms(ep[k][m & ~np.isnan(mp)]), bias(ep[k][m & ~np.isnan(mp)]))
            cells.append(f"{vals[k][0]:7.2f} {vals[k][1]:+7.2f} {vals[k][2]:7.2f} {vals[k][3]:+7.2f}  ")
        d = f" | v {vals['current'][0]-vals['baseline'][0]:+.2f}  p {vals['current'][2]-vals['baseline'][2]:+.2f}" if len(runs) > 1 else ""
        print(f"{label:22} {int(m.sum()):6} {int((m & ~np.isnan(mp)).sum()):6} | " + " | ".join(cells) + d)


for name, subset in (("held-out cartridges", test), ("all loads", np.ones(len(rows), bool))):
    sel = [(c.capitalize(), (cls == c) & subset) for c in ("rifle", "pistol")]
    for c in ("rifle", "pistol"):
        sel += [(f"  {c} {s}", (cls == c) & (src == s) & subset) for s in sorted(set(src[cls == c]))]
    table(f"{name} (velocity / pressure error in %, rms and mean; n(p) = loads with a published pressure)", sel)

if len(runs) > 1 and args.worst:
    d = np.abs(ep["current"]) - np.abs(ep["baseline"])
    for title, order in (("pressure error grew most", np.argsort(-np.nan_to_num(d))), ("pressure error shrank most", np.argsort(np.nan_to_num(d)))):
        print(f"\n{title}")
        for i in order[:args.worst]:
            r = rows[i]
            print(f"  {r['powder']:22} {r['caliber']:12} {r['bullet_gr']:>6} gr {r['kind']:5} {float(r['charge_g']):6.3f} g  published {mp[i]:5.0f} bar:"
                  f" baseline {runs['baseline'][1][i]:5.0f}  current {runs['current'][1][i]:5.0f}")
