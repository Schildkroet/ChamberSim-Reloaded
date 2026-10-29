#!/usr/bin/env python3
"""Collects all published loads of the sample files (hodgdon_extract.py, lovex_extract.py, barnes_pdf_extract.py, hn_extract.py, ...) in one file per powder maker.

    by_maker.py [--samples sources/samples] [--out sources/samples/by_maker]

A sample file is made per source (a manual, a web page), not per maker: the Hodgdon manual holds the loads of Hodgdon, IMR, Winchester, Accurate and Ramshot powders, a
bullet catalogue (H&N) or a bullet maker's PDF (Barnes) those of nearly all makers. This script sorts every load by the maker of its powder and writes
`<maker>.csv` (columns: those of the source files plus `source`, the name of the file the load comes from; shotshell loads in `<maker>_shot.csv`). Identical loads of several sources (same powder,
cartridge, bullet weight, kind, charge, velocity and cartridge length) are kept once, with the first source. The files are inputs for published_fit.py and the
analysis scripts (fit_physical_powders.py, fit_caliber_scales.py, scan_physical.py, ... take the files as arguments: `by_maker/*.csv` without `*_shot.csv`; compare_physical_published.py
and pressure_scatter.py read the directory by default): a source that is added later (extract it to sources/samples/<name>_samples.csv) only has to be run through this script again."""
import argparse
import csv
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[0]))
from recordfiles import slug  # noqa: E402

PREFIXES = [("Hodgdon", "Hodgdon"), ("IMR", "IMR"), ("Winchester", "Winchester"), ("Accurate", "Accurate"), ("Ramshot", "Ramshot"), ("Alliant", "Alliant"),
            ("ADI", "ADI"), ("Vectan", "Vectan"), ("Vihtavuori", "Vihtavuori"), ("RWS-Rottweil", "RWS-Rottweil"), ("Rottweil", "RWS-Rottweil"), ("Lovex", "Lovex"),
            ("ReloadSwiss", "ReloadSwiss"), ("Norma", "Norma")]
PATTERNS = [(r"^(\d*N\d+\w*)$", "Vihtavuori"), (r"^[SD]\d{3}(\.\d)?$", "Lovex"), (r"^RS\d+", "ReloadSwiss"), (r"^R\d{3}$", "RWS-Rottweil"),
            (r"^(XMP|XMR|LT-\d+)", "Accurate"), (r"^(MRP|URP)\b", "Norma")]  # Barnes' PDFs name some powders without their maker


ALIASES = {"Hodgdon Suprform": "Hodgdon Superformance", "Hodgdon SUPRFORM": "Hodgdon Superformance",  # the manual's own abbreviation
           # the spelling of the Lyman and Hornady handbooks and of the Western / Winchester tables -> the project's record
           "Winchester 231": "Winchester W231", "Winchester 296": "Winchester W296", "Winchester 748": "Winchester W748", "Winchester 760": "Winchester W760",
           "LEVERevolution": "Hodgdon LVR", "BL-C (2)": "Hodgdon BL-C(2)", "CFE223": "Hodgdon CFE 223", "Hodgdon MAGPRO": "Accurate Magpro",
           "Accurate No.9": "Accurate No. 9", "Hodgdon NO. 11FS": "Accurate No. 11 FS", "Accurate No. 11FS": "Accurate No. 11 FS",
           "Accurate 2015 Cup": "Accurate 2015", "Alliant Reloder 10": "Alliant Reloder 10X", "IMR 5744": "Accurate 5744",
           "H-4831SC": "Hodgdon H4831SC", "HP-38": "Hodgdon HP-38", "Lil'Gun": "Hodgdon Lil'Gun", "IMR SR-4756": "IMR SR 4756", "IMR SR-4759": "IMR SR 4759",
           "IMR-4320": "IMR 4320", "NORMA URP": "Norma URP", "NORMA MRP": "Norma MRP", "NORMA 200": "Norma 200", "NORMA 201": "Norma 201",
           "NORMA 202": "Norma 202", "NORMA 203 B": "Norma 203-B", "IMR 800 X": "IMR 800-X", "IMR 700 X": "IMR 700-X",
           "Hodgdon HP-38": "Hodgdon HP-38", "Alliant 300-MP": "Alliant Power Pro 300-MP"}


def maker(powder: str) -> str:
    first = powder.split(" ")[0]
    for prefix, name in PREFIXES:
        if first == prefix:
            return name
    for pattern, name in PATTERNS:
        if re.match(pattern, powder):
            return name
    return first if " " in powder else "other"  # "LT-32" or "MRP" without a maker: its own file "other"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--samples", type=Path, default=HERE / "sources" / "samples")
    ap.add_argument("--out", type=Path)
    args = ap.parse_args()
    out = args.out or args.samples / "by_maker"
    out.mkdir(parents=True, exist_ok=True)
    seen: set[tuple] = set()
    by_maker: dict[str, list[dict]] = {}
    columns: list[str] = []
    files = sorted(p for p in args.samples.glob("*.csv") if "lengths" not in p.name)
    for path in files:
        with open(path, newline="", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            if not reader.fieldnames or "powder" not in reader.fieldnames:
                continue
            for c in reader.fieldnames:
                if c not in columns:
                    columns.append(c)
            for row in reader:
                key = tuple((row.get(k) or "").upper() if k == "powder" else row.get(k) or ""
                            for k in ("powder", "caliber", "bullet_gr", "kind", "charge_g", "velocity_ms", "col_mm", "shot_mass_g"))
                if key in seen:
                    continue
                seen.add(key)
                row["powder"] = ALIASES.get(row["powder"], row["powder"])
                shot = "shot" in path.stem  # shotshell loads (shot mass, no bullet) are kept apart: `<maker>_shot.csv`
                by_maker.setdefault(maker(row["powder"]) + ("_shot" if shot else ""), []).append({**row, "source": path.stem})
    columns.append("source")
    for old in out.glob("*.csv"):
        old.unlink()
    total = 0
    for name, rows in sorted(by_maker.items()):
        used = [c for c in columns if c in ("powder", "caliber", "kind", "source") or any(r.get(c) for r in rows)]  # csr_eval reads shotgun columns as a shotshell load
        with open(out / f"{slug(name)}.csv", "w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, used, extrasaction="ignore")
            w.writeheader()
            w.writerows(rows)
        total += len(rows)
        print(f"{name:14} {len(rows):6} loads, {len({r['powder'] for r in rows}):3} powders, {len({r['source'] for r in rows})} sources")
    print(f"{total} loads of {len(files)} files -> {out}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
