#!/usr/bin/env python3
"""Reads the bullet list of sierrabullets.com into a CSV (bullets_sierra.csv).

    sierra_fetch.py [-o bullets_sierra.csv] [--cache dir]

Sierra publishes the caliber, weight, type and item number in the product name ("6.5MM 144 GR HPBT MatchKing (SMK)", SKU 1744);
no length and no ballistic coefficient: bullets_to_json.py estimates the length. The diameter follows from the caliber.
"""
import argparse
import html
import re
import sys
from pathlib import Path

from bulletfetch import get, write_csv

SITEMAP = "https://sierrabullets.com/xmlsitemap.php?type=products&page=1"
# caliber in the product name -> bullet diameter in inch
DIAMETER = {"17": 0.172, "20": 0.204, "22": 0.224, "6MM": 0.243, "25": 0.257, "6.5MM": 0.264, "270": 0.277, "7MM": 0.284,
            "30": 0.308, "303": 0.311, "32": 0.312, "8MM": 0.323, "338": 0.338, "35": 0.358, "9MM": 0.355, "38": 0.357,
            "357": 0.357, "10MM": 0.400, "40": 0.400, "375": 0.375, "41": 0.410, "44": 0.429, "45": 0.451, "416": 0.416,
            "458": 0.458, "223": 0.224, "50": 0.510, "5.7MM": 0.224, "45-70": 0.458}
NAME = re.compile(r"^(?P<cal>[\d.]+(?:-\d+)?(?:MM| CAL)?)\s+(?P<gr>\d+)\s*GR\s+(?P<style>.+)$", re.I)


def parse(url: str, page: str) -> dict | None:
    m = re.search(r'"@type": "Product".*?"name": "([^"]+)",\s*"sku": "(\w*)"', page, re.S)
    if not m:
        return None
    name, sku = html.unescape(m[1]).strip(), m[2]
    n = NAME.match(name)
    if not n:
        return None
    cal = re.sub(r"\s*CAL$", "", n["cal"].upper())
    dia = DIAMETER.get(cal)
    if not dia:
        return None
    style = n["style"]
    line = re.sub(r"\(.*?\)", "", style).strip()
    return {"manufacturer": "Sierra", "line": line, "part": sku, "caliber_name": n["cal"], "weight_gr": n["gr"], "diameter_in": dia,
            "base_style": "Boat Tail" if re.search(r"BT|boat", style, re.I) else "", "ogive_style": style, "url": url}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_sierra.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    urls = [u for u in re.findall(r"<loc>([^<]*)", get(SITEMAP, args.cache)) if re.search(r"-\d+-gr-", u)]
    rows = []
    for u in urls:
        row = parse(u, get(u, args.cache))
        if row:
            rows.append(row)
        else:
            print("not a bullet:", u, file=sys.stderr)
    write_csv(args.output, rows)
    print(f"{len(urls)} pages read", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
