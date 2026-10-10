#!/usr/bin/env python3
"""Bullet lengths from the product photos of grauwolf.net, for the rows of bullets_grauwolf.csv (needs numpy and opencv-python-headless).

    image_length.py [bullets_grauwolf.csv] [--images dir] [--cache dir]

The photos are 2048 x 1536, white background, one to three bullets, mostly standing. The longest dark/coloured blob that does not touch
the border is taken for a bullet; its minimum-area rectangle gives length / width, and with the known diameter (the width is the bearing
diameter) the length in mm. Checked against 31 Berger and Speer bullets with published lengths: median +2.9 % (perspective: the view
from slightly above adds the base face), most within 4 %; some photos fail (a bullet lying, packaging), so a photo length is used only
when it lies within 12 % of the estimated length corrected by the maker's median photo / estimate ratio (per bullet class). Other
rows keep the estimate times that ratio. Both are written to the CSV (length_in, length_note); bullets_to_json.py puts the note into
`descr`. The photos are downloaded once (0.4 s apart) into --images.
"""
import argparse
import csv
import html
import re
import statistics
import sys
import time
import urllib.request
from pathlib import Path

import cv2
import numpy as np

from bulletfetch import COLUMNS, UA
from estimate_length import classify, length_in
import grauwolf_fetch as gf

MEDIA = "https://www.grauwolf.net/media/catalog/product/"
BIAS = 1.03  # median of photo / published length of the check bullets
TOLERANCE = 0.12


def photo(rel: str, images: Path) -> Path | None:
    f = images / re.sub(r"[^\w.-]", "_", rel)
    if not f.exists():
        time.sleep(0.4)
        try:
            f.write_bytes(urllib.request.urlopen(urllib.request.Request(MEDIA + rel, headers=UA), timeout=30).read())
        except OSError:
            return None
    return f


def ratio(path: Path) -> float | None:
    """Length / width of the most elongated bullet in the photo."""
    im = cv2.imread(str(path))
    if im is None:
        return None
    im = cv2.resize(im, None, fx=1000 / max(im.shape[:2]), fy=1000 / max(im.shape[:2]), interpolation=cv2.INTER_AREA)
    hsv = cv2.cvtColor(im, cv2.COLOR_BGR2HSV)
    mask = ((cv2.cvtColor(im, cv2.COLOR_BGR2GRAY) < 205) | (hsv[..., 1] > 45)).astype(np.uint8)
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, np.ones((9, 9), np.uint8))
    n, labels, stats, _ = cv2.connectedComponentsWithStats(mask)
    best = None
    for i in range(1, n):
        x, y, w, h, area = stats[i]
        if area < 1500 or x <= 2 or y <= 2 or x + w >= im.shape[1] - 2 or y + h >= im.shape[0] - 2:
            continue
        contours, _ = cv2.findContours((labels == i).astype(np.uint8), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)
        _, (rw, rh), _ = cv2.minAreaRect(max(contours, key=cv2.contourArea))
        if min(rw, rh) >= 15 and (best is None or max(rw, rh) / min(rw, rh) > best):
            best = max(rw, rh) / min(rw, rh)
    return best


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", nargs="?", type=Path, default=Path("bullets_grauwolf.csv"))
    ap.add_argument("--images", type=Path, default=Path("grauwolf_images"))
    ap.add_argument("--cache", type=Path, help="the cache of grauwolf_fetch.py (the listing page)")
    args = ap.parse_args()
    args.images.mkdir(parents=True, exist_ok=True)
    listing = gf.get(gf.PAGE, args.cache)
    rel = {}
    for m in re.finditer(r'<img src="https://[^"]+?/small_image/[^"]*?/[0-9a-f]{32}/([^"]+)" width="196" height="147" alt="([^"]+)"', listing):
        p = gf.parse(html.unescape(m[2]))
        if p:
            rel.setdefault((p["manufacturer"], p["line"], round(float(p["diameter_in"]), 3), float(p["weight_gr"])), m[1])
    with open(args.csv, newline="", encoding="utf-8") as fh:
        rows = list(csv.DictReader(fh))
    todo = [r for r in rows if r["valid"] == "yes"]
    for r in todo:
        r["_est"] = length_in(float(r["weight_gr"]), float(r["diameter_in"]), r["ogive_style"])[0]
        r["_cls"] = classify(r["ogive_style"])
        key = (r["manufacturer"], r["line"], round(float(r["diameter_in"]), 3), float(r["weight_gr"]))
        f = photo(rel[key], args.images) if key in rel else None
        q = ratio(f) if f else None
        r["_photo"] = q * float(r["diameter_in"]) / BIAS if q else None  # inch
    by = {}
    for r in todo:
        if r["_photo"] and 0.65 < r["_photo"] / r["_est"] < 1.6:
            by.setdefault((r["manufacturer"], r["_cls"]), []).append(r["_photo"] / r["_est"])
    factor = {k: statistics.median(v) for k, v in by.items() if len(v) >= 5}
    maker = {m: statistics.median([x for (mk, _), v in by.items() if mk == m for x in v]) for m in {k[0] for k in by}}
    used = scaled = 0
    for r in todo:
        f = factor.get((r["manufacturer"], r["_cls"])) or maker.get(r["manufacturer"], 1.0)
        corrected = r["_est"] * f
        if r["_photo"] and abs(r["_photo"] / corrected - 1) <= TOLERANCE:
            r["length_in"] = f"{r['_photo']:.4f}"
            r["length_note"] = "Length measured on the product photo (length / width x diameter, about 5 % uncertain)"
            used += 1
        else:
            r["length_in"] = f"{corrected:.4f}"
            r["length_note"] = (f"Length estimated from mass, diameter and bullet type and scaled by {f:.2f}, the median "
                                f"photo / estimate ratio of the maker's bullets, about 10 % uncertain")
            scaled += 1
    for r in rows:
        for k in ("_est", "_cls", "_photo"):
            r.pop(k, None)
    cols = [*COLUMNS, "length_note"] if "length_note" not in COLUMNS else COLUMNS
    with open(args.csv, "w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, cols, extrasaction="ignore")
        w.writeheader()
        w.writerows(rows)
    print(f"{used} lengths from photos, {scaled} scaled estimates; factors per maker:", {k: round(v, 2) for k, v in sorted(maker.items())}, file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
