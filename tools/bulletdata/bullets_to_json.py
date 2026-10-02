#!/usr/bin/env python3
"""Writes the project's projectile records (data/projectiles/*.json) from CSVs made by the *_fetch.py scripts.

    bullets_to_json.py bullets_berger.csv [more.csv ...] [-o data/projectiles] [--overwrite]

CSV columns (units of the manufacturers' sites): manufacturer, line, part, weight_gr, diameter_in, length_in, g1bc, g7bc,
base_style (Boat Tail / Flat Base / Hollow Base), boat_tail_length_in, valid, note. Rows with valid = no (data insufficient,
or excluded by hand) are not written. A row without length gets an estimate from mass, diameter and bullet type
(estimate_length.py) and a note in `descr`. Records use the units of the database: mm for
diameter and length, grain for the mass. The name is "<line> <part number>", the caliber the diameter in thousandths of
an inch (".308"). Without a part number the diameter and weight stand in its place. Zero or missing values are left out.
"""
import argparse
import csv
import json
import re
import sys
import unicodedata
from pathlib import Path

from estimate_length import length_in

MM = 25.4


def slug(*parts: str) -> str:
    s = unicodedata.normalize("NFKD", "-".join(p for p in parts if p)).lower()
    out = ""
    for c in s:
        if c.isascii() and c.isalnum():
            out += c
        elif out and not out.endswith("-"):
            out += "-"
    return out.strip("-")


def f(row: dict, key: str) -> float:
    try:
        return float(row.get(key) or 0)
    except ValueError:
        return 0.0


def record(row: dict) -> dict:
    row = {k: re.sub(r"[®™]", "", v or "").strip() for k, v in row.items()}  # trademark signs do not belong in names
    estimated = ""
    if not f(row, "length_in") and f(row, "weight_gr") and f(row, "diameter_in"):
        text = " ".join(row.get(k, "") for k in ("line", "ogive_style", "base_style"))
        est, kind = length_in(f(row, "weight_gr"), f(row, "diameter_in"), text)
        row = {**row, "length_in": est}
        estimated = f"Length estimated from mass, diameter and bullet type ({kind}), about 10 % uncertain"
    dia, length, tail = f(row, "diameter_in") * MM, f(row, "length_in") * MM, f(row, "boat_tail_length_in") * MM
    style = (row.get("base_style") or "").lower()
    boat = "boat" in style
    rec = {"mname": row["manufacturer"], "pname": " ".join(p for p in (row.get("line"), row.get("part") or f"{row.get('diameter_in', '')} {f(row, 'weight_gr'):g} gr".strip()) if p), "lotid": "",
           "caliber": "." + str(round(f(row, "diameter_in") * 1000)) if dia else "", "gdia": round(dia, 2),
           "glen": round(length, 1), "gmass": f(row, "weight_gr"), "gtailtype": 2 if boat else (1 if "hollow" in style else 0),
           "gtaildiaA": round(dia, 2) if boat else 0, "gtailh": round(tail, 1) if boat else 0, "g1bc": f(row, "g1bc"),
           "g7bc": f(row, "g7bc")}
    if estimated:
        rec["descr"] = estimated
    return {k: v for k, v in rec.items() if v not in (0, 0.0, "")}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", nargs="+", type=Path)
    ap.add_argument("-o", "--output", type=Path, default=Path("data/projectiles"))
    ap.add_argument("--overwrite", action="store_true")
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    written = kept = skipped = 0
    names = set()
    for path in args.csv:
        with open(path, newline="", encoding="utf-8") as fh:
            for row in csv.DictReader(fh):
                if (row.get("valid") or "yes").strip().lower() == "no":
                    skipped += 1
                    continue
                rec = record(row)
                if not all(k in rec for k in ("gdia", "glen", "gmass")):
                    print("incomplete:", row.get("url") or row, file=sys.stderr)
                    continue
                name = slug(rec["mname"], rec["pname"])
                if name in names:
                    print("duplicate name:", rec["mname"], rec["pname"], file=sys.stderr)
                    continue
                names.add(name)
                out = args.output / (name + ".json")
                if out.exists() and not args.overwrite:
                    kept += 1
                    continue
                body = ",\n".join(f"  {json.dumps(k)}: {json.dumps(v, ensure_ascii=False)}" for k, v in rec.items())
                out.write_text("{\n" + body + "\n}\n", encoding="utf-8")
                written += 1
    print(f"{written} files written, {kept} kept, {skipped} rows marked invalid", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
