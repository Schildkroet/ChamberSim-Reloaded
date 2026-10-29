#!/usr/bin/env python3
"""Reads the dimension tables of "Cartridges of the World" (15th edition, PDF text) and lists the cartridges the project has no caliber record for.

    cotw_dimensions.py "Cartridges of the World 15th Edition.pdf" [-o cotw_dimensions.csv] [--data data]

The book ends its chapters with tables: case type (A rimmed bottleneck ... L), bullet diameter, neck, shoulder, base and rim diameter, rim thickness,
case length, cartridge length (all inch), primer size. The tables carry no shoulder position, no capacity and no pressure. Rows are matched to the
project's caliber records by name (norm of the names, then by the numbers: bullet diameter, case length and base diameter within a tolerance, since the
book's names differ from the C.I.P. names); `match` says how.
"""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "loaddata"))
from samples_common import Records  # noqa: E402

IN = 25.4
NUM = r"(?:\d*\.\d+|\d+\.?|UNK/NA|UNK|Various|N/A|NA)\??"
ROW = re.compile(rf"^\s*(?P<name>\S.*?)\s{{2,}}(?P<type>[A-L])\s+(?P<bullet>{NUM})\s+(?P<neck>{NUM})\s+(?P<shoulder>{NUM})\s+(?P<base>{NUM})\s+"
                 rf"(?P<rim>{NUM})\s+(?P<rimt>{NUM})\s+(?P<caselen>{NUM})\s+(?P<ctge>{NUM})(?:\s+(?P<primer>\S+(?: \S+)?))?(?:\s+(?P<page>\d{{1,4}}))?\s*$")


def value(text: str):
    try:
        return float(text.rstrip("?"))
    except ValueError:
        return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("pdf", type=Path)
    ap.add_argument("-o", "--out", type=Path, default=Path("cotw_dimensions.csv"))
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[3] / "data")
    args = ap.parse_args()
    text = subprocess.run(["pdftotext", "-layout", str(args.pdf), "-"], capture_output=True, text=True, check=True).stdout
    records = Records(args.data)
    cals = list({id(c): c for c in records.calibers.values()}.values())
    rows, seen = [], set()
    for line in text.splitlines():
        m = ROW.match(line)
        if not m:
            continue
        name = re.sub(r"\*+$", "", re.sub(r"\s+", " ", m["name"])).strip()
        v = {k: value(m[k]) for k in ("bullet", "neck", "shoulder", "base", "rim", "rimt", "caselen", "ctge")}
        if not v["bullet"] or not v["caselen"] or (name, v["caselen"]) in seen:
            continue
        seen.add((name, v["caselen"]))
        cal = records.caliber(name)
        how = "name" if cal else ""
        if not cal and v["base"]:
            near = [c for c in cals if c.get("G1") and c.get("P1") and c.get("L3") and abs(c["G1"] - v["bullet"] * IN) < 0.45
                    and abs(c["P1"] - v["base"] * IN) < 0.25 and abs(c["L3"] - v["caselen"] * IN) < 0.6]
            if near:
                cal, how = near[0], "dimensions"
        rows.append({"name": name, "case_type": m["type"], **{k: ("" if x is None else x) for k, x in v.items()}, "primer": m["primer"] or "",
                     "match": how, "caliber": cal["cipname"] if cal else ""})
    with args.out.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0]))
        w.writeheader()
        w.writerows(rows)
    new = [r for r in rows if not r["match"]]
    print(f"{len(rows)} cartridges, {len(rows) - len(new)} match a project caliber, {len(new)} do not -> {args.out}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
