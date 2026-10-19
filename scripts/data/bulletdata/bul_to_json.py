#!/usr/bin/env python3
"""Converts a QuickLOAD bullet file (.bul, as published by bullet makers) into projectile records of data/projectiles.

    bul_to_json.py FILE.bul --maker "Styria Arms" --source "bullet file of Styria Arms Mayerl, 01-Oct-2026" [--write]

A line of the file (CSV, quoted): name ("<caliber>, <grains>, <line>[, <variant>] - smax:<mm>" or "<caliber> / <grains>gr <line>"), grains,
length [inch], base diameter [mm], diameter where the boat tail starts [mm] (smaller than the bullet for a rebated boat tail), boat-tail
length [mm], BC type (G1 or G7), base type (2 = boat tail, empty = not given), bullet diameter [inch], BC, BC, ...
Lengths and diameters become mm, the mass stays in grains. A base type is only written where the file has one. The start pressure is
--pressure (default 350 bar, copper), 290 bar for the lines that match --brass. Without --write the records are summarised; with --write the
maker's file data/projectiles/<maker>.json is replaced."""
import argparse
import csv
import io
import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import recordfiles  # noqa: E402

MM = 25.4


def number(text: str) -> float:
    return float(text.replace(",", "."))


def caliber_names(data: Path) -> dict[str, tuple[float, int]]:
    """The caliber designations of the project's projectile records (".308") with their diameter [mm] and number of records."""
    count: dict[str, int] = {}
    for f in data.glob("*.json"):
        for r in json.loads(f.read_text(encoding="utf-8")):
            if re.fullmatch(r"\.\d{3}", r.get("caliber", "")):
                count[r["caliber"]] = count.get(r["caliber"], 0) + 1
    return {c: (float(c) * MM, n) for c, n in count.items()}


def records(path: Path, maker: str, source: str, pressure, calibers: dict[str, tuple[float, int]], strip: str) -> list[dict]:
    lines = path.read_text(encoding="latin-1").splitlines()
    out = []
    for row in csv.reader(io.StringIO("\n".join(l for l in lines if not l.startswith(";")))):
        if len(row) < 11:
            continue  # the " 230 [bullet]" line
        head, _, smax = row[0].partition("smax:")
        m = re.match(r"\s*(\.\d+)[^,/]*[,/]\s*([\d.]+)\s*(?:gr)?\s*,?\s*(.*?)[\s-]*$", head)
        nominal, grains, line = m[1], m[2], re.sub(strip, "", m[3]).strip() if strip else m[3].strip()
        dia = round(float(row[8]) * MM, 2)
        # the designation the project's records use for this diameter (.242 -> .243, .365 -> .366): the most common one within 0.05 mm
        near = [c for c in calibers if abs(calibers[c][0] - dia) < 0.05] or [min(calibers, key=lambda c: abs(calibers[c][0] - dia))]
        caliber = max(near, key=lambda c: calibers[c][1])
        rec = {"mname": maker, "pname": f"{line} {nominal} {number(grains):g} gr", "caliber": caliber, "gdia": dia,
               "glen": round(float(row[2]) * MM, 2), "gmass": number(row[1])}
        if row[7] == "2" and row[3] and row[5]:
            rec |= {"gtailtype": 2, "gtaildiaA": number(row[4]) if row[4] else dia, "gtaildiaB": number(row[3]), "gtailh": number(row[5])}
        if smax.strip():
            rec["gdepthmax"] = number(smax.strip())
        rec["g7bc" if row[6] == "G7" else "g1bc"] = number(row[9])
        rec["descr"] = f"Dimensions and ballistic coefficient (ICAO, {row[6]}) from the manufacturer's {source}"
        rec["gpressure"] = pressure(line)
        out.append(rec)
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("bul", type=Path)
    ap.add_argument("--maker", required=True)
    ap.add_argument("--source", required=True)
    ap.add_argument("--brass", default=r"(?!)", help="regular expression of the lines that are solid brass (290 bar), e.g. \\bSB\\b")
    ap.add_argument("--pressure", type=float, default=350.0, help="start pressure [bar] of the lines that are not --brass")
    ap.add_argument("--strip", default="", help="regular expression removed from the line name (the maker's name)")
    ap.add_argument("--data", type=Path, default=Path("data/projectiles"))
    ap.add_argument("--write", action="store_true")
    args = ap.parse_args()
    recs = records(args.bul, args.maker, args.source, lambda line: 290.0 if re.search(args.brass, line) else args.pressure,
                   caliber_names(args.data), args.strip)
    names = [r["pname"] for r in recs]
    dup = {n for n in names if names.count(n) > 1}
    for r in recs:  # versions of one bullet that differ in length and seating depth: the depth tells them apart
        if r["pname"] in dup:
            r["pname"] += f" smax {r['gdepthmax']:g}"
    print(f"{len(recs)} records, {sum('gtailtype' in r for r in recs)} with a base type, {len(dup)} names made unique with the seating depth")
    if args.write:
        target = args.data / (recordfiles.slug(args.maker) + ".json")
        target.write_text(json.dumps(sorted(recs, key=lambda r: (r["caliber"], r["gmass"], r["pname"])), indent=2, ensure_ascii=False) + "\n",
                          encoding="utf-8")
        print("written", target)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
