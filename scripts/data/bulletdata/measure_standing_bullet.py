#!/usr/bin/env python3
"""Length of the standing bullet in the two-bullet photos (one standing, one lying, seen from slightly above) of H&N and LOS.

    measure_standing_bullet.py <photo dir> [--manifest manifest.csv] [--out standing.csv] [--overlay DIR] [files ...]

The camera looks down at the elevation phi, so the standing bullet's silhouette is its profile compressed by cos(phi) plus the far rim of the
nose and the near rim of the base:   H' = L cos(phi) + (D / 2) (1 + m) sin(phi)   (L length, D diameter, m = nose end width / D).
  * the bullet's scale is its bearing diameter: the most common width of the rows of its silhouette (the straight sides) = D (known);
  * phi comes from the curvature of the base edge: the lower boundary is half an ellipse of height (D / 2) sin(phi);
  * m is the width of the silhouette just below the top over D; H' is the silhouette's height.
The second bullet (lying, in front or behind) is excluded: only the columns around the standing bullet's centre and the rows of its own run
are used. `flag` marks photos whose bearing width was not found cleanly (the two bullets merge over the whole height)."""
import argparse
import csv
import math
from pathlib import Path

import cv2
import numpy as np

GRAIN_PER_G = 15.4324


def foreground(img: np.ndarray) -> np.ndarray:
    bgr = img[..., :3]
    if img.shape[2] == 4 and img[..., 3].min() < 250:
        mask = (img[..., 3] > 200).astype(np.uint8) * 255  # transparent background; the soft shadow is translucent
    else:
        hsv = cv2.cvtColor(bgr, cv2.COLOR_BGR2HSV)
        mask = (((hsv[..., 1] > 38) & (hsv[..., 2] > 40)) | (hsv[..., 2] < 90)).astype(np.uint8) * 255  # coloured metal and its dark reflections, not the grey shadow
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, np.ones((9, 9), np.uint8))
    padded = cv2.copyMakeBorder(mask, 2, 2, 2, 2, cv2.BORDER_CONSTANT, value=0)
    inv = cv2.bitwise_not(padded)
    cv2.floodFill(inv, np.zeros((inv.shape[0] + 2, inv.shape[1] + 2), np.uint8), (0, 0), 0)
    mask = (padded | inv)[2:-2, 2:-2]
    n, labels, stats, _ = cv2.connectedComponentsWithStats(mask)
    keep = 1 + int(np.argmax(stats[1:, cv2.CC_STAT_AREA]))
    return (labels == keep).astype(np.uint8)


def runs(row: np.ndarray):
    """(start, end) of the runs of True in a 1-d boolean array."""
    d = np.diff(np.concatenate([[0], row.astype(np.int8), [0]]))
    return list(zip(np.nonzero(d == 1)[0], np.nonzero(d == -1)[0] - 1))


def measure(path: Path, diameter_in: float, overlay: Path | None, phi_deg: float | None = None, length_in_known: float | None = None):
    img = cv2.imread(str(path), cv2.IMREAD_UNCHANGED)
    if img is None:
        return None
    mask = foreground(img).astype(bool)
    ys, xs = np.nonzero(mask)
    height_of_col = np.zeros(mask.shape[1])
    for x in np.unique(xs):
        col = np.nonzero(mask[:, x])[0]
        height_of_col[x] = col.max() - col.min() + 1
    tall = height_of_col >= 0.88 * np.percentile(height_of_col[height_of_col > 0], 97)
    cand = runs(tall)
    if not cand:
        return None
    x0, x1 = max(cand, key=lambda r: r[1] - r[0])  # the columns where the standing bullet is as tall as it gets
    xc = int((x0 + x1) / 2)
    # the standing bullet's own run in the centre column
    col = np.nonzero(mask[:, xc])[0]
    y_top = int(col.min())
    # rows downward: the run containing xc; stop when the run is much wider than the bearing (the lying bullet merges)
    widths = {}
    for y in range(y_top, mask.shape[0]):
        if not mask[y, xc]:
            break
        r = next(r for r in runs(mask[y]) if r[0] <= xc <= r[1])
        widths[y] = r[1] - r[0] + 1
    ys_run = np.array(sorted(widths))
    w = np.array([widths[y] for y in ys_run], dtype=float)
    # bearing width: the most common width (the straight sides), 3 px bins; ignore the widest 15 % (merged with the lying bullet)
    wmax = np.percentile(w, 85)
    hist, edges = np.histogram(w[w <= wmax], bins=np.arange(w.min(), wmax + 4, 3))
    D_px = float(edges[int(np.argmax(hist))] + 1.5)
    # refine with the rows of that bin
    D_px = float(np.median(w[(w >= D_px - 3) & (w <= D_px + 3)]))
    H = float(ys_run.max() - y_top + 1)
    # base edge: lowest mask row of the columns around the centre, and at 0.45 D from the centre to both sides
    def bottom(x):
        c = np.nonzero(mask[y_top:, int(x)])[0]
        # contiguous from the top
        if len(c) == 0:
            return None
        gaps = np.nonzero(np.diff(c) > 1)[0]
        return y_top + (c[gaps[0]] if len(gaps) else c[-1])
    yb_c = bottom(xc)
    sag = []
    for side in (-1, 1):
        yb = bottom(xc + side * 0.45 * D_px)
        if yb is not None and yb_c is not None:
            sag.append(yb_c - yb)
    sin_phi = float(np.clip(max(sag) / (0.282 * D_px), 0.05, 0.6)) if sag else 0.25
    phi = math.asin(sin_phi) if phi_deg is None else math.radians(phi_deg)
    sin_phi = math.sin(phi)
    H = float(yb_c - y_top + 1)
    m = float(np.clip(np.median(w[: max(3, int(0.04 * H))]) / D_px, 0.0, 1.0)) if len(w) else 0.5
    # the width just below the very top is the nose end; the first rows are the antialiased edge
    near = w[int(0.02 * H): int(0.06 * H) + 2]
    m = float(np.clip(np.median(near) / D_px, 0.0, 1.0)) if len(near) else m
    s = D_px / diameter_in
    if length_in_known is not None:  # calibration: the elevation that makes H = L cos(phi) + (D / 2) (1 + m) sin(phi) for the known length
        A, B = length_in_known * s, 0.5 * D_px * (1 + m)
        R, delta = math.hypot(A, B), math.atan2(B, A)
        phi = delta + math.acos(min(H / R, 1.0))  # H = R cos(phi - delta), phi - delta > 0 (the camera looks down)
        sin_phi = math.sin(phi)
    L_px = (H - 0.5 * D_px * (1 + m) * sin_phi) / math.cos(phi)
    length_in = L_px / s
    flag = []
    if len(w) < 40 or (w <= wmax).sum() < 0.4 * len(w):
        flag.append("merged")
    if not sag:
        flag.append("no-base")
    if overlay is not None:
        vis = img[..., :3].copy()
        vis[mask] = (0.6 * vis[mask] + 0.4 * np.array([0, 255, 0])).astype(np.uint8)
        cv2.line(vis, (xc, y_top), (xc, int(yb_c)), (0, 0, 255), 3)
        cv2.rectangle(vis, (int(xc - D_px / 2), y_top), (int(xc + D_px / 2), int(yb_c)), (255, 0, 0), 2)
        cv2.imwrite(str(overlay / (path.stem + ".png")), cv2.resize(vis, None, fx=0.5, fy=0.5))
    return {"D_px": round(D_px, 1), "H_px": round(H, 1), "phi_deg": round(math.degrees(phi), 1), "phi_base_arc_deg": round(math.degrees(math.asin(float(np.clip(max(sag) / (0.282 * D_px), 0.05, 0.6)))) if sag else float("nan"), 1), "meplat_d": round(m, 2),
            "length_in": round(length_in, 4), "length_mm": round(length_in * 25.4, 2), "flag": " ".join(flag)}


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("files", nargs="*")
    ap.add_argument("--manifest")
    ap.add_argument("--out")
    ap.add_argument("--overlay")
    ap.add_argument("--phi", type=float, help="camera elevation in degrees (the same for all photos of one renderer) instead of the base edge of each")
    ap.add_argument("--kind", help="only photos of this manifest kind (einzeln, doppelt)")
    ap.add_argument("--calibrate", help="FILE:LENGTH_MM, a photo of a bullet of known length: prints the camera elevation and exits")
    args = ap.parse_args()
    root = Path(args.dir)
    manifest = {r["file"]: r for r in csv.DictReader(open(args.manifest or root / "manifest.csv", encoding="utf-8"))}
    overlay = Path(args.overlay) if args.overlay else None
    if overlay:
        overlay.mkdir(parents=True, exist_ok=True)
    if args.calibrate:
        f, mm = args.calibrate.rsplit(":", 1)
        m = manifest[f]
        res = measure(root / f, float(m["diameter_in"]), overlay, length_in_known=float(mm) / 25.4)
        print(f"{f}: elevation {res['phi_deg']} deg for a length of {mm} mm (the base edge of this photo says {res['phi_base_arc_deg']} deg); D {res['D_px']} px, height {res['H_px']} px, nose end {res['meplat_d']} D")
        return
    paths = [root / f for f in args.files] if args.files else sorted(p for p in root.glob("*") if p.name in manifest and (not args.kind or manifest[p.name].get("kind") == args.kind))
    rows = []
    for path in paths:
        m = manifest[path.name]
        d, wgr = float(m["diameter_in"]), float(m["weight_gr"])
        try:
            res = measure(path, d, overlay, args.phi)
        except Exception as e:  # a photo that does not fit the pattern
            print("failed:", path.name, e)
            continue
        if res is None:
            print("not measurable:", path.name)
            continue
        rho = wgr / GRAIN_PER_G / (math.pi * (d * 2.54) ** 2 / 4 * res["length_in"] * 2.54)
        rows.append({"file": path.name, "brand": m["brand"], "title": m["title"], "diameter_in": d, "weight_gr": wgr, **res, "eff_density": round(rho, 2)})
    out = Path(args.out or root / "standing.csv")
    with open(out, "w", newline="", encoding="utf-8") as f:
        wr = csv.DictWriter(f, list(rows[0].keys()))
        wr.writeheader()
        wr.writerows(rows)
    print(len(rows), "measured ->", out)


if __name__ == "__main__":
    main()
