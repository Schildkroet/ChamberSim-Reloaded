#!/usr/bin/env python3
"""Downloads the product photo of every Sierra bullet on sierrabullets.com for measuring bullet lengths (measure_side_photos.py).

    sierra_images.py [--out DIR] [--cache DIR]

The first photo of a product page is a side view of the bullet on white with the line's coloured banner and logo below it; the banner is cut off (the
first row below which nearly every pixel is not white) and the rest saved as `<sku>__w<weight>.png` with `manifest.csv` (file, brand, title, sku,
diameter_in, weight_gr, url, image_url) in --out. Needs OpenCV; files that exist are not downloaded again. Measure them with

    measure_side_photos.py DIR --out measured.csv
"""
import argparse
import csv
import re
import sys
import time
import urllib.request
from pathlib import Path

import cv2
import numpy as np

from bulletfetch import UA, get
from sierra_fetch import SITEMAP, parse


def crop_banner(img: np.ndarray) -> np.ndarray:
    """The part above the banner: rows from the top while the row is not (nearly) all coloured."""
    bgr = img[..., :3]
    coloured = (bgr.min(axis=2) < 235).mean(axis=1)  # share of non-white pixels per row
    full = np.nonzero(coloured > 0.9)[0]
    return img[:full[0]] if len(full) else img


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, default=Path("sierra_images"))
    ap.add_argument("--cache", type=Path)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    if args.cache:
        args.cache.mkdir(parents=True, exist_ok=True)
    urls = [u for u in re.findall(r"<loc>([^<]*)", get(SITEMAP, args.cache)) if re.search(r"-\d+-gr-", u)]
    rows = []
    for u in urls:
        page = get(u, args.cache)
        row = parse(u, page)
        photos = [p for p in re.findall(r"https://cdn11\.bigcommerce\.com/[^\"' ]+?/1280x1280/products/[^\"' ]+?\.jpg\?c=1", page) if "Box" not in p]
        if not row or not photos:
            print("no photo:", u, file=sys.stderr)
            continue
        name = f"{row['part']}__w{row['weight_gr']}.png"
        target = args.out / name
        if not target.exists():
            time.sleep(0.3)
            try:
                data = urllib.request.urlopen(urllib.request.Request(photos[0], headers=UA), timeout=30).read()
            except OSError as e:
                print("download failed:", photos[0], e, file=sys.stderr)
                continue
            img = cv2.imdecode(np.frombuffer(data, np.uint8), cv2.IMREAD_COLOR)
            if img is None:
                print("not an image:", photos[0], file=sys.stderr)
                continue
            cv2.imwrite(str(target), crop_banner(img))
        rows.append({"file": name, "brand": "Sierra", "title": row["ogive_style"], "sku": row["part"], "diameter_in": row["diameter_in"],
                     "weight_gr": row["weight_gr"], "url": u, "image_url": photos[0]})
    with open(args.out / "manifest.csv", "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} photos of {len(urls)} pages -> {args.out}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
