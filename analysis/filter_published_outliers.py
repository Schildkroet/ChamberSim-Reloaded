#!/usr/bin/env python3
"""Writes copies of the published-load sample files without the loads whose simulated pressure is far off the published one (probably misread or invalid).

    filter_published_outliers.py <csr_eval> [--max-dev 45] [--out DIR] [--profile physical] [samples.csv ...]

Every sample is simulated with the powder records of the build (csr_eval, default: all sample files of scripts/data/loaddata/sources/samples except
the shotshell files). A load with a published pressure whose simulated peak pressure differs by more than --max-dev percent (low or high) is left out of
the copy in DIR (default: scripts/data/loaddata/sources/samples_clean, git-ignored like the samples); the originals stay untouched. DIR/removed.csv lists
the dropped loads. Loads without a published pressure (Vihtavuori, ...) and loads that cannot be simulated are kept.
--apply instead edits the sample files themselves: the dropped loads are removed from them and appended (all columns, plus source_file, the reason and
the simulated pressure) to samples/invalid/invalid_samples.csv, a subfolder so that no tool picks it up as input; nothing is lost.
Use the copies with fit_physical_powders.py / compare_physical_published.py to fit and judge the powder records on the cleaned data.
"""
import argparse, csv, glob, json, os, subprocess, tempfile
from concurrent.futures import ThreadPoolExecutor

ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
ap.add_argument("eval_exe"); ap.add_argument("samples", nargs="*")
ap.add_argument("--max-dev", type=float, default=45.0)
ap.add_argument("--profile", default="physical")
ap.add_argument("--out", default="")
ap.add_argument("--apply", action="store_true", help="remove the loads from the sample files and keep them in samples/invalid/invalid_samples.csv")
ap.add_argument("--jobs", type=int, default=min(14, os.cpu_count() or 1))
args = ap.parse_args()
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
SDIR = os.path.join(ROOT, "scripts", "data", "loaddata", "sources", "samples")
out_dir = args.out or os.path.join(SDIR, "..", "samples_clean")
invalid_file = os.path.join(SDIR, "invalid", "invalid_samples.csv")
files = args.samples or sorted(f for f in glob.glob(os.path.join(SDIR, "*.csv")) if "shot" not in os.path.basename(f))
if not args.apply: os.makedirs(out_dir, exist_ok=True)
names = {}
for f in glob.glob(os.path.join(ROOT, "data", "propellants", "*.json")):
    for e in json.load(open(f)): names.setdefault(e["pname"].lower(), []).append(e["mname"] + " " + e["pname"])
full_names = {x.lower() for v in names.values() for x in v}
tmp = tempfile.mkdtemp(); removed, invalid_rows = [], []
for path in files:
    rows = list(csv.DictReader(open(path)))
    header = list(rows[0].keys())
    sim = [r.copy() for r in rows]
    for r in sim:  # a powder column without the maker ("N150") is matched by the product name when that is unique
        if r["powder"].lower() not in full_names and len(names.get(r["powder"].lower(), [])) == 1: r["powder"] = names[r["powder"].lower()][0]
    chunks = [list(range(i, len(rows), args.jobs)) for i in range(args.jobs)]
    pressure = {}
    def run(k):
        p = os.path.join(tmp, f"{os.path.basename(path)}.{k}.csv")
        with open(p, "w", newline="") as f:
            w = csv.DictWriter(f, header, extrasaction="ignore"); w.writeheader(); w.writerows(sim[i] for i in chunks[k])
        for l in subprocess.run([args.eval_exe, p, "--profile", args.profile], capture_output=True, text=True).stdout.splitlines():
            a = l.split(",")
            if len(a) == 3: pressure[chunks[k][int(a[0])]] = float(a[2])
    with ThreadPoolExecutor(args.jobs) as pool: list(pool.map(run, range(len(chunks))))
    kept = []
    for i, r in enumerate(rows):
        pub = float(r["pressure_bar"]) if r.get("pressure_bar") else 0.0
        if pub > 0 and i in pressure and abs(pressure[i] / pub - 1) * 100 > args.max_dev:
            removed.append(dict(file=os.path.basename(path), powder=r["powder"], caliber=r["caliber"], bullet=r["bullet"], bullet_gr=r["bullet_gr"], kind=r["kind"],
                                charge_g=r["charge_g"], published_bar=pub, simulated_bar=pressure[i], deviation_pct=round((pressure[i] / pub - 1) * 100, 1)))
            invalid_rows.append(dict(source_file=os.path.basename(path), invalid=f"pressure more than {args.max_dev:g} % off the simulation",
                                     sim_pressure_bar=pressure[i], deviation_pct=removed[-1]["deviation_pct"], **r))
        else: kept.append(r)
    with open(path if args.apply else os.path.join(out_dir, os.path.basename(path)), "w", newline="") as f:
        w = csv.DictWriter(f, header); w.writeheader(); w.writerows(kept)
    print(f"{os.path.basename(path):28} {len(rows):6} loads, {len(rows) - len(kept):4} removed")
if args.apply and invalid_rows:  # append to the earlier invalid loads (the columns of the files differ a little)
    old = list(csv.DictReader(open(invalid_file))) if os.path.exists(invalid_file) else []
    allrows = old + invalid_rows
    fields = list(dict.fromkeys(["source_file", "invalid", "sim_pressure_bar", "deviation_pct"] + [k for r in allrows for k in r]))
    os.makedirs(os.path.dirname(invalid_file), exist_ok=True)
    with open(invalid_file, "w", newline="") as f:
        w = csv.DictWriter(f, fields, restval=""); w.writeheader(); w.writerows(allrows)
    print(f"{len(removed)} loads moved to {invalid_file}")
elif removed:
    with open(os.path.join(out_dir, "removed.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, list(removed[0])); w.writeheader(); w.writerows(removed)
if not args.apply: print(f"{len(removed)} loads removed in total -> {out_dir}")
