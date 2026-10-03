#!/usr/bin/env python3
"""Writes the project's propellant records (data/propellants/*.json) from the fits of vv_fit_all.py.

    vv_to_json.py vv_fits.csv --work vv_work [-o data/propellants] [--max-rms 5.2] [--min-samples 80] [--overwrite]

Only powders whose fit is good enough are written: velocity error (rms) at most --max-rms percent from at least --min-samples
samples. The record holds the fitted burn rate Ba and, from Vihtavuori's published table (vv_powder_data.csv): the energy
content as the explosion heat Qex and the bulk density pcd. Not published, and therefore typical values or a choice made on the
velocity data (see README): k, the material density pc and the form function (a0, z1, z2); the description says so.
"""
import argparse
import csv
import json
import re
import unicodedata
from datetime import date
from pathlib import Path

TYPICAL = {"a1": 0.0, "k": 1.23, "pc": 1580.0, "pt": 21.0, "tcc": 0.0, "tch": 0.0}  # typical values of nitrocellulose powders
ORDER = ["mname", "pname", "lotid", "Ba", "Qex", "k", "a0", "a1", "z1", "z2", "pc", "pcd", "pt", "tcc", "tch", "cdate", "origin", "descr"]


def slug(*parts: str) -> str:
    s = unicodedata.normalize("NFKD", "-".join(p for p in parts if p)).lower()
    return re.sub(r"-+", "-", re.sub(r"[^a-z0-9]+", "-", s)).strip("-")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("fits", type=Path)
    ap.add_argument("--work", type=Path, default=Path("vv_work"))
    ap.add_argument("--table", type=Path, default=Path(__file__).resolve().parent / "vv_powder_data.csv")
    ap.add_argument("--form", default="0,0.3,0.6", help="form function a0,z1,z2 the fits were made with (vv_fit_all.py --form)")
    ap.add_argument("-o", "--output", type=Path, default=Path("data/propellants"))
    ap.add_argument("--max-rms", type=float, default=5.2)
    ap.add_argument("--min-samples", type=int, default=80)
    ap.add_argument("--overwrite", action="store_true")
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    written = 0
    a0, z1, z2 = (float(v) for v in args.form.split(","))
    with open(args.table, newline="", encoding="utf-8") as tf:
        table = {r["powder"]: r for r in csv.DictReader(tf)}
    with open(args.fits, newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            if row["kept"] != "yes" or float(row["rms_fit_%"]) > args.max_rms or int(row["samples"]) < args.min_samples:
                continue
            powder = row["powder"]
            cartridges = set()
            samples = args.work / f"vv_{powder}.csv"
            if samples.exists():
                with open(samples, newline="", encoding="utf-8") as sf:
                    cartridges = {r["caliber"] for r in csv.DictReader(sf)}
            t = table[powder]
            rec = {"mname": "Vihtavuori", "pname": powder, "lotid": "", "Ba": round(float(row["Ba"]), 4), "Qex": float(t["energy_J_g"]),
                   "k": TYPICAL["k"], "a0": a0, "a1": TYPICAL["a1"], "z1": z1, "z2": z2, "pc": TYPICAL["pc"], "pcd": float(t["bulk_density_g_l"]),
                   "pt": TYPICAL["pt"], "tcc": TYPICAL["tcc"], "tch": TYPICAL["tch"], "cdate": date.today().isoformat(),
                   "origin": "Vihtavuori's published powder data (energy content, bulk density) and load tables (tools/loaddata)",
                   "descr": (f"Qex (energy content) and pcd (bulk density) as published by Vihtavuori; grain {t['grain_length_mm']} x "
                             f"{t['grain_diameter_mm']} mm" + (f", {t['nitroglycerine_%']} % nitroglycerine" if t["nitroglycerine_%"] else "") +
                             f". Burn rate Ba fitted to {row['samples']} start and maximum loads of Vihtavuori's published reloading data "
                             f"({len(cartridges)} cartridges), velocity error {float(row['rms_fit_%']):.1f} % rms. Not published and therefore only "
                             "typical or chosen values: k, material density pc, and the form function (a0, z1, z2: the combination that reproduces "
                             "the velocities best, the data hardly determine it). Check every load against the manufacturer's current load data.")}
            path = args.output / (slug("vihtavuori", powder) + ".json")
            if path.exists() and not args.overwrite:
                continue
            body = ",\n".join(f"  {json.dumps(k)}: {json.dumps(rec[k], ensure_ascii=False)}" for k in ORDER)
            path.write_text("{\n" + body + "\n}\n", encoding="utf-8")
            written += 1
    print(f"{written} propellant files written to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
