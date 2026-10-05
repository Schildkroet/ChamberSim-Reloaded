#!/usr/bin/env python3
"""Reads the rifle bullet table of rws-ammunition.com into a CSV (bullets_rws.csv).

    rws_fetch.py [-o bullets_rws.csv] [--cache dir]

The table lists article number, bullet type, weight (g and gr), diameter and package size. No length and no ballistic
coefficient: bullets_to_json.py estimates the length. Grain and inch.
"""
import argparse
import html
import re
import sys
from pathlib import Path

from bulletfetch import get, number, text_lines, write_csv

URL = "https://rws-ammunition.com/en/products/reloading-components/rifle-cartridge-bullets"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_rws.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    rows = []
    for tr in re.findall(r"<tr[^>]*>(.*?)</tr>", get(URL, args.cache), re.S):
        cells = {k: re.sub(r"\s+", " ", html.unescape(re.sub(r"<[^>]+>", " ", v))).strip()
                 for k, v in re.findall(r'<t[dh][^>]*data-label="([^"]+)"[^>]*>(.*?)</t[dh]>', tr, re.S)}
        if "Article number" not in cells or not re.fullmatch(r"\d+", cells["Article number"]):
            continue
        words = text_lines(re.sub(r"<svg.*?</svg>", "", tr, flags=re.S))  # article number, type, g, gr, diameter, package
        kind = words[words.index(cells["Article number"]) + 1] if cells["Article number"] in words else ""
        rows.append({"manufacturer": "RWS", "line": kind.title() if kind.isupper() else kind, "part": cells["Article number"],
                     "weight_gr": number(cells.get("Weight (gr)", "")), "diameter_in": number(cells.get("Diameter", "")),
                     "base_style": "Boat Tail" if re.search(r"HPBT|MATCH|SCORION|TMS|TMR", kind) else "", "ogive_style": kind, "url": URL})
    write_csv(args.output, rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
