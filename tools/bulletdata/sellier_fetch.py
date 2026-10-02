#!/usr/bin/env python3
"""Reads the component bullet table of sellier-bellot.cz into a CSV (bullets_sellier.csv).

    sellier_fetch.py [-o bullets_sellier.csv] [--cache dir]

The table lists product number, cartridge, type (FMJ, JHP, SP, LRN, WC, ...), weight and diameter. No length and no
ballistic coefficient: bullets_to_json.py estimates the length. Grain and inch.
"""
import argparse
import html
import re
import sys
from pathlib import Path

from bulletfetch import get, write_csv

URL = "https://www.sellier-bellot.cz/en/products/components/bullets/"


def cells(row: str) -> list[str]:
    return [re.sub(r"\s+", " ", html.unescape(re.sub(r"<[^>]+>", " ", c)).replace("\xa0", " ")).strip()
            for c in re.findall(r"<t[dh][^>]*>(.*?)</t[dh]>", row, re.S)]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_sellier.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    rows, recommend = [], ""
    for tr in re.findall(r"<tr[^>]*>(.*?)</tr>", get(URL, args.cache), re.S):
        c = cells(tr)
        i = next((k for k, v in enumerate(c) if re.fullmatch(r"\.\d{3}", v)), None)  # the diameter in inch
        if i is None or i < 5 or not re.fullmatch(r"V\d+", c[0]):
            continue
        if i - 5 > 1:  # the "recommended for" cell is left out when it repeats the row above
            recommend = c[1]
        grs, kind = c[i - 4], c[i - 5]
        if not grs.isdigit():
            continue
        rows.append({"manufacturer": "Sellier & Bellot", "line": kind, "part": c[0], "caliber_name": recommend, "weight_gr": grs,
                     "diameter_in": c[i], "base_style": "Boat Tail" if re.search(r"BT|boat", kind, re.I) else "",
                     "ogive_style": f"{kind} {recommend}", "url": URL})
    write_csv(args.output, rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
