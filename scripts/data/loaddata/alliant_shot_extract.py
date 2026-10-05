#!/usr/bin/env python3
"""Turns the shotshell tables of Alliant Powder's Reloader's Guide (PDF) into fit samples (CSV) for the shotshell model.

    alliant_shot_extract.py guide.pdf [-o alliant_shot_samples.csv]       (needs pdftotext)

Two table layouts are read (lead shot only; steel, bismuth and other non-toxic tables are skipped):
- "Shot (oz) | Powder | Primer | Wad | velocity levels (fps) with charge and pressure": one cell "19.0 grs 7800 psi" per velocity level; the cell
  belongs to the velocity column whose heading starts nearest to it.
- the international target loads: "Shot (gram) | Velocity (fps) | Wad | Primer" and a pair (grains, psi) for each powder of the heading.
Pressures in psi are used, CUP is left out (not comparable). One CSV row per load, columns as hodgdon_shot_extract.py.
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
POWDERS = [(r"Red Dot", "Red Dot"), (r"Green Dot", "Green Dot"), (r"Blue Dot", "Blue Dot"), (r"Amer(?:ican|\.)? Select", "American Select"),
           (r"Clay Dot", "Clay Dot"), (r"e3", "e3"), (r"Promo", "Promo"), (r"Unique", "Unique"), (r"Herco", "Herco"), (r"Bullseye", "Bullseye"),
           (r"Extra-?Lite", "Extra-Lite"), (r"20/28", "20/28"), (r"410", "410"), (r"2400", "2400"), (r"Power Pro 300-MP", "Power Pro 300-MP"),
           (r"Pro Reach", "Pro Reach"), (r"Steel", "Steel")]
HULL_LINE = re.compile(r"^\s*(\.?\d+)[- ]?(?:GAUGE|BORE)\s+(\d)(?:\s+(\d)/(\d))?\s*INCH", re.I)
TITLE_LINE = re.compile(r"^\s*(\.?\d+)[- ](?:GAUGE|BORE)", re.I)
NONTOXIC = re.compile(r"steel|bismuth|hevi|tungsten|non-toxic", re.I)
CELL_B = re.compile(r"(\d+\.?\d*)\s*grs\s+([\d.,]+)\s*(psi|cup|lup)", re.I)
SHOT_OZ = re.compile(r"^\s*(\d+\s+\d+/\d+|\d+/\d+|\d+(?:\.\d+)?)\s{2,}(.*)$")


def oz(text: str) -> float:
    total = 0.0
    for p in text.split():
        total += eval(p.replace("/", "/")) if "/" in p else float(p)  # "1 1/8" -> 1 + 1/8 (fractions only)
    return total


def pressure_bar(text: str, unit: str):
    if unit.lower() != "psi":
        return ""
    t = text.replace(",", "")
    if re.fullmatch(r"\d\.\d{3}", t):  # a thousands point ("5.800")
        t = t.replace(".", "")
    return f"{float(t) * PSI_BAR:.0f}"


BRANDS = re.compile(r"federal|winchester|remington|fiocchi|rio|cheddite|estate|baschieri|activ|nobel", re.I)


def shell_name(title: str) -> str:
    """The shells named in a table title: "12-GAUGE 2 3/4 INCH FEDERAL HIGH OVER ALL, REMINGTON PLASTIC SHELLS (STS, NITRO 27, ...)" ->
    "Federal High Over All, Remington" (the group of shells the table was fired with)."""
    s = re.sub(r"^\s*\.?\d+\s*-?\s*(?:GAUGE|GA|BORE)\s*", "", title, flags=re.I)
    s = re.sub(r"^\s*\d+(?:\s+\d/\d)?\s*(?:INCH|IN|\")\s*", "", s, flags=re.I)
    s = re.sub(r"\(.*$", "", s)
    s = re.sub(r"PLASTIC\s*SHELLS?.*$", "", s, flags=re.I)
    s = re.sub(r"\s+", " ", s).strip(" ,-").title()
    return s if BRANDS.search(s) else ""  # a heading without a shell ("... Heavy Loads Continued"): the previous one still applies


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pdf", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("alliant_shot_samples.csv"))
    args = ap.parse_args()
    text = subprocess.run(["pdftotext", "-layout", str(args.pdf), "-"], capture_output=True, text=True).stdout
    lines = text.splitlines()
    rows, unknown = [], {}
    gauge = hull_mm = None
    shell = ""
    nontoxic = False
    levels: list[tuple[int, float]] = []  # (x of the heading, velocity m/s)
    a_powders: list[str] = []
    a_cols: list[int] = []
    for i, line in enumerate(lines):
        if TITLE_LINE.match(line):
            nontoxic = bool(NONTOXIC.search(line))
            m = HULL_LINE.match(line)
            if m:
                g = m[1].lstrip(".")
                gauge = int(g)
                inch = int(m[2]) + (int(m[3]) / int(m[4]) if m[3] else 0.0)
                hull_mm = min((65, 70, 76, 89), key=lambda h: abs(h - inch * 25.4))  # 2 1/2, 2 3/4, 3 and 3 1/2 inch
                levels, a_powders, a_cols = [], [], []
                shell = shell_name(line) or shell
            continue
        if NONTOXIC.search(line) and re.search(r"POWDER FOR RELOADING|NON-TOXIC SHOTSHELL", line, re.I):
            nontoxic = True
            continue
        if gauge is None or nontoxic:
            continue
        # format B: velocity headings
        heads = list(re.finditer(r"(\d{3,4})\s*fps", line))
        if len(heads) >= 2 and "grs" not in line:
            levels = [(m.start(), int(m[1]) * FPS) for m in heads]
            continue
        # format A: heading with the powders, then the unit line with "(grains)" positions
        if re.search(r"Velocity\s+Wad\s+Primer\s+\S", line):
            names = re.split(r"\s{2,}", line.split("Primer", 1)[1].strip())
            a_powders = []
            for n in names:
                mapped = next((v for pat, v in POWDERS if re.fullmatch(pat, n.strip(), re.I)), None)
                a_powders.append(mapped or n.strip())
            a_cols = []
            for j in (i + 1, i + 2):
                cols = [m.start() for m in re.finditer(r"\(grains\)", lines[j])]
                if cols:
                    a_cols = cols
                    break
            levels = []
            continue
        cells = list(CELL_B.finditer(line))
        if cells and levels:
            m = SHOT_OZ.match(line[: cells[0].start()] + " " * 0 + "") or SHOT_OZ.match(line)
            head = line[: cells[0].start()]
            m = SHOT_OZ.match(head)
            if not m:
                continue
            shot_g = oz(m[1]) * OZ_G
            rest = m[2].strip()
            powder = next(((v, len(re.match(pat, rest, re.I)[0])) for pat, v in POWDERS if re.match(pat + r"(?=\s)", rest, re.I)), None)
            if not powder:
                unknown[rest[:20]] = unknown.get(rest[:20], 0) + 1
                continue
            parts = re.split(r"\s{2,}", rest[powder[1]:].strip()) if rest[powder[1]:].strip() else []
            primer, wad = (parts + ["", ""])[:2]
            if powder[0] == "Steel":
                continue
            for c in cells:
                col = min(levels, key=lambda lv: abs(lv[0] - c.start()))
                if gauge not in (12, 16, 20, 28, 410):
                    continue
                rows.append({"powder": "Alliant " + powder[0], "gauge": gauge, "hull_mm": hull_mm, "shot_g": round(shot_g, 2), "wad": wad, "primer": primer, "shell": shell,
                             "charge_g": round(float(c[1]) * GRAIN_G, 4), "velocity_ms": round(col[1], 1), "pressure_bar": pressure_bar(c[2], c[3])})
            continue
        if a_powders and a_cols:
            m = re.match(r"^\s*(\d{2})\s+(\d{3,4})\s{2,}(.+)$", line)
            if not m:
                continue
            body = m[3]
            nums = list(re.finditer(r"(?<![\w.])(\d+\.\d|\d{4,5})(?![\w.])", body))
            offset = line.index(body)
            parts = re.split(r"\s{2,}", body[: nums[0].start()].strip()) if nums else []
            if len(parts) < 2:
                continue
            wad, primer = parts[0], parts[1]
            by_col: dict[int, list[float]] = {}
            for n in nums:
                x = offset + n.start()
                k = max((idx for idx, cx in enumerate(a_cols) if cx - 4 <= x), default=0)
                by_col.setdefault(k, []).append(float(n[1]))
            for k, vals in by_col.items():
                if len(vals) < 1 or k >= len(a_powders):
                    continue
                pname = next((v for pat, v in POWDERS if re.fullmatch(pat, a_powders[k], re.I)), a_powders[k])
                rows.append({"powder": "Alliant " + pname, "gauge": gauge, "hull_mm": hull_mm, "shot_g": float(m[1]), "wad": wad, "primer": primer, "shell": shell,
                             "charge_g": round(vals[0] * GRAIN_G, 4), "velocity_ms": round(int(m[2]) * FPS, 1),
                             "pressure_bar": f"{vals[1] * PSI_BAR:.0f}" if len(vals) > 1 else ""})
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, ["powder", "gauge", "hull_mm", "shot_g", "wad", "primer", "shell", "charge_g", "velocity_ms", "pressure_bar"])
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} loads -> {args.output}", file=sys.stderr)
    print("unknown powders:", dict(sorted(unknown.items(), key=lambda kv: -kv[1])[:10]), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
