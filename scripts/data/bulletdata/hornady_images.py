#!/usr/bin/env python3
"""Downloads the side-view illustration of every Hornady bullet on hornady.com for measuring bullet lengths (measure_side_photos.py).

    hornady_images.py [--out DIR] [--cache DIR]

Every bullet page of the site map (hornady.com/bullets/rifle/... and /handgun/...) carries its data as a JSON object (hornady_fetch.py); `thumbnail` is a
rendered picture of the bullet lying on a white background, cropped to the bullet. The pictures are saved as `<sku>__w<weight>.jpg` with `manifest.csv`
(file, brand, title, sku, diameter_in, weight_gr, url, image_url) in --out; files that exist are not downloaded again. Measure them with

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

from bulletfetch import UA, get, number

from hornady_fetch import SITEMAP, clean


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, default=Path("hornady_images"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    urls = [u for u in re.findall(r"<loc>([^<]*)", get(SITEMAP, args.cache)) if re.match(r"https://www\.hornady\.com/bullets/(rifle|handgun)/.+", u)]
    rows, seen = [], set()
    for u in urls:
        try:
            m = re.search(r"var item = (\{.*?\});\s*(?:var|</script)", get(u, args.cache), re.S)
            d = json.loads(m[1]) if m else None
        except (ValueError, OSError):
            d = None
        if not d or not d.get("thumbnail") or not d.get("weight") or not d.get("caliberdiameter"):
            print("no picture:", u, file=sys.stderr)
            continue
        name = f"{d['sku']}__w{number(str(d['weight']))}.jpg"
        if name in seen:
            continue
        seen.add(name)
        target = args.out / name
        if not target.exists():
            time.sleep(0.3)
            try:
                target.write_bytes(urllib.request.urlopen(urllib.request.Request(d["thumbnail"], headers=UA), timeout=30).read())
            except OSError as e:
                print("download failed:", d["thumbnail"], e, file=sys.stderr)
                continue
        rows.append({"file": name, "brand": "Hornady", "title": clean(d.get("title", "")), "sku": d["sku"], "diameter_in": number(d["caliberdiameter"]),
                     "weight_gr": number(str(d["weight"])), "url": u, "image_url": d["thumbnail"]})
    with open(args.out / "manifest.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} pictures of {len(urls)} pages -> {args.out}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
