#!/usr/bin/env python3
"""Reads the component bullets of federalpremium.com into a CSV (bullets_federal.csv).

    federal_fetch.py [-o bullets_federal.csv] [--cache dir]

Walks the bullet listing (page by page) and reads the title of every bullet page ("Fusion Component Bullet, .284, 140
Grain") and its part number. No length and no ballistic coefficient: bullets_to_json.py estimates the length.
"""
import argparse
import re
import sys
from pathlib import Path

from bulletfetch import get, text_lines, write_csv

BASE = "https://www.federalpremium.com"
LISTING = BASE + "/reloading/bullets/"
TITLE = re.compile(r"^(?P<line>.+?),\s*(?P<dia>\.\d{3,4}),\s*(?P<gr>\d+(?:\.\d+)?)\s*Grain", re.I)


def product_urls(cache: Path | None) -> list[str]:
    urls: dict[str, None] = {}
    for start in range(0, 600, 24):
        page = get(f"{LISTING}?start={start}&sz=24", cache)
        found = [u for u in re.findall(r'href="(/reloading/bullets/[^"/]+/[^"]+\.html)"', page) if u not in urls]
        for u in found:
            urls[u] = None
        if not found:
            break
    return [BASE + u for u in urls]


def parse(url: str, page: str) -> dict | None:
    lines = text_lines(page)
    title = next((l for l in lines if TITLE.match(l)), None)
    if not title:
        return None
    m = TITLE.match(title)
    part = lines[lines.index("Part #") + 1] if "Part #" in lines else url.rsplit("/", 1)[1].removesuffix(".html")
    line = re.sub(r"\s*Component Bullet$", "", re.sub(r"^Buy\s+", "", m["line"])).strip()
    return {"manufacturer": "Federal", "line": line, "part": part, "weight_gr": m["gr"], "diameter_in": m["dia"],
            "base_style": "Boat Tail" if re.search(r"fusion|terminal ascent|boat|bt", m["line"], re.I) else "", "ogive_style": m["line"], "url": url}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--output", type=Path, default=Path("bullets_federal.csv"))
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
