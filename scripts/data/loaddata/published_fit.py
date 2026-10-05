#!/usr/bin/env python3
"""Fits the burn rate Ba of every powder in a samples CSV (lovex_extract.py, rs_extract.py) and writes the propellant records.

    published_fit.py samples.csv --maker Lovex --source "..." [--base-from-name] [-o data/propellants] [--fits fits.csv]

Per powder grt_fitdata fits Ba (velocities of the start and maximum loads and the measured pressures of the table, --use-pressure)
with the form function a0 = 0, z1 = 0.3, z2 = 0.6 (see README: the data do not determine it) and the explosion heat of a single-base
(3800 kJ/kg) or a double-base powder (4300 kJ/kg). With --base-from-name the type follows the name (Lovex: S = single, D = double base),
else both are tried and the one with the smaller velocity error is kept. Qex, k, pc and pcd are not published: typical values.
Records are written for powders with at least --min-samples samples and at most --max-rms percent velocity error.
"""
import argparse
import csv
import json
import re
import subprocess
import sys
import unicodedata
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "scripts" / "data"))
import recordfiles  # noqa: E402

FORM = {"a0": 0.0, "z1": 0.3, "z2": 0.6}
BASES = {"single-base": {"Qex": 3800.0, "k": 1.23, "pc": 1580.0}, "double-base": {"Qex": 4300.0, "k": 1.24, "pc": 1620.0}}
PCD = 900.0
NAME_FIX = {"HODGDON SUPRFORM": "Hodgdon Superformance"}  # the manual's own abbreviation
ORDER = ["mname", "pname", "lotid", "Ba", "Qex", "k", "a0", "a1", "z1", "z2", "pc", "pcd", "pt", "tcc", "tch", "cdate", "origin", "descr"]


def slug(*parts: str) -> str:
    s = unicodedata.normalize("NFKD", "-".join(p for p in parts if p)).lower()
    return re.sub(r"-+", "-", re.sub(r"[^a-z0-9]+", "-", s)).strip("-")


def fit(csv_path: Path, base: dict, fitdata: Path, pressure: bool) -> dict | None:
    start = ",".join(f"{k}={v}" for k, v in {**base, "pcd": PCD, **FORM}.items())
    cmd = [str(fitdata), str(csv_path), "--params", "Ba", "--start", start] + (["--use-pressure"] if pressure else [])
    out = subprocess.run(cmd, capture_output=True, text=True).stdout
    m = re.search(r"Ba ([\d.]+) -> ([\d.]+)", out)
    e = re.search(r"Velocity error \(rms\): ([\d.]+) % -> ([\d.]+) %(?:;\s+pressure error \(rms, measured\): ([\d.]+) % -> ([\d.]+) %)?", out)
    n = re.search(r"Samples: (\d+) fitted.*?(\d+) charge too high", out)
    if not (m and e and n):
        return None
    return {"samples": int(n[1]), "too_high": int(n[2]), "Ba": float(m[2]), "v_rms": float(e[2]), "p_rms": float(e[4] or 0)}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("samples", type=Path, nargs="+", help="one or more sample files; identical samples of several sources count once")
    ap.add_argument("--maker", help="maker of all powders (default: the first word of the powder name: \"Accurate 4350\")")
    ap.add_argument("--source", required=True, help="where the data come from, for the record's origin and description")
    ap.add_argument("--base-from-name", action="store_true", help="S... = single base, D... = double base (Lovex)")
    ap.add_argument("--fitdata", type=Path, default=ROOT / "build" / "release" / "grt_fitdata")
    ap.add_argument("--base", choices=list(BASES), help="the type of all powders when the maker states it (Vectan: single base)")
    ap.add_argument("--note", default="", help="added to the description of the records (assumptions about the data)")
    ap.add_argument("--no-pressure", action="store_true", help="fit the velocities only")
    ap.add_argument("--min-samples", type=int, default=40)
    ap.add_argument("--max-rms", type=float, default=6.0)
    ap.add_argument("--fits", type=Path, help="write all fits to this CSV")
    ap.add_argument("-o", "--output", type=Path, default=ROOT / "data" / "propellants")
    ap.add_argument("--only", help="comma separated powders")
    ap.add_argument("--write", action="store_true", help="write the records (default: only report)")
    ap.add_argument("--overwrite", action="store_true")
    ap.add_argument("--skip-existing", action="store_true", help="do not fit powders that already have a record")
    ap.add_argument("--min-cartridges", type=int, default=1, help="records need samples of at least this many cartridges")
    args = ap.parse_args()
    existing = {recordfiles.key(r): r for _, r in recordfiles.read(args.output)}
    rows, seen, names = [], set(), {}
    for samples in args.samples:
        with open(samples, newline="", encoding="utf-8") as f:
            for r in csv.DictReader(f):
                key = tuple(r[k].upper() if k == "powder" else r[k] for k in ("powder", "caliber", "bullet_gr", "kind", "charge_g", "velocity_ms", "col_mm"))
                if r.get("pressure_bar") and not 500 <= float(r["pressure_bar"]) <= 9000:
                    r["pressure_bar"] = ""  # a misread column
                if key not in seen:
                    seen.add(key)
                    r["powder"] = names.setdefault(r["powder"].upper(), NAME_FIX.get(r["powder"].upper(), r["powder"]))
                    rows.append(r)
    powders = sorted({r["powder"] for r in rows})
    if args.only:
        powders = args.only.split(",")
    work = args.samples[0].parent / f"{args.samples[0].stem}_by_powder"
    work.mkdir(exist_ok=True)
    fits, written = [], 0
    for powder in powders:
        mine = [r for r in rows if r["powder"] == powder]
        if len(mine) < args.min_samples:
            continue
        mk, pn = (args.maker, powder) if args.maker else powder.split(" ", 1)
        if args.skip_existing and recordfiles.key({"mname": mk, "pname": pn}) in existing:
            continue
        path = work / f"{slug(powder)}.csv"
        with open(path, "w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, list(mine[0].keys()))
            w.writeheader()
            w.writerows(mine)
        kinds = ([("single-base" if powder.upper().startswith("S") else "double-base")] if args.base_from_name
                 else [args.base] if args.base else list(BASES))
        best = None
        for kind in kinds:
            res = fit(path, BASES[kind], args.fitdata, not args.no_pressure)
            if res and (best is None or res["v_rms"] < best["v_rms"]):
                best = {**res, "base": kind}
        if not best:
            print(f"{powder}: no fit")
            continue
        cartridges = {r["caliber"] for r in mine}
        fits.append({"powder": powder, **best, "cartridges": len(cartridges)})
        print(f"{powder:7} n={best['samples']:4} {best['base']:11} Ba {best['Ba']:.4f}  velocity {best['v_rms']:.2f} %  pressure {best['p_rms']:.1f} %  ({len(cartridges)} cartridges)")
        if not args.write or best["samples"] < args.min_samples or best["v_rms"] > args.max_rms or len(cartridges) < args.min_cartridges:
            continue
        base = BASES[best["base"]]
        how = ("the type follows the name (S = single base, D = double base)" if args.base_from_name
               else f"the maker states {args.base}" if args.base
               else f"the type (single/double base) is not published, {best['base']} assumed because it fits the velocities better")
        maker, pname = (args.maker, powder) if args.maker else powder.split(" ", 1)
        rec = {"mname": maker, "pname": pname, "lotid": "", "Ba": round(best["Ba"], 4), "Qex": base["Qex"], "k": base["k"], **FORM, "a1": 0.0,
               "pc": base["pc"], "pcd": PCD, "pt": 21.0, "tcc": 0.0, "tch": 0.0, "cdate": date.today().isoformat(), "origin": args.source,
               "descr": (f"Burn rate Ba fitted to {best['samples']} start and maximum loads of the manufacturer's published load data "
                         f"({len(cartridges)} cartridges), velocity error {best['v_rms']:.1f} % rms"
                         + ("" if args.no_pressure else f", pressure error {best['p_rms']:.0f} % rms (published pressures)")
                         + f". Not published and therefore only typical or chosen values: Qex ({how}), k, material density pc, bulk density pcd, "
                           "and the form function (a0, z1, z2: the data hardly determine it). Check every load against the manufacturer's current load data."
                         + (f" {args.note}" if args.note else "")
                         + (" Fewer data or a poorer fit than most other records: use with more caution."
                            if best["samples"] < 40 or best["v_rms"] > 5.5 else ""))}
        if recordfiles.key(rec) in existing and not args.overwrite:
            continue
        existing[recordfiles.key(rec)] = {k: rec[k] for k in ORDER}
        written += 1
    if written:
        recordfiles.write(args.output, list(existing.values()))  # data/propellants/<manufacturer>.json
    if args.fits:
        with open(args.fits, "w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, ["powder", "samples", "cartridges", "base", "Ba", "v_rms", "p_rms", "too_high"])
            w.writeheader()
            w.writerows(fits)
    print(f"{written} records written")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
