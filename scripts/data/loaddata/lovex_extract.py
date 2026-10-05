#!/usr/bin/env python3
"""Turns the rifle tables of Explosia's LOVEX Reloading Guide (PDF) into fit samples (CSV), like vv_extract.py.

    lovex_extract.py guide.pdf [-o lovex_samples.csv]       (needs pdftotext)

The guide lists per cartridge (test barrel, case, maximum C.I.P. pressure) and bullet (maker, type, mass, cartridge length) the
recommended and the maximum charge of each powder with the velocity (V5) and, for the maximum load, the measured pressure.
Every powder row gives a "start" and a "max" sample; the max sample carries the pressure (column pressure_bar), which the fit can use
to determine more than the burn rate. Cartridges are matched to data/calibers, bullets to data/projectiles (else a length is
estimated from mass, diameter and type, see scripts/data/bulletdata/estimate_length.py).
"""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

from samples_common import Records  # noqa: E402

MAKERS = ["Ares", "Barnes", "Hornady", "Lapua", "Norma", "Nosler", "Sellier & Bellot", "Sierra", "Winchester"]
POWDER = re.compile(r"^\s*([SD]\d{3}(?:\.\d)?)\s+((?:[\d.]+\s+){9}[\d.]+)(?:\s+([\d.]+)\s*mm)?\s*(?:compressed)?\s*$")
COL_ONLY = re.compile(r"\s([\d.]+)\s*mm\s*$")


def parse(text: str):
    """Yields dicts: cartridge, barrel, maxcol, maker, btype, mass_g, grains, col, powder, numbers."""
    lines = text.splitlines()
    start = next(i for i, l in enumerate(lines) if l.strip() == "RIFLE DATA")
    cart = barrel = maxcol = None
    blk = None
    for i in range(start, len(lines)):
        line = lines[i]
        m = re.search(r"Barrel Length:\s*([\d.]+)\s*mm.*Max\. Cartridge Length:\s*([\d.]+)\s*mm", line)
        if m:
            j = i - 1
            while not lines[j].strip():
                j -= 1
            cart = re.sub(r"\s*\(continued\)", "", lines[j]).strip()
            barrel, maxcol = float(m[1]), float(m[2])
            blk = None
            continue
        if re.match(r"\s*Bullet\s+Recommended Loads", line):
            maker = next((mk for mk in MAKERS if mk.lower() in line.lower()), "")
            blk = {"maker": maker, "btype": "", "mass_g": 0.0, "grains": 0.0, "rows": [], "col": None}
            continue
        if blk is None:
            continue
        m = re.match(r"\s*(.+?)\s{2,}Powder Charge", line)
        if m:
            blk["btype"] = m[1].strip()
            if not blk["maker"]:
                blk["maker"] = next((mk for mk in MAKERS if mk.lower() in line.lower()), "")
            continue
        m = re.match(r"\s*([\d.]+)\s*g\s*$", line)
        if m:
            blk["mass_g"] = float(m[1])
            continue
        m = re.match(r"\s*(\d+)\s*grs\b", line)
        if m:
            blk["grains"] = float(m[1])
            continue
        m = POWDER.match(line)
        if m and cart:
            nums = [float(x) for x in m[2].split()]
            col = m[3] or (COL_ONLY.search(line) or [None, None])[1]
            blk["rows"].append((m[1], nums, float(col) if col else None))
            # a block is complete when the next header comes; yield lazily
            yield {"cartridge": cart, "barrel": barrel, "maxcol": maxcol, "blk": blk, "powder": m[1], "nums": nums, "col": float(col) if col else None}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pdf", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("lovex_samples.csv"))
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    text = subprocess.run(["pdftotext", "-layout", str(args.pdf), "-"], capture_output=True, text=True, check=True).stdout

    records = Records(args.data)

    # the cartridge length of a bullet block is printed in one row only: collect the rows per block first
    blocks = {}
    for r in parse(text):
        blocks.setdefault(id(r["blk"]), {"r": r, "rows": []})["rows"].append(r)
    rows, miss = [], {}
    for b in blocks.values():
        first = b["r"]
        blk = first["blk"]
        col = next((r["col"] for r in b["rows"] if r["col"]), None) or first["maxcol"]
        cal = records.caliber(first["cartridge"])
        if not cal:
            miss[first["cartridge"]] = miss.get(first["cartridge"], 0) + 1
            continue
        dia = cal.get("G1") or 0
        if not dia or not blk["grains"]:
            continue
        length, source, tail = records.bullet(blk["maker"], blk["btype"], blk["grains"], dia)
        for r in b["rows"]:
            n = r["nums"]  # rec g, rec gr, m/s, fps, max g, gr, m/s, fps, bar, psi
            for kind, g, v, p in (("start", n[0], n[2], ""), ("max", n[4], n[6], n[8])):
                rows.append({"powder": r["powder"], "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""),
                             "bullet": f"{blk['maker']} {blk['btype']}".strip(), "bullet_gr": blk["grains"], "bullet_dia_mm": dia,
                             "bullet_len_mm": f"{length:.2f}", "len_source": source, "tail_type": tail, "col_mm": col,
                             "barrel_mm": first["barrel"], "twist_mm": "", "kind": kind, "charge_g": g, "velocity_ms": v, "pressure_bar": p})
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} samples -> {args.output}", file=sys.stderr)
    print("cartridges not found:", ", ".join(f"{k} ({v})" for k, v in sorted(miss.items())), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
