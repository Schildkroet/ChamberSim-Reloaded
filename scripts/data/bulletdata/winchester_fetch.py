#!/usr/bin/env python3
"""Reads the component bullet tables of winchester.com into a CSV (bullets_winchester.csv).

    winchester_fetch.py [-o bullets_winchester.csv] [--cache dir]

One table per bullet type (Silvertip Hollow Point, Full Metal Jacket, ...) with symbol, caliber, nominal diameter and weight.
No length and no ballistic coefficient: bullets_to_json.py estimates the length. Grain and inch.
"""
import argparse
import re
import sys
from pathlib import Path

from bulletfetch import get, number, text_lines, write_csv

URL = "https://winchester.com/Products/Ammunition/Components/Bullets"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_winchester.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    lines = text_lines(get(URL, args.cache))
    rows, kind, i = [], "", 0
    while i < len(lines):
        if lines[i:i + 4] == ["Symbol", "Caliber", "Nominal Bullet Diameter", "Bullet Weight"]:
            kind = lines[i - 1]
            i += 4
            continue
        if re.fullmatch(r"W[A-Z0-9]{4,}", lines[i]) and i + 3 < len(lines):
            symbol, caliber, dia, weight = lines[i:i + 4]  # "WB44ST210", "44 (.430) Cal", "0", "0": no data
            rows.append({"manufacturer": "Winchester", "line": kind, "part": symbol, "caliber_name": caliber,
                         "weight_gr": number(weight) if weight != "0" else "", "diameter_in": number(dia) if dia != "0" else "",
                         "base_style": "Hollow Base" if "hollow base" in kind.lower() else ("Boat Tail" if re.search(r"boat|\bbt\b|bc max", kind, re.I) else ""), "ogive_style": kind, "url": URL})
            i += 4
            continue
        i += 1
    write_csv(args.output, rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
