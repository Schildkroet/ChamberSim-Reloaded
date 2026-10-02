#!/usr/bin/env python3
"""Reads the bullet specifications published on bergerbullets.com into a CSV (bullets_berger.csv).

    berger_fetch.py [-o bullets_berger.csv] [--cache dir]

Walks the product sitemap and reads the "Specifications" list of every product page (weight, diameter, OAL, G1/G7 BC,
base style, boat tail length, part number). Values stay in the units of the site: grain and inch. Be polite: one
request every half second; pages are cached in --cache so a rerun does not fetch them again.
"""
import argparse
import re
import sys
from pathlib import Path

from bulletfetch import get, number, text_lines, write_csv

SITEMAP = "https://bergerbullets.com/product-sitemap.xml"


def parse(url: str, page: str) -> dict | None:
    lines = text_lines(page)
    if "Specifications" not in lines:
        return None
    i = lines.index("Specifications")
    end = next((j for j in range(i, len(lines)) if lines[j].startswith("View All Bullet")), i + 60)
    body = lines[i + 1:end]
    spec = {body[k]: body[k + 1] for k in range(0, len(body) - 1, 2)}
    if "Bullet Dia." not in spec and "OAL" not in spec:  # ammunition pages
        return None
    return {"manufacturer": "Berger", "line": spec.get("Product Line", ""), "part": spec.get("100ct Part #", ""),
            "caliber_name": spec.get("Caliber", ""), "weight_gr": number(spec.get("Bullet Weight", "")),
            "diameter_in": number(spec.get("Bullet Dia.", "")), "length_in": number(spec.get("OAL", "")), "g1bc": number(spec.get("G1 BC", "")),
            "g7bc": number(spec.get("G7 BC", "")), "base_style": spec.get("Base Style", ""),
            "ogive_style": spec.get("Ogive Style", ""), "boat_tail_length_in": number(spec.get("Boat Tail Length", "")),
            "discontinued": "yes" if "discontinued" in url else "", "url": url}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_berger.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    urls = [u for u in re.findall(r"<loc>([^<]*)", get(SITEMAP, args.cache)) if "/product/" in u]
    rows = []
    for u in urls:
        row = parse(u, get(u, args.cache))
        if row:
            rows.append(row)
        else:
            print("no specifications:", u, file=sys.stderr)
    write_csv(args.output, rows, needs_length=True)
    print(f"{len(urls)} pages read", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
