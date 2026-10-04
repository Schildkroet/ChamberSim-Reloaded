#!/usr/bin/env python3
"""Turns the shotshell tables of the Vectan reloading catalogue (pages 27-44, PDF with a text layer) into fit samples (CSV), like
hodgdon_shot_extract.py.

    vectan_shot_extract.py catalogue.pdf [-o vectan_shot_samples.csv]       (needs pdftotext)

Every table has a header "12 GA. 2 3/4, A24 POWDER, LEAD SHOT, FOLD CRIMP" (12 gauge: the powder is in the header) or
"20 GA. 2 3/4, 7/8 oz. LEAD SHOT, FOLD CRIMP" (16 and 20 gauge: the powder is a column) and rows of shot weight, hull, primer, wad,
powder, charge (grains), velocity (fps) and pressure (psi). Same columns as the Hodgdon file: the CSV has gauge, hull length (mm),
shot (g), charge (g), velocity (m/s, measured in front of the muzzle, see calibrate_shotshell.py) and pressure (bar).
"""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

OZ = 28.349523125
GR = 0.06479891
FPS = 0.3048
PSI = 0.0689475729
ROW = re.compile(r"^\s*(\d+(?: \d/\d)?|\d/\d)\s*oz\.\s+(.+?)\s+([\d.]+)\s*gr\s+(\d+)\s+([\d,]+)\s*PSI\s*$")
HEADER = re.compile(r"(\d+)\s*GA\.\s*(\d)\s*(\d/\d)?(?:\s*,\s*(.*?)\s*POWDER)?")
POWDERS = ["PRIMA SV", "PRIMA V", "206V", "206 SV", "GM3", "D20", "A24", "A1", "A0", "AS"]


def oz(text: str) -> float:
    total = 0.0
    for part in text.split():
        a, _, b = part.partition("/")
        total += int(a) / int(b) if b else int(a)
    return total


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pdf", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("vectan_shot_samples.csv"))
    ap.add_argument("--first", default="27")
    ap.add_argument("--last", default="44")
    args = ap.parse_args()
    text = subprocess.run(["pdftotext", "-layout", "-f", args.first, "-l", args.last, str(args.pdf), "-"], capture_output=True, text=True,
                          check=True).stdout
    rows, gauge, hull, powder, bad = [], None, None, None, 0
    for line in text.splitlines():
        if "GA." in line and "LEAD SHOT" in line:
            m = HEADER.search(line)
            gauge = int(m[1])
            hull = 70.0 if m[3] == "3/4" else 65.0 if m[3] == "1/2" else 76.0  # 2 3/4" = 70 mm, 2 1/2" = 65 mm
            powder = re.sub(r"\s+", " ", m[4]).strip() if m[4] else None
            continue
        m = ROW.match(line)
        if not (m and gauge):
            continue
        parts = re.split(r"\s{2,}", m[2].strip())
        # hull, primer, wad and the powder (the last column, also in the tables with the powder in the header)
        if len(parts) < 4:
            bad += 1
            continue
        hullname, primer, wad, name = parts[0], parts[1], " ".join(parts[2:-1]), parts[-1]
        name = re.sub(r"\s+", " ", name).strip()
        name = {"PRIMA SV": "Prima SV", "PRIMA V": "Prima V"}.get(name.upper(), name)
        rows.append({"powder": f"Vectan {name}", "gauge": gauge, "hull_mm": f"{hull:.0f}", "shot_g": f"{oz(m[1]) * OZ:.2f}", "wad": wad,
                     "primer": primer, "charge_g": f"{float(m[3]) * GR:.4f}", "velocity_ms": f"{int(m[4]) * FPS:.1f}",
                     "pressure_bar": f"{float(m[5].replace(',', '')) * PSI:.0f}", "hull": hullname})
    if not rows:
        print("no rows", file=sys.stderr)
        return 1
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    import collections
    print(f"{len(rows)} loads -> {args.output} ({bad} rows not parsed)", file=sys.stderr)
    print("powders:", dict(collections.Counter(r["powder"] for r in rows)), file=sys.stderr)
    print("gauge/shot:", dict(collections.Counter((r["gauge"], r["shot_g"]) for r in rows)), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
