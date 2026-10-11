#!/usr/bin/env python3
"""Measures bullets on side-view product photos (hunting_sport_fetch.py): length, and the profile (nose, meplat, bearing, base).

    measure_side_photos.py <photo dir> [--manifest manifest.csv] [--out measured.csv] [--overlay DIR]

The photos are studio shots of one bullet on a white or transparent background. The silhouette (alpha channel, else everything that is not
white, holes filled) gives, in the bullet's own axis (long side of the minimum-area rectangle), the length and the width profile. The widest
section is the bearing diameter (known from the manifest), which sets the scale: length = L / W x diameter. The profile gives, in diameters:
meplat (width at the nose end, 3 % of the length from the tip), nose (from the tip to where the width reaches 98 % of the widest), bearing
(the length at 98 % or more of the widest) and base (width at the base end over the widest: a boat tail is below 1). `ratio` is the length
over the diameter: below 1.15 the long side of the silhouette may not be the bullet's axis (wadcutters), and `flag` says so.
Only files that exist are measured (deleted photos are skipped); subdirectories are included (good/)."""
import argparse
import csv
import math
from pathlib import Path

import cv2
import numpy as np

GRAIN_PER_G = 15.4324


def silhouette(img: np.ndarray) -> np.ndarray:
    bgr = img[..., :3] if img.ndim == 3 else cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
    hsv = cv2.cvtColor(bgr, cv2.COLOR_BGR2HSV)
    mask = (bgr.min(axis=2) < 238) | (hsv[..., 1] > 28)  # not white: the bullet (white and glossy parts are filled in below)
    if img.ndim == 3 and img.shape[2] == 4 and img[..., 3].min() < 250:
        mask &= img[..., 3] > 200  # the soft shadow under the bullet is translucent
    mask = mask.astype(np.uint8) * 255
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, np.ones((5, 5), np.uint8))
    n, labels, stats, _ = cv2.connectedComponentsWithStats(mask)
    if n < 2:
        return mask
    keep = 1 + int(np.argmax(stats[1:, cv2.CC_STAT_AREA]))
    mask = (labels == keep).astype(np.uint8) * 255
    padded = cv2.copyMakeBorder(mask, 2, 2, 2, 2, cv2.BORDER_CONSTANT, value=0)  # a bullet touching the border must not split the background
    inv = cv2.bitwise_not(padded)  # fill the holes (bright reflections inside the bullet)
    cv2.floodFill(inv, np.zeros((inv.shape[0] + 2, inv.shape[1] + 2), np.uint8), (0, 0), 0)
    return (padded | inv)[2:-2, 2:-2]


def measure(path: Path, diameter_in: float, overlay: Path | None):
    img = cv2.imread(str(path), cv2.IMREAD_UNCHANGED)
    if img is None:
        return None
    mask = silhouette(img)
    ys, xs = np.nonzero(mask)
    if len(xs) < 500:
        return None
    pts = np.column_stack([xs, ys]).astype(np.float64)
    # the bullet's axis: the long side of the minimum-area rectangle (the principal axis of a stubby, tapered shape is not reliable)
    (cx, cy), (rw, rh), angle = cv2.minAreaRect(pts.astype(np.float32))
    a = math.radians(angle if rw >= rh else angle + 90.0)
    axis = np.array([math.cos(a), math.sin(a)])
    c = np.array([cx, cy])
    t = (pts - c) @ axis
    v = (pts - c) @ np.array([-axis[1], axis[0]])
    L = t.max() - t.min()
    # width profile along the axis in 100 slices
    edges = np.linspace(t.min(), t.max(), 101)
    idx = np.clip(np.digitize(t, edges) - 1, 0, 99)
    width = np.array([(v[idx == k].max() - v[idx == k].min()) if np.any(idx == k) else 0.0 for k in range(100)])
    W = np.percentile(width[width > 0], 98)
    r = L / W
    prof = width / W
    # the tip is the narrower end (end slices); orient so that the nose is at the high end
    if prof[:5].mean() < prof[-5:].mean():
        prof = prof[::-1]
    meplat = float(prof[-3:].mean())
    base = float(prof[:3].mean())
    full = np.nonzero(prof >= 0.98)[0]
    bearing = (full.max() - full.min() + 1) / 100.0 * r if len(full) else 0.0
    nose = (99 - full.max()) / 100.0 * r if len(full) else 0.0
    length_in = r * diameter_in
    if overlay is not None:
        vis = cv2.cvtColor(mask, cv2.COLOR_GRAY2BGR) // 2 + (img[..., :3] // 2 if img.ndim == 3 else 0)
        p1, p2 = c + axis * t.min(), c + axis * t.max()
        cv2.line(vis, tuple(int(x) for x in p1), tuple(int(x) for x in p2), (0, 0, 255), 3)
        cv2.imwrite(str(overlay / (path.stem + ".png")), cv2.resize(vis, None, fx=0.5, fy=0.5))
    return {"L_px": round(L, 1), "W_px": round(W, 1), "ratio": round(r, 3), "length_in": round(length_in, 4), "length_mm": round(length_in * 25.4, 2),
            "meplat_d": round(meplat, 3), "nose_d": round(nose, 3), "bearing_d": round(bearing, 3), "base_d": round(base, 3)}


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("--manifest")
    ap.add_argument("--out")
    ap.add_argument("--overlay")
    args = ap.parse_args()
    root = Path(args.dir)
    manifest = {r["file"]: r for r in csv.DictReader(open(args.manifest or root / "manifest.csv", encoding="utf-8"))}
    overlay = Path(args.overlay) if args.overlay else None
    if overlay:
        overlay.mkdir(parents=True, exist_ok=True)
    rows = []
    for path in sorted(root.rglob("*")):
        if path.suffix.lower() not in (".jpg", ".png") or path.name not in manifest:
            continue
        m = manifest[path.name]
        d, w = float(m["diameter_in"]), float(m["weight_gr"])
        res = measure(path, d, overlay)
        if res is None:
            print("not measurable:", path.name)
            continue
        area_cm2 = math.pi * (d * 2.54) ** 2 / 4
        rho = w / GRAIN_PER_G / (area_cm2 * res["length_in"] * 2.54)
        flag = []
        if res["ratio"] < 1.15:
            flag.append("axis?")
        if not 6.0 <= rho <= 12.5:
            flag.append("density")
        rows.append({"set": path.parent.name, "file": path.name, "brand": m["brand"], "title": m["title"], "diameter_in": d, "weight_gr": w, **res,
                     "eff_density": round(rho, 2), "flag": " ".join(flag)})
    out = Path(args.out or root / "measured.csv")
    with open(out, "w", newline="", encoding="utf-8") as f:
        wr = csv.DictWriter(f, list(rows[0].keys()))
        wr.writeheader()
        wr.writerows(rows)
    print(len(rows), "measured ->", out)


if __name__ == "__main__":
    main()
