#!/usr/bin/env python3
"""Turns the shotshell tables of the Hodgdon Reloading Manual (PDF) into fit samples (CSV) for the shotshell model.

    hodgdon_shot_extract.py manual.pdf [-o hodgdon_shot_samples.csv]       (needs pdftotext)

Each table block is one hull (12 gauge, 2 3/4") with one shot weight; below the heading the columns are velocity levels (fps). Every
row is powder, primer, wad and one cell per velocity level: "charge grains / pressure PSI" ("-": no such load). Pressures in LUP (older,
not comparable to PSI) are left out, the load's charge and velocity stay. One CSV row per load.
"""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

GRAIN_G = 0.06479891
OZ_G = 28.349523
FPS = 0.3048
PSI_BAR = 0.0689476
POWDERS = {"Clays": "Hodgdon Clays", "Titewad": "Hodgdon Titewad", "Internat’l": "Hodgdon International", "Internat'l": "Hodgdon International",
           "700-X": "Hodgdon 700-X", "Universal": "Hodgdon Universal", "Longshot": "Hodgdon Longshot", "Titegroup": "Hodgdon Titegroup",
           "HS-6": "Hodgdon HS-6", "H110": "Hodgdon H110", "Lil’Gun": "Hodgdon Lil'Gun", "Lil'Gun": "Hodgdon Lil'Gun",
           "Super Hcp": "Winchester Super Handicap", "WST": "Winchester WST", "WSF": "Winchester WSF", "572": "Winchester 572",
           "231": "Winchester 231", "Competition": "Ramshot Competition", "Zip": "Ramshot Zip", "True Blue": "Ramshot True Blue"}
HEAD = re.compile(r"^\s*(\d+)\s*G(?:A)?,?\s.*?(\d)\s+(\d)/(\d)\W.*?-\s*([\d ]+(?:/\d)?)\s*oz", re.I)
LEVELS = re.compile(r"^\s*(\d{3,4})\s*fps(?:\s+(\d{3,4})\s*fps)*")
CELL = re.compile(r"(\d+\.?\d*)\s*/\s*([\d,]+)\s*(PSI|LUP)")


def shot_oz(text: str) -> float:
    parts = text.split()
    total = 0.0
    for p in parts:
        if "/" in p:
            a, b = p.split("/")
            total += float(a) / float(b)
        else:
            total += float(p)
    return total


BRANDS = re.compile(r"federal|winchester|remington|fiocchi|rio|cheddite|estate|baschieri|activ|nobel", re.I)


def shell_name(heading: str) -> str:
    """The shell (hull) named in a table heading: "12G FeDeral GolD MeDal 2 3/4” PlaStiC ShellS - 1 1/8oz leaD Shot" -> "Federal Gold Medal"
    (the manual's small caps come out as mixed case)."""
    s = re.sub(r"^\s*\d+\s*G(?:A)?[,.]?\s*", "", heading)
    s = re.sub(r"\d\s+\d/\d\W*", " ", s)  # the hull length
    s = re.sub(r"[-–—]\s*[\d ./]+\s*oz.*$", "", s, flags=re.I)  # the shot weight and what follows
    s = re.sub(r"plastic\s*shells?", "", s, flags=re.I)
    s = re.sub(r"\s+", " ", s).strip(" ,-").title()
    return s if BRANDS.search(s) else ""  # a heading without a shell ("... Heavy Loads Continued"): the previous one still applies


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pdf", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("hodgdon_shot_samples.csv"))
    ap.add_argument("--first-page", type=int, default=173)
    ap.add_argument("--last-page", type=int, default=194)
    args = ap.parse_args()
    text = ""
    for page in range(args.first_page, args.last_page + 1):
        text += subprocess.run(["pdftotext", "-f", str(page), "-l", str(page), "-layout", str(args.pdf), "-"], capture_output=True, text=True).stdout
    rows, unknown = [], {}
    gauge = hull_in = shot = None
    shell = ""
    levels: list[float] = []
    for line in text.splitlines():
        m = HEAD.match(line)
        if m and "shot" in line.lower():
            gauge = int(m[1])
            hull_in = int(m[2]) + int(m[3]) / int(m[4])
            shot = shot_oz(m[5].strip()) * OZ_G
            shell = shell_name(line) or shell
            levels = []
            continue
        m = re.match(r"^\s*((?:\d{3,4}\s*fps\s*)+)$", line)
        if m:
            levels = [float(v) * FPS for v in re.findall(r"(\d{3,4})\s*fps", line)]
            continue
        if gauge is None or not levels:
            continue
        cells = list(CELL.finditer(line))
        if not cells:
            continue
        left = re.split(r"\s{2,}", line[: cells[0].start()].strip())
        # the cells sit in the columns of the levels: a gap (an en dash) shifts nothing, so find the column of each cell by order from the right
        tail = line[left and len(line[: cells[0].start()].rstrip()) :]
        parts = re.split(r"\s{2,}", line.strip())
        if len(parts) < 4:
            continue
        powder, primer, wad = parts[0], parts[1], parts[2]
        name = POWDERS.get(powder)
        if not name:
            unknown[powder] = unknown.get(powder, 0) + 1
            continue
        cell_texts = parts[3:]
        if len(cell_texts) != len(levels):  # a column of dashes can be missing from the text: align from the right
            cell_texts = ["–"] * (len(levels) - len(cell_texts)) + cell_texts
        for velocity, cell in zip(levels, cell_texts):
            c = CELL.search(cell)
            if not c:
                continue
            charge = float(c[1]) * GRAIN_G
            pressure = float(c[2].replace(",", "")) * PSI_BAR if c[3] == "PSI" else ""
            rows.append({"powder": name, "gauge": gauge, "hull_mm": round(hull_in * 25.4), "shot_g": round(shot, 2), "wad": wad, "primer": primer, "shell": shell,
                         "charge_g": round(charge, 4), "velocity_ms": round(velocity, 1), "pressure_bar": f"{pressure:.0f}" if pressure != "" else ""})
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} loads -> {args.output}", file=sys.stderr)
    print("powders not mapped:", unknown, file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
