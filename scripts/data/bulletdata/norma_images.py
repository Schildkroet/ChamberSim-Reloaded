#!/usr/bin/env python3
"""Downloads the picture of every Norma component bullet of norma-ammunition.com for measuring bullet lengths (measure_side_photos.py).

    norma_images.py [--out DIR] [--cache DIR]

The site's Sanity content database (norma_fetch.py) lists per bullet product its media assets: the first is a rendering of the bullet on a transparent
background, standing on a tall canvas. The files are saved as `<sku>__w<weight>.png` with `manifest.csv` (file, brand, title, sku, diameter_in,
weight_gr, url, image_url) in --out; files that exist are not downloaded again. Measure them with

    measure_side_photos.py DIR --out measured.csv
"""
import argparse
import csv
import json
import re
import sys
import time
import urllib.parse
import urllib.request
from pathlib import Path

from bulletfetch import UA, get, number

QUERY = '*[_type=="productData" && cartridgeType=="Centerfire Rifle - Bullet"]{sku,name,nameImp,caliber,status,majorFacts,mediaAssets,urlPaths}'
API = "https://6psbvfzb.api.sanity.io/v2021-06-07/data/query/production?query="
CDN = "https://cdn.sanity.io/images/6psbvfzb/production/"


def image_url(ref: str) -> str:
    """image-<id>-<width>x<height>-<ext>  ->  <id>-<width>x<height>.<ext>"""
    m = re.fullmatch(r"image-([0-9a-f]+-\d+x\d+)-(\w+)", ref)
    return f"{CDN}{m[1]}.{m[2]}" if m else ""


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, default=Path("norma_images"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    rows = []
    for it in json.loads(get(API + urllib.parse.quote(QUERY), args.cache))["result"]:
        facts = it.get("majorFacts") or {}
        name = it.get("nameImp") or it.get("name") or ""
        dia = re.search(r"\((\.\d{3})\)", name) or re.search(r"\((\.\d{3})\)", it.get("caliber", ""))
        asset = next((m["asset"]["_ref"] for m in it.get("mediaAssets") or [] if m.get("_type") == "image"), "")
        url = image_url(asset)
        if not (url and dia and facts.get("weight_grain")):
            print("no picture:", name, file=sys.stderr)
            continue
        file = f"{it['sku']}__w{number(str(facts['weight_grain']))}.png"
        target = args.out / file
        if not target.exists():
            time.sleep(0.3)
            target.write_bytes(urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=30).read())
        page = next((u["url"] for u in it.get("urlPaths", []) if u.get("language") == "en"), "")
        rows.append({"file": file, "brand": "Norma", "title": name, "sku": it["sku"], "diameter_in": number(dia[1]), "weight_gr": number(str(facts["weight_grain"])),
                     "url": "https://www.norma-ammunition.com" + page, "image_url": url})
    with open(args.out / "manifest.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} pictures -> {args.out}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
