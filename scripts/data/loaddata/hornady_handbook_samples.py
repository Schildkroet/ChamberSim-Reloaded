#!/usr/bin/env python3
"""Turns the CSV of hornady_handbook_extract.py into fit samples (CSV for csr_fitdata), like barnes_pdf_extract.py.

    hornady_handbook_samples.py hornady_handbook_loads.csv [-o hornady_samples.csv]

One sample per table cell. The table of a weight group belongs to all its bullets; the mean mass and the first bullet's type, length and
cartridge length stand for the group. A red (maximum) cell is kind "max", all others "table". Cartridges that the project does not know and
bullets whose diameter does not fit are skipped and listed on stderr.
"""
import argparse
import csv
import json
import re
import sys
import unicodedata
from collections import Counter
from pathlib import Path

from barnes_pdf_extract import powder_name  # noqa: E402
from samples_common import IN, Records  # noqa: E402

GRAIN_G = 0.06479891
FPS = 0.3048
ALIASES = {"6.5 X 55mm Swedish": "6.5 x 55 SE", "7mm-08 Remington": "7 mm-08 Rem.", "30-06 Springfield": "30-06 Spring.",
           "5.56mm NATO": "223 Rem.", "223 Remington Service Rifle Data": "223 Rem.", "308 Winchester Service Rifle": "308 Win.",
           "M1 Garand": "30-06 Spring.", "7 X 57mm Mauser": "7 x 57", "8 X 57mm Mauser": "8 x 57 IS", "300 Blackout": "300 AAC Blackout",
           "30 M1 Carbine": "30 Carbine", "30 M1 Carbine (T/C)": "30 Carbine", "9 X 18mm Makarov": "9 mm Makarov",
           "7.62 X 25mm Tokarev": "7.62 x 25 Tokarev", "7.62 X 54R": "7.62 x 54 R", "7.62 X 39mm, M43": "7.62 x 39",
           "50 BMG": "50 Browning", "45 Automatic": "45 Auto", "380 Automatic (9mm Kurz)": "9 mm Browning court", "25 Automatic": "6.35 mm Browning",
           "32 Automatic (7.65mm Browning)": "7.65 mm Browning", "38 Super Automatic": "38 Super Auto", "44 Special": "44 S&W Special",
           "40 Smith & Wesson": "40 S.& W.", "45 Colt (Revolver)": "45 Colt", "7.5 X 55mm Schmidt Rubin (7.5mm Swiss)": "7.5 x 55 Suisse",
           "7.62 X 54R Russian": "7.62 x 54 R", "8 X 57mm Mauser": "8 x 57 IS", "6.5 X 54mm Mannlicher-Schoenauer": "6.5 x 54 Mannl. Sch.",
           "7 X 64mm Brenneke": "7 x 64", "7.7 X 58mm Japanese": "7.7x58 Arisaka", "357 Remington Maximum": "357 Rem. Max.",
           "303 British": "303 British", "450 Marlin": "450 Marlin", "375 Super Magnum": "375 Ruger",
           "22 PPC": "22 PPC-USA", "25 Automatic": "6.35 Browning", "300 Ruger Compact Magnum": "300 RCM", "338 Ruger Compact Magnum": "338 RCM",
           "325 Winchester Short Magnum": "325 WSM", "32 Winchester Special": "32 Win. Spec.", "404 Jeffery": "404 Riml. N.E.",
           "41 Action Express": "41 ACT EXP", "450 Nitro Express 3\u00bc\"": "450 N.E. 3 1/4\"", "460 Smith & Wesson Magnum": "460 S&W Mag",
           "500 Smith & Wesson Magnum": "500 S&W Mag", "505 Gibbs": "505 Mag. Gibbs", "50 Action Express": "50 AE",
           "5.6 X 50mm Magnum": "5.6 x 50 Mag.", "5.6 X 57mm RWS": "5.6 x 57", "5.6 X 52mm (22 Savage High Power)": "5.6 x 52 R",
           "6.5 X 57mm": "6.5 x 57", "6.5 X 50mm Japanese": "6.5 x 50 R", "7.65 X 53mm Belgian Mauser": "7.65 x 53 Arg.",
           "7.7 X 58mm Japanese": "7.7x58 Arisaka", "7mm Shooting Times Westerner": "7 mm STW", "38 Automatic": "38 Auto",
           "357 Remington Maximum": "357 Maximum", "357 Remington Maximum (T/C)": "357 Maximum",
           "7mm Remington Short Action Ultra Magnum": "7 mm Rem. SA Ultra Mag.", "44 Auto Magnum": "44 Auto Mag.",
           "338-06": "338-06", "6.5mm-06": "6.5-06", "8mm-06": "8mm-06", "22 K Hornet": "22 K-Hornet", "22 Bench Rest Remington": "22 BR Rem.",
           "6.5mm-284": "6.5-284 Win."}
ABBREV = (("Remington", "Rem."), ("Winchester", "Win."), ("Magnum", "Mag."), ("Weatherby", "Weath."), ("Smith & Wesson", "S.& W."),
          ("Automatic", "Auto"), ("Short Action Ultra", "SA Ultra"), ("Ultra Magnum", "Ultra Mag."), ("Short Magnum", "Short Mag."),
          ("Compact Magnum", "Compact Mag."), ("Nitro Express", "N.E."), (" Government", " Govt."))


POWDERS = {"WIN Supreme 780": "Hodgdon Supreme 780", "WIN AutoComp": "Hodgdon AutoComp", "BE-86": "Alliant BE-86",
           "CFE Pistol": "Hodgdon CFE Pistol", "AMERICAN SELECT": "Alliant American Select", "Power Pro Varmint": "Alliant Varmint",
           "SOLO 1000": "Accurate Solo 1000", "VIHT 3N37": "Vihtavuori 3N37", "VIHT 20N29": "Vihtavuori 20N29", "VIHT 24N41": "Vihtavuori 24N41",
           "WIN WSF": "Hodgdon WSF", "Vectan 3000": "Vectan A1", "Vectan SP 3": "Vectan SP3", "Vectan SP 2": "Vectan SP2",
           "Vectan A1": "Vectan A1", "Vectan AO": "Vectan A0", "Vectan 5000": "Vectan A24"}


def clean_powder(raw: str) -> str:
    """The row label with the stray caliber words of the page ('257 IMR 7828 SSC', 'Luger CFE Pistol', '19mm, TITEGROUP') removed."""
    name = re.sub(r"^(?:\d+(?:mm)?,? |X |Luger |Remington |Schmidt |Alliant |338-378 |222 |44 )+(?=\S)", "", raw.strip())
    name = re.sub(r"^(\d+mm|\d{3}) (?=[A-Za-z])", "", name)
    return POWDERS.get(name, POWDERS.get(raw, name))


def find_caliber(records, title: str):
    base = re.sub(r"\s*\([^)]*\)", "", title).strip()
    cands = [ALIASES.get(title, title), ALIASES.get(base, base), base]
    for n in list(cands):
        for a, b in ABBREV:
            n = n.replace(a, b)
        cands.append(n)
        cands.append(re.sub(r"\s*[xX]\s*", " x ", n))
    for n in cands:
        if rec := records.caliber(n):
            return rec
    return None


def number(text: str) -> float | None:
    """'23\u215d', '8\u00be', '26' -> inch (vulgar fractions included)."""
    m = re.fullmatch(r"(\d+(?:\.\d+)?)?\s*(\D)?", text.strip())
    if not m or not (m[1] or m[2]):
        return None
    return float(m[1] or 0) + (unicodedata.numeric(m[2]) if m[2] else 0.0)


def barrel_in(text: str):
    """(barrel length in inch, twist in inch) of '23\u00be", 1 in 10" Twist' or '35gr-60gr: 26", 1 in 14" Twist'."""
    m = re.search(r'(\d+(?:\.\d+)?\s*[^\d\s".]?)\s*"', re.sub(r"^\W*\d+gr-\d+gr:", "", text))
    twist = re.search(r'1 in (\d+(?:\.\d+)?[^\d\s"]?)"', text)
    return (number(m[1]) if m else None), (number(twist[1]) if twist else None)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("loads", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("hornady_samples.csv"))
    ap.add_argument("--lengths", type=Path, help="measured.csv of measure_side_photos.py on the bullet pictures")
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    records = Records(args.data)
    known = {f"{p['mname']} {p['pname']}" for f in (args.data / "propellants").glob("*.json") for p in json.loads(f.read_text(encoding="utf-8"))}
    photo = {}
    if args.lengths:
        photo = {re.sub(r"\W+", "", r["title"]).lower() + r["diameter_in"]: float(r["length_in"]) * IN
                 for r in csv.DictReader(args.lengths.open(encoding="utf-8")) if not r["flag"]}
    rows, miss, unknown = [], Counter(), Counter()
    for r in csv.DictReader(args.loads.open(encoding="utf-8")):
        cal = find_caliber(records, r["cartridge"])
        if not cal:
            miss[r["cartridge"]] += 1
            continue
        masses = [float(m) for m in re.findall(r"(\d+(?:\.\d+)?) gr\.", r["bullets"])]
        first = r["bullets"].split(" | ")[0]
        if not masses:
            continue
        grains = sum(masses) / len(masses)
        btype = re.sub(r"^\d+(?:\.\d+)? gr\. ", "", first)
        dia = float(r["diameter_in"]) * IN
        length, source, tail = records.bullet("Hornady", btype, masses[0], dia)
        if (meas := photo.get(re.sub(r"\W+", "", first).lower() + f"{dia / IN:.3f}")) and abs(meas / length - 1) < 0.3:
            length, source = meas, "photo:hornady-handbook"  # the picture counts only when it is near the record or the estimate
        barrel, twist = barrel_in(r["barrel"])
        col = float(r["col_in"] or r["max_col_in"] or 0) * IN
        if not (barrel and col):
            miss[r["cartridge"] + " (barrel/COL)"] += 1
            continue
        pname = powder_name(clean_powder(r["powder"]))
        if pname not in known:
            unknown[r["powder"]] += 1
        rows.append({"powder": pname, "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""), "bullet": f"Hornady {btype}",
                     "bullet_gr": f"{grains:.1f}", "bullet_dia_mm": f"{dia:.3f}", "bullet_len_mm": f"{length:.2f}", "len_source": source,
                     "tail_type": tail, "col_mm": f"{col:.2f}", "barrel_mm": f"{barrel * IN:.0f}",
                     "twist_mm": f"{twist * IN:.0f}" if twist else "", "kind": "max" if r["maximum"] == "1" else "table",
                     "charge_g": round(float(r["charge_gr"]) * GRAIN_G, 4), "velocity_ms": round(int(r["velocity_fps"]) * FPS, 1),
                     "pressure_bar": "", "primer": r["primer"]})
    with args.output.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0]))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} samples -> {args.output}", file=sys.stderr)
    print("cartridges skipped:", "; ".join(f"{k} ({v})" for k, v in sorted(miss.items())), file=sys.stderr)
    print("powders without a project record:", "; ".join(f"{k} ({v})" for k, v in unknown.most_common()), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
