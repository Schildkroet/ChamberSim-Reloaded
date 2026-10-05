#!/usr/bin/env python3
"""Writes the project's caliber records (data/calibers/*.json) from the CSV made by cip_extract.py.

    cip_to_json.py cip_calibers.csv [wiki_calibers.csv] [-o data/calibers] [--overwrite]

One file per cartridge, named like the program names exported records (lower case ASCII slug of the name).
Empty CSV cells are left out of the record. Files that exist are kept unless --overwrite is given.
"""
import argparse
import csv
import json
import re
import sys
import unicodedata
from pathlib import Path

# columns of a caliber record in schema order (data/README.md); everything else in the CSV is not written
COLUMNS = ["cipname", "altname", "standard", "ciporigin", "ciptype", "cipdate", "ciprevdate", "cippdf", "L0", "L1", "L2", "L3",
           "L4", "L5", "L6", "R", "R1", "R3", "E", "E1", "Emin", "delta", "FG", "beta", "P0", "P1", "P2", "alpha", "S", "r1min",
           "r2", "H1", "H2", "G1", "G2", "f", "L3G", "Pmax", "PK", "PE", "EE", "V", "M", "c_L0", "c_L1", "c_L2", "c_L3", "c_R",
           "c_R1", "c_R2", "c_R3", "c_r_", "c_E", "c_P0", "c_P1", "c_P2", "c_alpha", "c_S", "c_r1max", "c_r2_", "c_H1", "c_H2",
           "c_G", "c_G1", "c_alpha1", "c_h_", "c_s_", "c_i_", "c_w_", "c_F", "c_Z", "c_b_", "c_N", "c_u_", "c_Q", "c_delta",
           "c_Fe", "c_Fe_proof", "c_Fe_auto", "sebert", "origin", "descr"]
TEXT = {"cipname", "altname", "standard", "ciporigin", "ciptype", "cipdate", "ciprevdate", "cippdf", "delta", "beta", "alpha",
        "M", "origin", "descr"}
CIPTYPE = {"I": "1 rimless", "II": "2 rimmed", "III": "3 magnum", "IV": "4 pistol/revolver", "V": "5 rimfire",
           "VII": "7 shotshell"}
BASE_URL = "https://bobp.cip-bobp.org/uploads/tdcc/"


def slug(name: str) -> str:
    s = unicodedata.normalize("NFKD", name).lower()
    out = ""
    for c in s:
        if c.isascii() and c.isalnum():
            out += c
        elif out and not out.endswith("-"):
            out += "-"
    return out.strip("-") or "record"


def alt_names(cipname: str, given: str) -> str:
    """Other names of a cartridge, comma separated: those entered in the CSV plus the inch spelling with a leading
    dot (".308 Win." for the C.I.P. name "308 Win.") for designations that are inch calibers in hundredths."""
    names = [n.strip() for n in given.split(",") if n.strip()]
    m = re.match(r"(\d{2,3})(?:\s+(?!mm\b|x\b)|-|$)", cipname)
    if m and ("." + cipname) not in names:
        names.insert(0, "." + cipname)
    return ", ".join(names)


def record(row: dict) -> dict:
    row = dict(row)
    row["altname"] = alt_names(row.get("cipname", ""), row.get("altname", ""))
    row.setdefault("standard", "CIP")
    row["standard"] = row.get("standard") or "CIP"
    row["ciptype"] = row.get("ciptype") or CIPTYPE.get(row.get("table", ""), "")
    row["origin"] = row.get("origin") or "cip-bobp.org"
    if row.get("source") and not row.get("cippdf") and row["source"].endswith(".pdf"):
        row["cippdf"] = BASE_URL + row["source"].replace("__", "/", 1)
    rec = {}
    for c in COLUMNS:
        v = (row.get(c) or "").strip()
        if not v:
            continue
        if c in TEXT:
            rec[c] = v
        else:
            num = float(v)
            rec[c] = int(num) if num.is_integer() and abs(num) < 1e15 else num
    return rec


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", type=Path, nargs="+", help="more CSVs (wiki_calibers.csv) only add cartridges not in the first")
    ap.add_argument("-o", "--output", type=Path, default=Path("data/calibers"))
    ap.add_argument("--overwrite", action="store_true")
    args = ap.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    written = skipped = 0
    seen = {}
    shapes = []  # (G1, L3, P1) of the records written so far: later CSVs only add cartridges that are not there yet
    for number, csv_path in enumerate(args.csv):
        with open(csv_path, newline="", encoding="utf-8") as f:
            rows = list(csv.DictReader(f))
        for row in rows:
            rec = record(row)
            if not rec.get("cipname"):
                continue
            if not rec.get("L3") or not rec.get("V"):  # the program needs the case length and capacity
                print(f"skipped {rec['cipname']!r}: no L3 or V ({row.get('source')})", file=sys.stderr)
                continue
            shape = (rec.get("G1", 0), rec["L3"], rec.get("P1", 0))
            if number > 0 and any(abs(shape[0] - g) < 0.06 and abs(shape[1] - l) < 0.6 and abs(shape[2] - p) < 0.25 for g, l, p in shapes):
                print(f"already there: {rec['cipname']!r}", file=sys.stderr)  # same bullet diameter, case length and base diameter
                continue
            path = args.output / (slug(rec["cipname"]) + ".json")
            if path in seen:
                print(f"duplicate name {rec['cipname']!r}: {row.get('source')} and {seen[path]}", file=sys.stderr)
                continue
            seen[path] = row.get("source")
            shapes.append(shape)
            if path.exists() and not args.overwrite:
                skipped += 1
                continue
            body = ",\n".join(f"  {json.dumps(k)}: {json.dumps(v, ensure_ascii=False)}" for k, v in rec.items())
            path.write_text("{\n" + body + "\n}\n", encoding="utf-8")
            written += 1
    print(f"{written} files written, {skipped} kept", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
