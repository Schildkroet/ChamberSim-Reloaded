#!/usr/bin/env python3
"""Reads C.I.P. TDCC data sheets (PDF, tables I-V) and writes one CSV row per cartridge.

    cip_extract.py <pdf dir or files...> -o cip_calibers.csv

Needs `pdftotext` (poppler). The CSV columns are the column names of the project's caliber records
(see data/README.md), so scripts/data/cipdata/cip_to_json.py can turn the rows into data/calibers/*.json.
The sheets have two columns of values: the cartridge (maximum) on the left, the chamber (minimum) on the right.
Units as in the sheets: lengths mm, pressures bar, energy J, angles are converted to decimal degrees.
Shotshell sheets (table VII) have another layout and are not read here. Decimal commas in names become dots.
"""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

# sheet label (without footnote marks) -> column; left block = cartridge, right block = chamber
LEFT = {
    "L1": "L1", "L2": "L2", "L3": "L3", "L4": "L4", "L5": "L5", "L6": "L6", "R": "R", "R1": "R1", "R3": "R3",
    "E": "E", "E1": "E1", "e min": "Emin", "δ": "delta", "f": "f", "β": "beta", "P1": "P1", "P2": "P2",
    "α": "alpha", "S": "S", "r1 min": "r1min", "r2": "r2", "H1": "H1", "H2": "H2", "G1": "G1", "G2": "G2",
    "L3+G": "L3G", "Pmax": "Pmax", "PK": "PK", "PE": "PE", "EE": "EE", "M": "M", "Fe": "c_Fe",
}
RIGHT = {
    "L0": "c_L0", "L1": "c_L1", "L2": "c_L2", "L3": "c_L3", "R": "c_R", "R1": "c_R1", "R2": "c_R2", "R3": "c_R3",
    "r": "c_r_", "E": "c_E", "P0": "c_P0", "P1": "c_P1", "P2": "c_P2", "α": "c_alpha", "S": "c_S",
    "r1 max": "c_r1max", "r2": "c_r2_", "H1": "c_H1", "H2": "c_H2", "G": "c_G", "G1": "c_G1", "α1": "c_alpha1",
    "h": "c_h_", "s": "c_s_", "i": "c_i_", "w": "c_w_", "F": "c_F", "Z": "c_Z", "b": "c_b_", "N": "c_N",
    "u": "c_u_", "Q": "c_Q", "δ": "c_delta", "Fe": "c_Fe",
}
# the order of the left block; a value whose label was lost by the PDF text layer (Greek letters) takes the next one
LEFT_ORDER = ["L1", "L2", "L3", "L4", "L5", "L6", "R", "R1", "R3", "E", "E1", "e min", "δ", "f", "β", "P1", "P2", "α", "S",
              "r1 min", "r2", "H1", "H2", "G1", "G2"]
ANGLES = {"delta", "beta", "alpha", "c_alpha", "c_alpha1", "c_i_", "c_delta"}

FIELD = re.compile(r"(?P<label>[^\s=][^=]*?)\s*=\s*(?P<value>-?\d[\d.,]*(?:°\s*\d*'?\s*\d*\"?)?|\s*(?=\s{2}|$))")
LABEL_CLEAN = re.compile(r"\s*(\d\)|\*)+\s*$")


def angle(text: str) -> float:
    m = re.fullmatch(r"(\d+(?:[.,]\d+)?)°\s*(?:(\d+)'?)?\s*(?:(\d+(?:\.\d+)?)\")?", text.strip())
    if not m:
        return float(text.replace(",", "."))
    return round(float(m[1].replace(",", ".")) + int(m[2] or 0) / 60 + float(m[3] or 0) / 3600, 4)


def clean_label(raw: str) -> str:
    raw = raw.strip()
    while True:
        new = LABEL_CLEAN.sub("", raw)
        new = re.sub(r"(\s+\d\))+$", "", new)
        if new == raw:
            return raw
        raw = new


def date(text: str) -> str:
    m = re.fullmatch(r"(\d\d)-(\d\d)-(\d\d)", text.strip())
    if not m:
        return ""
    yy = int(m[1])
    return f"{1900 + yy if yy > 30 else 2000 + yy}-{m[2]}-{m[3]}"


def parse_block(text: str, table: dict, order: list | None) -> dict:
    out = {}
    pos = 0
    for m in re.finditer(r"([^=\n]*?)=[ \t]*(-?\d[\d.,]*(?:[ \t]*°[ \t]*\d*'?[ \t]*\d*\"?)?)?", text):
        label = clean_label(m[1])
        value = m[2]
        label = label.split("  ")[-1].strip()
        if not label and value and "°" in value:
            if order:  # cartridge side: delta follows e min, alpha follows P2
                label = "δ" if "Emin" in out and "delta" not in out and "beta" not in out else "α"
            else:  # chamber side: the label of the angle "i" is printed one line above its value
                label = "i" if "c_alpha1" in out else "α"
        if not label and value and "°" not in value:  # "L3 1)" and its value are printed on separate lines in some sheets
            l3, l2 = ("L3", "L2") if order else ("L3", "L2")
            if table[l2] in out and table[l3] not in out:
                label = l3
        if label == "α*":
            label = "α"
        if label not in table or not value:
            continue
        col = table[label]
        v = value.strip()
        out[col] = angle(v) if col in ANGLES else (float(v.replace(",", ".")) if col != "M" else v)
        pos = m.end()
    return out


def parse_page(page: str, source: str) -> dict | None:
    lines = [ln for ln in page.splitlines() if ln.strip()]
    head = next((i for i, ln in enumerate(lines) if "C.I.P." in ln), None)
    if head is None:
        return None
    # the name sits between "C.I.P." and "Date", usually on the same line, in some sheets one line above or below
    block = lines[max(head - 1, 0):head + 3]
    dline = next((ln for ln in block if re.search(r"\sDat(?:e|um)\s", ln)), "")
    md = re.search(r"Dat(?:e|um)\s+(\S*)", dline)
    cdate = date(md[1]) if md else ""
    name = ""
    for ln in block:
        t = re.sub(r"Dat(?:e|um)\s+\S*", "", ln.replace("C.I.P.", ""))
        if re.search(r"TAB\.|Country|Ursprungsland|Revision|Maximum|MAXI|Cartridge|PATRONE", t) or not t.strip():
            continue
        name = re.sub(r"(?<=\d),(?=\d)", ".", t.strip())  # "6,5 x 55" -> "6.5 x 55"
        break
    if not name:
        return None
    nxt = next((ln for ln in block if re.search(r"Origin|Ursprungsland", ln)), "")
    country = re.search(r"(?:Origin|Ursprungsland):\s*(\w+)", nxt)
    rev = re.search(r"Revision\s+(\S+)", "\n".join(block))
    table = re.search(r"TAB\.\s+(\S+)", "\n".join(lines[:head + 1]))
    # column split: the right (chamber) block starts a little left of the word CHAMBER
    ch = next((ln.find(k) for ln in lines for k in ("CHAMBER", "PATRONENLAGER", "Minimum Chamber") if k in ln), -1)
    if ch < 0:
        return None
    split = ch - 14
    left_lines, right_lines = [], []
    for ln in lines[head + 2:]:
        left_lines.append(ln[:split].rstrip())
        right_lines.append(ln[split:].rstrip())
    if table and table[1] == "VII":
        return None  # shotshells: other layout (diameter, chamber length, ...)
    row = {"cipname": name, "ciporigin": country[1] if country else "", "cipdate": cdate,
           "ciprevdate": date(rev[1]) if rev else "", "table": table[1] if table else "", "source": source}
    left = parse_block("\n".join(left_lines), LEFT, LEFT_ORDER)
    # tolerance values ("-0.20") hide behind the '=' search only when they follow the value, so nothing to remove
    right = parse_block("\n".join(right_lines), RIGHT, None)
    row |= left | right
    # pressures with units are matched as numbers; the energy is in Joule
    return row


def pdf_pages(path: Path) -> list[str]:
    txt = subprocess.run(["pdftotext", "-layout", str(path), "-"], capture_output=True, text=True, errors="replace").stdout
    return [p for p in txt.split("\f") if p.strip()]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("inputs", nargs="+", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("cip_calibers.csv"))
    args = ap.parse_args()
    files = []
    for p in args.inputs:
        files += sorted(p.glob("*.pdf")) if p.is_dir() else [p]
    rows, skipped = [], []
    for f in files:
        for page in pdf_pages(f):
            row = parse_page(page, f.name)
            (rows if row else skipped).append(row or f.name)
    cols = ["cipname", "altname", "ciporigin", "table", "cipdate", "ciprevdate", *dict.fromkeys([*LEFT.values(), *RIGHT.values()]), "source"]
    with open(args.output, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, cols, extrasaction="ignore")
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} cartridges from {len(files)} files -> {args.output}; {len(skipped)} pages not read", file=sys.stderr)
    for s in sorted(set(skipped)):
        print("  not read:", s, file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
