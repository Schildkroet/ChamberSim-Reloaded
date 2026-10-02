#!/usr/bin/env python3
"""Adds an estimated case capacity V (grain of water) to the CSV made by cip_extract.py.

    cip_estimate_v.py cip_calibers.csv [-o cip_calibers.csv]

The C.I.P. sheets give the outer shape of the maximum cartridge but not its capacity. The estimate revolves the
inside of the case: outer radius (base P1 -> P2 at L1, shoulder to the neck H1/H2 at L2, neck to the mouth L3, or
P1 -> H2 straight over L3 for straight cases) minus a wall, starting behind a solid head. Wall and head are two
assumed constants (below). Cases without the needed dimensions get no value. Checked against known capacities the error is about 6 % (rms) for
centerfire cases and 10 % for rimfire (other constants).
"""
import argparse
import csv
import math
import sys
from pathlib import Path

WALL = 0.45        # mm, case wall (body and neck)
HEAD = 0.50        # solid head: this fraction of the base diameter P1 (primer pocket and web)
RIMFIRE = (0.25, 0.10)  # wall, head: rimfire cases are thin and have no solid head
NECK_OVER_BULLET = 0.87  # mm, neck outside diameter above the bullet diameter G1 when the sheet gives no H1/H2
GRAIN_PER_CM3 = 15.4324
NOTE = "V estimated from the sheet geometry (wall %.2f mm, head %.2f x P1)"


def num(row: dict, key: str) -> float:
    try:
        return float(row.get(key) or 0)
    except ValueError:
        return 0.0


def capacity(row: dict, wall: float = WALL, head: float = HEAD) -> float | None:
    """Capacity in grain H2O, or None if the sheet lacks the dimensions."""
    p1, l3 = num(row, "P1"), num(row, "L3")
    if p1 <= 0 or l3 <= 0:
        return None
    l1, l2, p2, h1, h2 = (num(row, k) for k in ("L1", "L2", "P2", "H1", "H2"))
    if not h1 and not h2 and num(row, "G1") > 0:
        h1 = h2 = num(row, "G1") + NECK_OVER_BULLET
    if l1 > 0 and l2 > l1 and p2 > 0 and h1 > 0:  # bottleneck / tapered with a shoulder
        pts = [(0.0, p1), (l1, p2), (l2, h1), (l3, h2 or h1)]
    elif h2 > 0:  # straight or slightly tapered case
        pts = [(0.0, p1), (l3, h2)]
    else:
        return None
    x0 = head * p1
    # piecewise linear outer radius, integrated numerically
    steps = 2000
    vol = 0.0
    dx = (l3 - x0) / steps
    for i in range(steps):
        x = x0 + (i + 0.5) * dx
        for (xa, da), (xb, db) in zip(pts, pts[1:]):
            if xa <= x <= xb:
                d = da + (db - da) * (x - xa) / (xb - xa)
                break
        else:
            continue
        r = d / 2 - wall
        if r > 0:
            vol += math.pi * r * r * dx
    return vol / 1000 * GRAIN_PER_CM3  # mm3 -> cm3 -> grain H2O


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", type=Path)
    ap.add_argument("-o", "--output", type=Path)
    args = ap.parse_args()
    with open(args.csv, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        cols = list(reader.fieldnames)
        rows = list(reader)
    for c in ("V", "descr"):
        if c not in cols:
            cols.insert(cols.index("source"), c)
    n = 0
    for r in rows:
        v = capacity(r, *RIMFIRE) if r.get("table") == "V" else capacity(r)
        r["V"] = f"{v:.1f}" if v else ""
        r["descr"] = NOTE % (RIMFIRE if r.get("table") == "V" else (WALL, HEAD)) if v else ""
        n += bool(v)
    with open(args.output or args.csv, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, cols)
        w.writeheader()
        w.writerows(rows)
    print(f"V estimated for {n} of {len(rows)} cartridges", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
