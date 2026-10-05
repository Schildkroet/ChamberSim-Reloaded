#!/usr/bin/env python3
"""Fits the burn rate Ba of every Vihtavuori powder to the published load data.

    vv_fit_all.py [-o vv_fits.csv] [--fitdata build/release/grt_fitdata] [--json file] [--work dir]

For each powder: scripts/data/loaddata/vv_extract.py makes the samples, then grt_fitdata fits Ba twice, with the explosion heat of a
single-base (3800 kJ/kg) and of a double-base powder (4300 kJ/kg): the tables do not say which kind a powder is, and Ba
depends on the heat. The fit with the smaller velocity error is kept. Samples that the solver rejects (charge too high for the case at the assumed
bulk density) are left out and counted. Qex, k and the densities are NOT fitted: they stay
at these generic values until they are taken from the manufacturer's safety data sheet.
"""
import argparse
import csv
import json
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
VARIANTS = {"single-base": "Qex=3800,k=1.23,pc=1580,pcd=900", "double-base": "Qex=4300,k=1.24,pc=1620,pcd=900"}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("vv_fits.csv"))
    ap.add_argument("--fitdata", type=Path, default=ROOT / "build" / "release" / "grt_fitdata")
    ap.add_argument("--json", type=Path)
    ap.add_argument("--work", type=Path, default=Path("vv_work"))
    ap.add_argument("--table", type=Path, help="vv_powder_data.csv: the published energy content and bulk density per powder replace the generic values")
    ap.add_argument("--form", default="1.2,0.45,0.8", help="form function a0,z1,z2 used while Ba is fitted")
    ap.add_argument("--powders", help="comma separated powders (default: all)")
    args = ap.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    data = args.json or args.work / "vv_data.json"
    if not data.exists():
        subprocess.run([sys.executable, str(HERE / "vv_extract.py"), "N150", "--cache", str(args.work), "-o", str(args.work / "tmp.csv")], check=True)
    powders = [p["PowderType"] for p in json.loads(data.read_text(encoding="utf-8"))["PowderData"]]
    if args.powders:
        powders = args.powders.split(",")
    a0, z1, z2 = args.form.split(",")
    table = {}
    if args.table:
        with open(args.table, newline="", encoding="utf-8") as f:
            table = {r["powder"]: r for r in csv.DictReader(f)}
    rows = []
    for powder in sorted(powders):
        csv_path = args.work / f"vv_{powder}.csv"
        r = subprocess.run([sys.executable, str(HERE / "vv_extract.py"), powder, "--json", str(data), "-o", str(csv_path)], capture_output=True, text=True)
        if r.returncode != 0 or not csv_path.exists():
            print(f"{powder}: no samples", file=sys.stderr)
            continue
        best = None
        variants = VARIANTS
        if table:
            if powder not in table:
                continue
            t = table[powder]  # published: energy content J/g = kJ/kg, bulk density g/l = kg/m3; k and pc are typical values
            variants = {"table": f"Qex={t['energy_J_g']},k=1.23,pc=1580,pcd={t['bulk_density_g_l']}"}
        for kind, start in variants.items():
            start += f",a0={a0},z1={z1},z2={z2}"
            out = subprocess.run([str(args.fitdata), str(csv_path), "--params", "Ba", "--start", start], capture_output=True, text=True).stdout
            m = re.search(r"Ba ([\d.]+) -> ([\d.]+)", out)
            e = re.search(r"Velocity error \(rms\): ([\d.]+) % -> ([\d.]+) %", out)
            n = re.search(r"Samples: (\d+) fitted.*?(\d+) charge too high", out)
            if not (m and e and n):
                continue
            res = {"powder": powder, "samples": int(n[1]), "assumed": kind, "Ba": float(m[2]), "too_high": int(n[2]), "rms_start_%": float(e[1]), "rms_fit_%": float(e[2])}
            if best is None or res["rms_fit_%"] < best["rms_fit_%"]:
                best = res
            res_both = res
            rows.append({**res_both, "kept": ""})
        if best:
            for row in rows:
                if row["powder"] == powder and row["assumed"] == best["assumed"]:
                    row["kept"] = "yes"
            print(f"{powder:6} n={best['samples']:4} (+{best['too_high']:3} rejected)  {best['assumed']:11} Ba {best['Ba']:.4f}  rms {best['rms_fit_%']:.2f} %", file=sys.stderr)
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, ["powder", "samples", "too_high", "assumed", "Ba", "rms_start_%", "rms_fit_%", "kept"])
        w.writeheader()
        w.writerows(rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
