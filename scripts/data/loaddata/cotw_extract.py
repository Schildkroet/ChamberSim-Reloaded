#!/usr/bin/env python3
"""Reads the loading data tables of "Cartridges of the World" (15th edition, PDF) into a CSV.

    cotw_extract.py "Cartridges of the World 15th Edition.pdf" [-o cotw_loads.csv]

The chapters show their tables as pictures ("View a text version of this table"); the text versions follow at the end of the book, in the same order,
each ending with "Return to main text". The k-th marker of the chapters therefore belongs to the k-th text table. The marker's heading
("7mm TCU (7mmx223) Loading Data", "... (28-inch barrel)") names the cartridge and often the barrel. A loading table has the columns bullet (grains and
type), powder, charge (grains), velocity (fps), energy and source; factory loads ("FL") are skipped. No pressures are given.
Output: one row per load, with the heading, cartridge, barrel (inch, if the heading gives it), bullet weight and type, powder, charge, velocity, source.
"""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

MARK = "View a text version of this table"
END = "Return to main text"
HEAD = re.compile(r"(?P<cart>\S.*?)\s+Loading Data(?P<rest>.*)$", re.I)
BARREL = re.compile(r"(\d+(?:\.\d+)?)[- ]?(?:inch|in\.|\")\s*(?:barrel|bbl)", re.I)
ROW = re.compile(r"^\s*(?P<bullet>\d{2,3}(?:\.\d)?\*?\s*[^\d\s].*?|\d{2,3}\*)\s{2,}(?P<powder>\S.*?)\s{2,}(?P<grains>\d+(?:\.\d+)?)\s+(?P<fps>\d[\d,]{2,5})\s+"
                 r"(?P<energy>\d[\d,]*)\s*(?P<source>\S.*)?$")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("pdf", type=Path)
    ap.add_argument("-o", "--out", type=Path, default=Path("cotw_loads.csv"))
    args = ap.parse_args()
    lines = subprocess.run(["pdftotext", "-layout", str(args.pdf), "-"], capture_output=True, text=True, check=True).stdout.splitlines()
    marks = [i for i, l in enumerate(lines) if MARK in l]
    start = marks[-1]
    # the text tables begin after the last marker's chapter ends: the first table's end marks the first "Return to main text" after the appendix' start
    ends = [i for i, l in enumerate(lines) if END in l and i > start]
    tables, prev = [], None
    first_start = None
    for k, e in enumerate(ends):
        lo = ends[k - 1] + 1 if k else None
        if lo is None:  # the first table starts at its own header line
            lo = max(i for i in range(e) if re.search(r"Bullet \(grains/type\)|MUZZLE VELOCITY|Cartridge\s+Bullet", lines[i]) and i > start) if False else None
        tables.append((lo, e))
    # first table: scan back from the first end to the previous blank block
    f = ends[0]
    lo = f
    while lo > start and f - lo < 40 and (lines[lo - 1].strip() or lines[lo - 2].strip()):
        lo -= 1
    tables[0] = (lo, f)
    print(f"{len(marks)} markers, {len(tables)} text tables", file=sys.stderr)
    heads = []
    for m in marks:
        title = ""
        for j in range(m - 1, max(m - 14, 0), -1):
            t = lines[j].strip()
            if t and not t.startswith(MARK) and re.search(r"Loading Data|Factory Ballistics|Ballistics|Performance|Comparison", t, re.I):
                title = re.sub(r"\s+", " ", t)
                break
        heads.append(title)
    rows, kinds = [], {"loads": 0, "other": 0}
    for (lo, hi), head in zip(tables, heads):
        body = lines[lo:hi]
        if not any("Bullet (grains/type)" in l for l in body):
            kinds["other"] += 1
            continue
        kinds["loads"] += 1
        h = HEAD.match(head)
        cart = h["cart"] if h else head
        barrel = BARREL.search(head) or BARREL.search(" ".join(body))
        for l in body:
            m = ROW.match(l)
            if not m or m["powder"].strip() in ("FL", "Factory Load"):
                continue
            w = re.match(r"(\d+(?:\.\d)?)\*?\s*(.*)$", m["bullet"].strip())
            rows.append({"heading": head, "cartridge": re.sub(r"\s*\(.*", "", cart).strip(), "barrel_in": barrel[1] if barrel else "",
                         "bullet_gr": w[1], "bullet": w[2].strip(), "powder": re.sub(r"\s+", " ", m["powder"]).strip(), "charge_gr": m["grains"],
                         "velocity_fps": m["fps"].replace(",", ""), "energy": m["energy"].replace(",", ""), "source": (m["source"] or "").strip()})
    with args.out.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0]))
        w.writeheader()
        w.writerows(rows)
    print(f"{kinds} -> {len(rows)} loads, {len({r['cartridge'] for r in rows})} cartridges, {sum(1 for r in rows if r['barrel_in'])} with a barrel length -> {args.out}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
