#!/usr/bin/env python3
"""Turns the load tables of the H&N bullet catalogue (H&N Geschossprogramm, 15th edition, PDF of lhs-germany.de) into fit samples (CSV).

    hn_extract.py catalogue.pdf [-o hn_samples.csv]

Pages 10-13: "ideal loads" of handgun cartridges, pages 16-17: reduced rifle loads, one load per row (no minimum or maximum loads): form, bullet
(H&N, diameter, weight in grains), cartridge length, case, primer, powder maker, charge (g), powder, crimp, V0 (m/s) and the test weapon. The
weapon is named, its barrel length is not: the barrel is assumed (--handgun-barrel 120 mm, --rifle-barrel 600 mm, as for the Vectan catalogue); no
pressures. Every row is written as a "start" load (it is neither a start nor a maximum load: no pressure assumption is made for it).
The bullets are H&N's: a project record of H&N gives the measured length."""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

from samples_common import Records  # noqa: E402

GR = 0.06479891
IN = 25.4
HANDGUN, RIFLE = (10, 13), (16, 17)
NUM = r"(\d+(?:,\d+)?)"
ROW = re.compile(rf"^\s*(?P<form>[A-Z]{{2,3}}(?: CR)?)\s+\.(?P<dia>\d{{3}})\s+(?P<type>HS|fett)\s+(?P<gr>\d{{3}})\s+(?P<col>{NUM}\*?)\s+(?P<case>\S.*?)\s{{2,}}"
                 rf"(?P<primer>\S.*?)\s{{2,}}(?P<maker>[A-Za-z]+)\s+(?P<charge>{NUM})\s+(?P<powder>\S.*?)\s+(?P<crimp>kein|Taper|Roll|Friction|Factory)\s+"
                 rf"(?P<v0>\d+)(?:\s+(?P<rest>\S.*))?$")
CALIBER = re.compile(r"^\s*Kaliber:?\s+(?P<name>\S.*?)\**\s*$")
CALIBERS = {"7,65 Para": "7.65 Parabellum", "9 mm Luger": "9 mm Luger", ".38 Special": "38 Special", ".38 Spezial": "38 Special", ".357 Magnum": "357 Mag.",
            ".44 Magnum": "44 Rem. Mag.", ".45 ACP": "45 Auto", "8 x 57IS": "8 x 57 IS", "7 x 57": "7 x 57", ".30 Carb.": "30 Carbine",
            "7,62 x 54R": "7.62 x 54 R", "7,5 x 55": "7.5 x 55 Suisse", "7,65 Arg.": "7.65 x 53 Arg.", ".303 Brit.": "303 British", "6,5 x 55": "6.5 x 55 SE",
            ".223 Rem.": "223 Rem.", ".222 Rem.": "222 Rem.", ".22 Hornet": "22 Hornet", ".308 Win.": "308 Win.", ".30-30": "30-30 Win.", ".30-06": "30-06 Spring.",
            "8,15 x 46R": "8.15 x 46 R", ".375 H&H": "375 H&H Mag.", ".500 S&W": "500 S&W Mag", ".45/70": "45-70 Govt.", ".44/40": "44-40 Win."}
MAKERS = {"Vihtavuori": "Vihtavuori", "Hodgdon": "Hodgdon", "Rottweil": "RWS-Rottweil", "Vectan": "Vectan", "Alliant": "Alliant", "Accurate": "Accurate", "IMR": "IMR"}


def powder_name(maker: str, powder: str) -> str:
    """The table's spelling -> the name the other sample files use. The catalogue sometimes names the wrong maker (Tin Star, Trail Boss)."""
    p = powder.strip()
    if re.match(r"^(N|P|R|H|Ba|BA|A) ?\d", p) or p in ("AO", "A0"):
        p = re.sub(r"\s+", "", p)
    fixed = {"HP 38": "Hodgdon HP-38", "Am.Selec": "Alliant American Select", "AO": "Vectan A0", "Tin Star": "Hodgdon Tin Star",
             "Trail Boss": "Hodgdon Trail Boss", "No. 2": "Accurate No. 2", "No. 5": "Accurate No. 5", "No. 7": "Accurate No. 7", "No. 9": "Accurate No. 9"}
    if p in fixed:
        return fixed[p]
    return f"{MAKERS.get(maker, maker)} {p.replace('BA', 'Ba') if maker == 'Vectan' else p}"


def pages(pdf: Path, first: int, last: int) -> str:
    return subprocess.run(["pdftotext", "-layout", "-f", str(first), "-l", str(last), str(pdf), "-"], capture_output=True, text=True).stdout


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pdf", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("hn_samples.csv"))
    ap.add_argument("--handgun-barrel", type=float, default=120.0)
    ap.add_argument("--rifle-barrel", type=float, default=600.0)
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    records = Records(args.data)
    rows, miss = [], {}
    for (first, last), barrel in ((HANDGUN, args.handgun_barrel), (RIFLE, args.rifle_barrel)):
        cal = None
        for line in pages(args.pdf, first, last).splitlines():
            m = CALIBER.match(line) or re.match(r"^\s*Kaliber\s+(?P<name>\S.*?)\**\s*$", line)
            if m:
                name = m["name"].strip().rstrip("*").strip()
                cal = records.caliber(CALIBERS.get(name, name))
                if not cal:
                    miss[name] = miss.get(name, 0) + 1
                continue
            r = ROW.match(line)
            if not (r and cal and cal.get("G1")):
                if r and not cal:
                    miss["(row without cartridge)"] = miss.get("(row without cartridge)", 0) + 1
                continue
            dia, gr = cal["G1"], float(r["gr"])
            length, source, tail = records.bullet("H&N", f"{r['form']} HS", gr, dia)
            col = float(r["col"].rstrip("*").replace(",", "."))
            rows.append({"powder": powder_name(r["maker"], r["powder"]), "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""),
                         "bullet": f"H&N {r['form']} {r['type']} {r['dia']}", "bullet_gr": gr, "bullet_dia_mm": dia, "bullet_len_mm": f"{length:.2f}",
                         "len_source": source, "tail_type": tail, "col_mm": f"{col:.2f}", "barrel_mm": f"{barrel:.0f}", "twist_mm": "", "kind": "start",
                         "charge_g": round(float(r["charge"].replace(",", ".")), 4), "velocity_ms": float(r["v0"]), "pressure_bar": "", "primer": r["primer"]})
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} samples -> {args.output}", file=sys.stderr)
    print("cartridges not found:", ", ".join(f"{k} ({v})" for k, v in sorted(miss.items())), file=sys.stderr)
    print("powders:", ", ".join(sorted({r["powder"] for r in rows})), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
