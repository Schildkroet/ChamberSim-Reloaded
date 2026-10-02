#!/usr/bin/env python3
"""Reads the bullets of remington.com's handloading page into a CSV (bullets_remington.csv).

    remington_fetch.py [-o bullets_remington.csv] [--cache dir]

Walks the bullet listing (page by page); the page title of every bullet carries line, diameter and weight ("Core-Lokt Rifle
Bullet, .257, 100 Grain"), the part number is in the address. No length and no ballistic coefficient:
bullets_to_json.py estimates the length. Grain and inch.
"""
import argparse
import html
import re
import sys
from pathlib import Path

from bulletfetch import get, write_csv

BASE = "https://www.remington.com"
LISTING = BASE + "/handloading/bullets/"
TITLE = re.compile(r"^(?:Buy\s+)?(?P<line>.+?),\s*(?P<dia>\.\d{3,4}),\s*(?P<gr>\d+(?:\.\d+)?)\s*Grain", re.I)


def product_urls(cache: Path | None) -> list[str]:
    urls: dict[str, None] = {}
    for start in range(0, 600, 24):
        page = get(f"{LISTING}?start={start}&sz=24", cache)
        found = [u for u in re.findall(r'href="(/handloading/bullets/[^"/?]+\.html)"', page) if u not in urls]
        for u in found:
            urls[u] = None
        if not found:
            break
    return [BASE + u for u in urls]


def parse(url: str, page: str) -> dict | None:
    t = re.search(r"<title>(.*?)</title>", page, re.S)
    m = TITLE.match(html.unescape(t[1]).strip()) if t else None
    if not m:
        return None
    line = re.sub(r"\s*(Rifle|Handgun)?\s*Bullets?$", "", m["line"]).strip()
    return {"manufacturer": "Remington", "line": line, "part": url.rsplit("/", 1)[1].removesuffix(".html"), "weight_gr": m["gr"],
            "diameter_in": m["dia"], "base_style": "Boat Tail" if re.search(r"boat|\\bbt\\b|bthp|ballistic tip|accutip", m["line"], re.I) else "",
            "ogive_style": m["line"], "url": url}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_remington.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    rows = []
    for u in product_urls(args.cache):
        row = parse(u, get(u, args.cache))
        if row:
            rows.append(row)
        else:
            print("no title:", u, file=sys.stderr)
    write_csv(args.output, rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
