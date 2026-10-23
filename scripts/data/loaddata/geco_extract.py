#!/usr/bin/env python3
"""Turns the GECO reloading data (PDFs of geco-ammunition.com > Service > Downloads) into fit samples (CSV).

    geco_extract.py GECO_Reloading-Data_CFR_DE_23.pdf GECO_Reloading-Data_P_R_DE.pdf [-o geco_samples.csv]

Rifle cartridges (7 pages) and handgun cartridges (2 pages) are tables of numbered loads: cartridge, bullet (GECO: type, weight, diameter), powder maker
and powder, maximum and minimum charge, primer, case, cartridge length L6 and the velocity v3 at the maximum charge. The minimum charge has no velocity, so
every load is one "max" sample (v3 is measured 3 m in front of the muzzle, taken as the muzzle velocity). The test barrel is not given: it is assumed
(--handgun-barrel 120 mm, --rifle-barrel 600 mm, as for the H&N catalogue); no pressures. The bullets are GECO's: a project record gives the measured or
estimated length, the cartridge length of the table the seating depth."""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

from hn_extract import powder_name
from samples_common import Records  # noqa: E402

GR = 0.06479891
IN = 25.4
NUM = r"\d+(?:,\d+)?"
ROW = re.compile(rf"^(?P<n>\d+) (?P<cal>.+?) (?P<dmm>{NUM}(?: S)?) \|\s*\.(?P<dia>\d{{2,3}}) (?P<type>.+?) (?P<g>\d+,\d+) ?\|\s*(?P<gr>\d+) "
                 rf"(?P<maker>.+?) \|\s*(?P<powder>.+?) (?P<maxg>\d,\d{{3}}) \|\s*(?P<maxgr>{NUM}) (?P<ming>\d,\d{{3}}) ?\|\s*(?P<mingr>{NUM}) "
                 rf"(?P<primer>.+?) (?P<case>[A-Za-z]+) (?P<col>\d+,\d\d) (?P<v>\d+)$")
# the table's spelling of cartridges -> the C.I.P. names of the records (what is not here is looked up as written)
CALIBERS = {"6,5 x 55 SE": "6.5 x 55 SE", "6,5 x 57": "6.5 x 57", "6,5 x 57 R": "6.5 x 57 R", "6,5 x 68": "6.5 x 68", "7,5 x 55": "7.5 x 55 Suisse",
            "7,62 x 54 R": "7.62 x 54 R", "8 x 57 IS": "8 x 57 IS", "8 x 57 IRS": "8 x 57 IRS", "9,3 x 62": "9.3 x 62", "9,3 x 74 R": "9.3 x 74 R",
            ".30-06 Spr.": "30-06 Spring.", ".30-06": "30-06 Spring.", ".300 Win. Mag.": "300 Win. Mag.", ".308 Win.": "308 Win.",
            ".30 R Blaser": "30 R Blaser", "Blaser .30 R": "30 R Blaser", "Weatherby Mag.": "300 Weath. Mag.", ".300 WSM": "300 Win. Short Mag.",
            ".357 Mag.": "357 Mag.", ".45 Auto": "45 Auto", ".44 Rem. Mag.": "44 Rem. Mag.", "9 mm Luger": "9 mm Luger"}
MAKERS = {"Rottweil": "Rottweil", "Reload Swiss": "Reload Swiss", "Vihtavuori": "Vihtavuori", "Hodgdon": "Hodgdon", "Vectan": "Vectan"}


def rows_of(pdf: Path) -> list[str]:
    """One text line per numbered load, from the words' positions (the PDF's text order is scrambled). The row number stands in the first column, the
    words of a row lie within half a row height of it. A cell of two or more lines (maker above powder, primer type above name) is kept together."""
    xml = subprocess.run(["pdftotext", "-bbox", "-f", "2", str(pdf), "-"], capture_output=True, text=True, check=True).stdout
    rows: dict[int, str] = {}
    for page in xml.split("<page ")[1:]:
        words = [(float(x0), float(y0), float(x1), float(y1), t) for x0, y0, x1, y1, t in
                 re.findall(r'xMin="([\d.]+)" yMin="([\d.]+)" xMax="([\d.]+)" yMax="([\d.]+)">(.*?)</word>', page)]
        numbers = sorted(((w[1] + w[3]) / 2, int(w[4])) for w in words if w[4].isdigit() and w[0] < 30)
        if len(numbers) < 2:
            continue
        pitch = min(b[0] - a[0] for a, b in zip(numbers, numbers[1:]) if b[0] - a[0] > 5)
        for cy, n in numbers:
            row = [w for w in words if abs((w[1] + w[3]) / 2 - cy) < pitch / 2 and w[0] >= 30]
            single = [w for w in row if abs((w[1] + w[3]) / 2 - cy) <= 3]
            stacked = sorted((w for w in row if w not in single), key=lambda w: w[0])
            cells: list[list[tuple]] = []
            for w in stacked:  # words above each other (x ranges overlap or touch) form one cell
                if cells and w[0] < max(c[2] for c in cells[-1]) + 6:
                    cells[-1].append(w)
                else:
                    cells.append([w])
            parts = [(w[0], [w[4]]) for w in single]
            parts += [(min(c[0] for c in cell), [w[4] for w in sorted(cell, key=lambda w: (round((w[1] + w[3]) / 12), w[0]))]) for cell in cells]
            rows[n] = f"{n} " + " ".join(t for _, ts in sorted(parts, key=lambda p: p[0]) for t in ts)
    return [rows[n] for n in sorted(rows)]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("rifle", type=Path)
    ap.add_argument("handgun", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("geco_samples.csv"))
    ap.add_argument("--handgun-barrel", type=float, default=120.0)
    ap.add_argument("--rifle-barrel", type=float, default=600.0)
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    records = Records(args.data)
    out, miss, bad = [], {}, []
    for pdf, barrel in ((args.rifle, args.rifle_barrel), (args.handgun, args.handgun_barrel)):
        for text in rows_of(pdf):
            r = ROW.match(re.sub(r"\s+", " ", text))
            if not r:
                bad.append(text)
                continue
            name = r["cal"].strip()
            cal = records.caliber(CALIBERS.get(name, name))
            if not (cal and cal.get("G1")):
                miss[name] = miss.get(name, 0) + 1
                continue
            mass_g, dia_mm = float(r["g"].replace(",", ".")), float("0." + r["dia"]) * IN
            grains = mass_g / GR
            length, source, tail = records.bullet("GECO", r["type"], grains, dia_mm)
            maker = MAKERS.get(r["maker"].strip(), r["maker"].strip())
            sample = {"powder": r["powder"].strip() if maker == "Reload Swiss" else powder_name(maker, r["powder"]), "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""),
                        "bullet": f"GECO {r['type']} .{r['dia']}", "bullet_gr": round(grains, 1), "bullet_dia_mm": round(dia_mm, 3), "bullet_len_mm": f"{length:.2f}",
                        "len_source": source, "tail_type": tail, "col_mm": f"{float(r['col'].replace(',', '.')):.2f}", "barrel_mm": f"{barrel:.0f}", "twist_mm": "",
                        "kind": "max", "charge_g": round(float(r["maxg"].replace(",", ".")), 4), "velocity_ms": float(r["v"]), "pressure_bar": "",
                        "primer": r["primer"]}
            if sample not in out:  # the rifle table repeats some loads (rows 75-87 = rows 51-67)
                out.append(sample)
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(out[0].keys()))
        w.writeheader()
        w.writerows(out)
    print(f"{len(out)} samples -> {args.output}", file=sys.stderr)
    print("cartridges not found:", ", ".join(f"{k} ({v})" for k, v in sorted(miss.items())), file=sys.stderr)
    print("rows not understood:", *bad, sep="\n  ", file=sys.stderr)
    print("powders:", ", ".join(sorted({r["powder"] for r in out})), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
