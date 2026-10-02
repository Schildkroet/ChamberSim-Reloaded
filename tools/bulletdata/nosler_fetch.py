#!/usr/bin/env python3
"""Reads the bullet specifications published on nosler.com into a CSV (bullets_nosler.csv).

    nosler_fetch.py [-o bullets_nosler.csv] [--cache dir]

Walks the caliber listings of the bullet shop (with their pages) and reads the "Specifications" of every bullet page:
weight, diameter, OAL, G1 BC, base (flat / boat tail), bullet type and SKU. Grain and inch as on the site. One request
every half second; pages are cached in --cache.
"""
import argparse
import re
import sys
from pathlib import Path

from bulletfetch import get, number, text_lines, write_csv

BASE = "https://www.nosler.com"
PRODUCT = re.compile(r'href="(https://www\.nosler\.com/[^"/]*(?:\d+gr|\d+ct)[^"/]*\.html)"')


def product_urls(cache: Path | None) -> list[str]:
    start = get(BASE + "/products/bullets.html", cache)
    listings = sorted(set(re.findall(r'href="(https://www\.nosler\.com/products/bullets/caliber/[^"?]*\.html)"', start)))
    urls: dict[str, None] = {}
    for listing in listings:
        for page in range(1, 30):
            html_ = get(listing if page == 1 else f"{listing}?p={page}", cache)
            found = [u for u in PRODUCT.findall(html_) if u not in urls]
            for u in found:
                urls[u] = None
            if not found:
                break
    return list(urls)


def parse(url: str, page: str) -> dict | None:
    lines = text_lines(page)
    if "Specifications" not in lines:
        return None
    body = lines[lines.index("Specifications"):]
    spec = {}
    for k, v in zip(body, body[1:]):
        if k in ("Manufacturer SKU", "BC G1", "Overall Length (OAL) (in.)", "Diameter", "Bullet Base", "Bullet Profile",
                 "Bullet Type", "Bullet Weight", "Caliber", "Product Name"):
            spec.setdefault(k, v)
    if "Bullet Weight" not in spec:
        return None
    base = spec.get("Bullet Base", "")
    return {"manufacturer": "Nosler", "line": spec.get("Bullet Type", ""), "part": spec.get("Manufacturer SKU", ""),
            "caliber_name": spec.get("Caliber", ""), "weight_gr": number(spec["Bullet Weight"]),
            "diameter_in": number(spec.get("Diameter", "")), "length_in": number(spec.get("Overall Length (OAL) (in.)", "")),
            "g1bc": number(spec.get("BC G1", "")), "base_style": "Boat Tail" if "boat" in base.lower() else base,
            "ogive_style": spec.get("Bullet Profile", ""), "url": url}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_nosler.csv"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    urls = product_urls(args.cache)
    best: dict[tuple, dict] = {}
    for u in urls:
        row = parse(u, get(u, args.cache))
        if not row:
            print("no specifications:", u, file=sys.stderr)
            continue
        key = (row["part"], row["weight_gr"], row["line"])  # the same bullet in 50 and 100 count boxes, and blemish packs
        complete = bool(row["diameter_in"] and row["length_in"])
        if key not in best or (complete and not (best[key]["diameter_in"] and best[key]["length_in"])):
            best[key] = row
    rows = list(best.values())
    write_csv(args.output, rows, needs_length=True)
    print(f"{len(urls)} pages read", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
