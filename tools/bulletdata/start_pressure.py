#!/usr/bin/env python3
"""Estimates the initial gas pressure (bullet start pressure, field gpressure) of a bullet from its construction.

    start_pressure.py [--data data] [--write] [--overwrite]

The start pressure is the gas pressure at which the bullet leaves the case and engraves into the rifling. It depends on how hard the
bullet is to engrave. The classes and values follow the pattern of the original program's bullet database (computed from it, no values
copied) and were checked against published pressures (see CLASSES): lead (cast, swaged, plated) 150 bar, jacketed bullets 250 bar (also
handgun bullets: GRT's 150 bar made those results worse), brass monolithic 290 bar, copper monolithic 350 bar. The class comes from the maker and the product name; handgun bullets from the diameter (pistol and revolver
calibers) or from the cartridge when it is known (estimate(..., handgun=True)).

Without --write the classes are printed; --write puts gpressure into the records of data/projectiles that have none (--overwrite: all).
"""
import argparse
import json
import re
import sys
from pathlib import Path

# Checked with the physical model against the published pressures (held-out cartridges, burn values refitted): copper 350 bar instead of
# 250 lowers the median peak pressure error 7.6 -> 6.3 % (bias -5.1 -> -1.3 %); lead (GRT 80 bar) is best at 150 bar (bias +4.1 % with
# 250, -2.4 % with 150, -9.9 % with 80; only 77 loads); jacketed handgun bullets (GRT 150 bar) stay at 250 (150: median 8.1 -> 10.0 %).
CLASSES = {"lead": 150.0, "handgun": 250.0, "jacketed": 250.0, "brass": 290.0, "copper": 350.0}

LEAD_MAKERS = {"h&n", "lyman", "lee", "hw-wiederladen", "wm-bullets", "rcbs", "hornady lead"}
LEAD_WORDS = r"\b(lead|cast|plated|swaged|lrn|lfn|lswc|swc|wc|hbwc|dewc|lrnfp|lfp|hard ?cast|bevel ?base|plated)\b"
COPPER_WORDS = (r"\b(tsx|ttsx|lrx|tac-?xp?|banded|cx|gmx|monoflex|mono ?flex|e-?tip|naturalis|ecostrike|powerhead|trophy copper|"
                r"cutting edge|lehigh|solid copper|copper solid|monolithic|all[- ]copper|hit|raptor|impala|evo green|x-bullet|xpb|"
                r"bullet x|gs custom|tac-tx|vor-tx|copper impact|solid)\b")
BRASS_WORDS = r"\b(peregrine|brass|bushmaster brass|vlr|plainsmaster)\b"
# pistol and revolver bullet diameters [inch]
HANDGUN_DIAMETERS = {".251", ".312", ".355", ".356", ".357", ".363", ".400", ".401", ".410", ".429", ".430", ".451", ".452", ".475",
                     ".476", ".500"}


def estimate(maker: str, name: str, diameter_in: str = "", handgun: bool | None = None) -> tuple[float, str]:
    """(start pressure in bar, class) of a bullet. `handgun`: known from the cartridge (None: from the diameter)."""
    text = f"{maker} {name}".lower()
    # Barnes: monolithic copper, except the lead-core Burner, Original, Buster and Varminator and the powder-core MPG / Varmint Grenade
    barnes = maker.lower() == "barnes" and not re.search(r"burner|original|buster|varminator|mpg|varmint grenade", text)
    if re.search(r"lead[- ]?free", text) or barnes:
        cls = "copper"
    elif maker.lower() in LEAD_MAKERS or re.search(LEAD_WORDS, text):
        cls = "lead"
    elif re.search(BRASS_WORDS, text):
        cls = "brass"
    elif re.search(COPPER_WORDS, text):
        cls = "copper"
    else:
        cls = "jacketed"
    if handgun is None:
        handgun = diameter_in.strip() in HANDGUN_DIAMETERS
    if handgun and cls in ("jacketed", "copper"):  # handgun bullets engrave with less force, also the copper ones
        cls = "handgun"
    return CLASSES[cls], cls


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--data", default=str(Path(__file__).resolve().parents[2] / "data"))
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--overwrite", action="store_true")
    args = ap.parse_args()
    counts = {}
    for f in sorted((Path(args.data) / "projectiles").glob("*.json")):
        record = json.loads(f.read_text(encoding="utf-8"))
        bar, cls = estimate(record.get("mname", ""), record.get("pname", ""), record.get("caliber", ""))
        counts[cls] = counts.get(cls, 0) + 1
        if not args.write:
            print(f"{cls:9s} {bar:5.0f}  {record.get('mname', '')} {record.get('pname', '')} ({record.get('caliber', '')})")
            continue
        if record.get("gpressure") and not args.overwrite:
            continue
        # after the mass, in the records' format (indent 2, schema order)
        out = {}
        for k, v in record.items():
            if k != "gpressure": out[k] = v
            if k == "gmass": out["gpressure"] = bar
        out.setdefault("gpressure", bar)
        f.write_text(json.dumps(out, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(counts, file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
