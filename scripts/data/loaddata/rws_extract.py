#!/usr/bin/env python3
"""Turns the reloading data of RWS (rws-ammunition.com, Rottweil powders) into fit samples (CSV), like rs_extract.py.

    rws_extract.py [-o rws_samples.csv] [--html page.html]

The page /de/infotainment/wiederladen has one HTML table: per row cartridge, bullet (maker, type, mass), Rottweil powder with maximum and
minimum charge, primer, case, cartridge length L6 and the velocity v3 of the maximum load (3 m from the muzzle). The page names no pressures
and only the maximum load has a velocity, so only velocities are fitted. The test barrels are those of the reloading book (BARRELS, 600 mm
unless listed).
Cartridges and bullets are matched as in lovex_extract.py.
"""
import argparse
import csv
import html
import re
import sys
import urllib.request
from pathlib import Path

from samples_common import Records  # noqa: E402

PAGE = "https://rws-ammunition.com/de/infotainment/wiederladen"
BARREL_MM = 600.0
# test barrel lengths of the reloading book (chapter 9, page 20, "Tabelle mit Lauflängen"); all other rifle cartridges: 600 mm
BARRELS = {"6 mm Rem.": 650, "6 x 62 Freres": 650, "6,5 x 54 M.Sch.": 560, "6,5 x 68 R": 650, ".300 Win. Mag.": 650, ".300 Weatherby Mag.": 650,
           "7 mm Rem. Mag.": 650, "7 x 64": 650, ".30 M 1 Carbine": 450, "7,62 x 39": 650, "8 x 68 S": 650, "8,5 x 63": 650, "8,5 x 63 R": 650,
           "9,3 x 64": 650, ".375 H& H Mag.": 650, ".378 Weatherby Mag.": 650, ".416 Rem. Mag.": 650, ".444 Marlin": 650, ".470 N.E.": 665}


def rows(page: str) -> list[list[str]]:
    t = page[page.index("<table"):page.index("</table>")]
    return [[html.unescape(re.sub(r"<[^>]+>", "", c)).strip() for c in re.findall(r"<t[dh].*?</t[dh]>", r, re.S)]
            for r in re.findall(r"<tr.*?</tr>", t, re.S)]


def num(v: str) -> float:
    try:
        return float(v.replace(",", ".").replace(" ", ""))
    except ValueError:
        return 0.0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("rws_samples.csv"))
    ap.add_argument("--html", type=Path, help="saved page instead of downloading it")
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    if args.html:
        page = args.html.read_text(encoding="utf-8")
    else:
        req = urllib.request.Request(PAGE, headers={"User-Agent": "Mozilla/5.0 (ChamberSimReloaded data collection)"})
        page = urllib.request.urlopen(req, timeout=60).read().decode("utf-8")
    records = Records(args.data)
    barrels = {}
    for name, mm in BARRELS.items():
        if rec := records.caliber(name):
            barrels[rec["cipname"]] = mm
    out, miss = [], {}
    for r in rows(page)[1:]:
        if len(r) < 19:
            continue
        cal = records.caliber(r[1])
        if not cal:
            miss[r[1]] = miss.get(r[1], 0) + 1
            continue
        dia, grains = cal.get("G1") or 0, num(r[6])
        charge, velocity, col = num(r[9]), num(r[18]), num(r[17])
        if not (dia and grains and charge and velocity and col):
            continue
        length, source, tail = records.bullet(r[3], r[4], grains, dia)
        out.append({"powder": r[8].replace(" ", ""), "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""),
                    "bullet": f"{r[3]} {r[4]}".strip(), "bullet_gr": grains, "bullet_dia_mm": dia, "bullet_len_mm": f"{length:.2f}",
                    "len_source": source, "tail_type": tail, "col_mm": col, "barrel_mm": barrels.get(cal["cipname"], BARREL_MM), "twist_mm": "", "kind": "max",
                    "charge_g": charge, "velocity_ms": velocity, "pressure_bar": ""})
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(out[0].keys()))
        w.writeheader()
        w.writerows(out)
    print(f"{len(out)} samples -> {args.output}", file=sys.stderr)
    print("cartridges not found:", ", ".join(f"{k} ({v})" for k, v in sorted(miss.items())), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
