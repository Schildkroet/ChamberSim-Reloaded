#!/usr/bin/env python3
"""Puts the lengths (and ballistic coefficients) a maker publishes into the project's existing projectile records.

    apply_csv_lengths.py bullets_federal.csv [more.csv ...] [--data DIR] [--validated] [--dry-run]

A row of a CSV made by a *_fetch.py script (manufacturer, line, weight_gr, diameter_in, length_in, g1bc, g7bc) is matched to a record of the same
manufacturer by the part number in its name, else by diameter (0.06 mm), weight (0.6 gr) and, if several fit, the line in the name. The record gets the published length and BC (the
estimate note of `descr` goes; only the maker's file is rewritten). Rows that match no record are reported, bullets_to_json.py adds them.
--validated also sets gValid. Rows without a length are skipped."""
import argparse
import csv
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import recordfiles  # noqa: E402

MM = 25.4


def num(row: dict, key: str) -> float:
    try:
        return float(row.get(key) or 0)
    except ValueError:
        return 0.0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", nargs="+", type=Path)
    ap.add_argument("--data", type=Path, default=HERE.parents[2] / "data")
    ap.add_argument("--validated", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    folder = args.data / "projectiles"
    files: dict[str, tuple[Path, list[dict]]] = {}
    changed = unmatched = 0
    for path in args.csv:
        for row in csv.DictReader(open(path, newline="", encoding="utf-8")):
            length = num(row, "length_in") * MM
            if not length:
                continue
            maker = row["manufacturer"]
            target = folder / (recordfiles.slug(maker) + ".json")
            if maker not in files:
                files[maker] = (target, recordfiles.records_of(target))
            records = files[maker][1]
            dia, mass = num(row, "diameter_in") * MM, num(row, "weight_gr")
            part = row.get("part") or "\0"
            fit = [r for r in records if part in r["pname"].split() and abs(r["gmass"] - mass) < 0.6]  # the part number is in the name
            if not fit:
                fit = [r for r in records if abs(r["gdia"] - dia) < 0.06 and abs(r["gmass"] - mass) < 0.6]
                if len(fit) > 1:
                    fit = [r for r in fit if row["line"].lower() in r["pname"].lower()] or fit
            if len(fit) != 1:
                print(f"{'no record' if not fit else 'ambiguous'}: {maker} {row['line']} {row.get('part', '')} {mass:g} gr {dia:.2f} mm")
                unmatched += 1
                continue
            rec = fit[0]
            new = {"glen": round(length, 1)}
            for col, key in (("g1bc", "g1bc"), ("g7bc", "g7bc")):
                if num(row, col):
                    new[key] = num(row, col)
            if args.validated:
                new["gValid"] = True
            if any(rec.get(k) != v for k, v in new.items()):
                print(f"{rec['pname']}: length {rec['glen']} -> {new['glen']}", {k: v for k, v in new.items() if k != "glen"})
                rec.update(new)
                rec.pop("descr", None) if "estimated" in rec.get("descr", "") else None
                changed += 1
    print(f"{changed} records updated, {unmatched} rows without a unique record", file=sys.stderr)
    if not args.dry_run:
        for path, records in files.values():
            path.write_text(json.dumps(sorted(records, key=recordfiles.key), indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
