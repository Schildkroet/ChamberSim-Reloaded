#!/usr/bin/env python3
"""Downloads the product pictures of the Lapua bullets of lapua.com for measuring bullet lengths (measure_side_photos.py).

    lapua_images.py [--out DIR] [--cache DIR]

Every bullet page (product list as in lapua_fetch.py) has two renderings of the bullet on a transparent background: one standing (`...-1-1.png`)
and one lying (`...-2-1.png`), both from the side. Of every page the full-size pictures named after the bullet are saved as

    <bullet code>__w<weight gr>__<n>.png

with `manifest.csv` (file, brand, title, sku, diameter_in, weight_gr, url, image_url) in --out; files that exist are not downloaded again. Measure
them with

    measure_side_photos.py DIR --out measured.csv
"""
import argparse
import csv
import json
import re
import sys
import time
import urllib.request
from pathlib import Path

from bulletfetch import UA, get
from lapua_fetch import API, parse

# full-size uploads of the bullet itself; the thumbnails (-300x150), the cartridge pictures and the case or box pictures of "related products" are not wanted
PICTURE = re.compile(r"https://www\.lapua\.com/wp-content/uploads/\d{4}/\d{2}/[^\"' ,)]*\.png")


def pictures(page: str, code: str) -> list[str]:
    urls = []
    for u in PICTURE.findall(page):
        name = u.rsplit("/", 1)[1]
        if re.search(r"-\d+x\d+\.png$", name) or "cartridge" in name.lower() or code.lower() not in name.lower().replace("_", "-"):
            continue
        if u not in urls:
            urls.append(u)
    return urls


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, default=Path("lapua_images"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    products = []
    for pg in range(1, 10):
        chunk = json.loads(get(API + str(pg), args.cache))
        products += chunk
        if len(chunk) < 100:
            break
    rows = []
    for p in products:
        if "bullet" not in p["slug"]:
            continue
        page = get(p["link"], args.cache)
        rec = parse(p["link"], page)
        if not rec:
            continue
        urls = pictures(page, rec["part"])
        if not urls:
            print("no picture:", p["link"], file=sys.stderr)
            continue
        for n, url in enumerate(urls, 1):
            file = f"{rec['part']}__w{rec['weight_gr']}__{n}.png"
            target = args.out / file
            if not target.exists():
                time.sleep(0.3)
                target.write_bytes(urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=30).read())
            rows.append({"file": file, "brand": "Lapua", "title": f"{rec['line']} {rec['part']}", "sku": rec["part"], "diameter_in": rec["diameter_in"],
                         "weight_gr": rec["weight_gr"], "url": p["link"], "image_url": url})
    with open(args.out / "manifest.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} pictures -> {args.out}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
