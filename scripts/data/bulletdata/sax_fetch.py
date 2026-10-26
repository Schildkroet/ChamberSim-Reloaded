#!/usr/bin/env python3
"""Reads the bullets of sax-munition.de into a CSV (bullets_sax.csv).

    sax_fetch.py [-o bullets_sax.csv] [--cache dir]

Walks the listing of the component bullets (page by page); every product block lists the part number, the ballistic coefficient, the
diameter (mm / caliber), the weight (g) and the length (mm). The CSV has the values in grain and inch like the other fetch scripts;
apply_csv_lengths.py --validated puts the lengths into the records.
"""
import argparse
import html
import re
import sys
from pathlib import Path

from bulletfetch import get, number, write_csv

BASE = "https://www.sax-munition.de"
LISTING = BASE + "/geschosse/"
MM = 25.4
GRAIN_G = 0.06479891
BLOCK = re.compile(r'<div class="card h-100 product-box.*?</table>', re.S)
FIELD = re.compile(r"<td>\s*([^<]*?)\s*</td>\s*<td>\s*([^<]*?)\s*</td>", re.S)
TITLE = re.compile(r'<a\s+href="([^"]+)"\s+class="product-name"\s+title="([^"]*)"')
CALIBER = re.compile(r"^.*?\(\.[^)]*\)\s*")  # ".22 Hornet (.223) KJG-HS (2,4 g)" -> "KJG-HS (2,4 g)"
WEIGHT = re.compile(r"\s*\(\d+,\d+ g\)$")


def listing_pages(cache: Path | None) -> list[str]:
    pages = []
    for p in range(1, 30):
        page = get(f"{LISTING}?p={p}", cache)
        if p > 1 and page == pages[-1]:
            break
        pages.append(page)
        if f'for="p{p + 1}"' not in page:
            break
    return pages


def parse(block: str) -> dict | None:
    title = TITLE.search(block)
    spec = {k: html.unescape(v) for k, v in FIELD.findall(block)}
    if not title or "Geschossgewicht" not in spec or "Diameter" not in spec:
        return None
    name = html.unescape(title[2])
    mm = number(spec["Diameter"].replace(",", ".").split("/")[0])
    grams = number(spec["Geschossgewicht"])
    length = number(spec.get("Geschosslänge", ""))
    line = WEIGHT.sub("", CALIBER.sub("", name)) or name
    return {"manufacturer": "Sax", "line": line, "part": spec.get("Artikel", ""),
            "caliber_name": name, "weight_gr": f"{float(grams) / GRAIN_G:.1f}", "diameter_in": f"{float(mm) / MM:.4f}",
            "length_in": f"{float(length) / MM:.5f}" if length and float(length) > 0 else "",
            "g1bc": number(spec.get("BC", "")), "url": html.unescape(title[1])}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_sax.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    rows = {}
    for page in listing_pages(args.cache):
        for block in BLOCK.findall(page):
            row = parse(block)
            if row:
                rows[row["part"] or row["url"]] = row
            else:
                print("not parsed:", block[:80].replace("\n", " "), file=sys.stderr)
    write_csv(args.output, list(rows.values()), needs_length=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
