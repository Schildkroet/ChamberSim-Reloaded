#!/usr/bin/env python3
"""Reads the bullet components listed on barnesbullets.com into a CSV (bullets_barnes.csv).

    barnes_fetch.py [-o bullets_barnes.csv] [--cache dir]

The product names of the component bullets carry what the site publishes: diameter, line, weight and base
("0.410\\" 450-400 CAL TSX 400 GR FB"). No length and no ballistic coefficient: bullets_to_json.py estimates the length.
"""
import argparse
import html
import re
import sys
from pathlib import Path

from bulletfetch import get, write_csv

SITEMAP = "https://www.barnesbullets.com/xmlsitemap.php?type=products&page=1"
NAME = re.compile(r'^(?P<dia>0\.\d{3})"?\s+(?P<cal>.+?)\s+(?P<line>TSX|TTSX|LRX|TAC-?XP|TAC-?TX|VOR-?TX|XPB|MRX|BANDED|[A-Z-]+)\s+(?P<gr>\d+)\s*GR\s*(?P<base>FB|BT|FN|SP)?\b', re.I)


def parse(url: str, page: str) -> dict | None:
    t = re.search(r"<title>(.*?)</title>", page, re.S)
    name = html.unescape(t[1]).rsplit(" - Barnes", 1)[0].strip() if t else ""
    m = NAME.match(name)
    if not m:
        return None
    base = (m["base"] or "").upper()
    return {"manufacturer": "Barnes", "line": m["line"].upper(), "part": re.search(r'"sku":\s*"(\w+)"', page)[1] if re.search(r'"sku":\s*"(\w+)"', page) else "",
            "caliber_name": m["cal"], "weight_gr": m["gr"], "diameter_in": m["dia"], "base_style": "Boat Tail" if base == "BT" else ("Flat Base" if base == "FB" else ""),
            "ogive_style": name, "url": url}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_barnes.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    urls = re.findall(r"<loc>([^<]*)", get(SITEMAP, args.cache))
    rows = []
    for u in urls:
        if not re.search(r"barnesbullets\.com/0-\d{3}-", u):  # components only, not apparel and ammunition
            continue
        row = parse(u, get(u, args.cache))
        if row:
            rows.append(row)
    write_csv(args.output, rows)
    print(f"{len(urls)} pages listed", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
