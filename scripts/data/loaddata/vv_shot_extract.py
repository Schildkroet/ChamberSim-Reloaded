#!/usr/bin/env python3
"""Turns Vihtavuori's two shotgun load tables (12/76, lead and steel shot; PDFs with a text layer, 2020) into fit samples (CSV) for the shotshell model.

    vv_shot_extract.py Vihtavuori_ReloadingData_Shotgun_leadshot_052020.pdf Vihtavuori_ReloadingData_Shotgun_2020_02.pdf [-o vv_shot_samples.csv]

Every row of the PDFs gives a start and a maximum load (charge in g, velocity in m/s, 28" test barrel). The velocity is measured 2.5 m in front of the
muzzle with 3 mm pellets (U.S. No. 5); the columns `distance_m`, `pellet_mm` and `shot_density` (g/cm3, csr_eval: length of the shot column) carry what is
needed to convert it to the muzzle. The tables contain no pressures (`pressure_bar` is empty). Needs pdftotext.
"""
import argparse
import csv
import re
import subprocess
from pathlib import Path

ROW = re.compile(r"^\s+(3?N\d+)\s+(\S+(?: \S+)*?)\s{2,}(.+?)\s+Paper\s+Roll Crimp\s+([\d,]+)\s+[\d.]+\s+(\d+)\s+\d+\s+([\d,]+)\s+[\d.]+\s+(\d+)\s+\d+\s*$")
DENSITY = {"lead": 11.34, "steel": 7.87}


def rows(pdf: Path):
    text = subprocess.run(["pdftotext", "-layout", str(pdf), "-"], capture_output=True, text=True, check=True).stdout
    material = "steel" if "STEEL SHOT" in text else "lead"
    shell = re.search(r"SHELL:\s*(.+?)\s{2,}", text).group(1).title()
    shot = 0.0
    for line in text.splitlines():
        m = re.search(r"Shot Load (\d+) g", line)
        if m:
            shot = float(m.group(1))
        m = ROW.match(line)
        if m:
            powder, primer, wad = m.group(1), m.group(2), m.group(3)
            for kind, i in (("start", 4), ("max", 6)):
                yield dict(powder="Vihtavuori " + powder, gauge=12, hull_mm=76, shot_g=shot, wad=wad.strip(), primer=primer, shell=shell,
                           charge_g=float(m.group(i).replace(",", ".")), velocity_ms=int(m.group(i + 1)), pressure_bar="", shot_density=DENSITY[material],
                           pellet_mm=3.0, distance_m=2.5, material=material, kind=kind)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pdf", type=Path, nargs="+")
    ap.add_argument("-o", "--output", type=Path, default=Path("vv_shot_samples.csv"))
    args = ap.parse_args()
    out = [r for pdf in args.pdf for r in rows(pdf)]
    with open(args.output, "w", newline="") as f:
        w = csv.DictWriter(f, list(out[0].keys()))
        w.writeheader()
        w.writerows(out)
    print(len(out), "loads ->", args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
