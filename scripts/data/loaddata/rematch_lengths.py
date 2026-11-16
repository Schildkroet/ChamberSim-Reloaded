#!/usr/bin/env python3
"""Looks up the bullet lengths of existing sample files again with the current matcher (samples_common.Records.bullet).

    rematch_lengths.py samples.csv... [--write] [--backup DIR] [--list] [--estimates] [--stale]

Only rows whose length came from a record (len_source "database") and is the length of one of the records this matcher considers (same maker,
weight and diameter) are looked up again; rows matched another way (product numbers), photo measurements and estimates stay. The
bullet column is "Maker Type" (the maker is the longest record maker that starts it). Without --write it only reports what would change.
--write rewrites the files in place, after copying the originals to --backup (default: sources/samples_prematch).
--estimates: rows with an estimated length (len_source "estimate:<class>") are estimated again (estimate_length.length_in on the bullet column)
when the estimate's class changed to or from a copper class (2026-11-16: XPB, Lehigh, Cutting Edge, MonoFlex, Naturalis were lead-core
estimates); other estimates stay (their class can depend on text the extractor had and the bullet column lacks).
--stale: rows with a record length that no current record has (the record was measured or corrected after the extraction: 2026-11-16 about
20,000 rows, e.g. Hornady 165 gr GMX 27.8 mm, a lead-core length, the record 33 mm) get the length of the record the matcher finds now, or an
estimate when no record of the same construction is left (samples_common.Records.match).
"""
import argparse
import collections
import csv
import shutil
import sys
from pathlib import Path

from samples_common import IN, Records, length_in

HERE = Path(__file__).resolve().parent


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+", type=Path)
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--backup", type=Path, default=HERE / "sources" / "samples_prematch")
    ap.add_argument("--list", action="store_true", help="print every changed bullet")
    ap.add_argument("--data", type=Path, default=HERE.parents[2] / "data")
    ap.add_argument("--estimates", action="store_true", help="estimate again the rows whose estimate class changed")
    ap.add_argument("--stale", action="store_true", help="also look up rows whose record length no current record has")
    args = ap.parse_args()
    records = Records(args.data)
    makers = sorted({p["mname"] for p in records.projectiles}, key=len, reverse=True)
    total = collections.Counter()
    for path in args.files:
        with open(path, newline="", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            fields = reader.fieldnames or []
            rows = list(reader)
        if not {"bullet", "bullet_gr", "bullet_dia_mm", "bullet_len_mm", "len_source"} <= set(fields):
            continue
        changed, seen = 0, {}
        for r in rows:
            if args.estimates and r["len_source"].startswith("estimate:"):
                est, kind = length_in(float(r["bullet_gr"]), float(r["bullet_dia_mm"]) / IN, r["bullet"])
                old_kind = r["len_source"].split(":", 1)[1]
                if kind != old_kind and ("copper" in kind or "copper" in old_kind):  # other classes depend on text the column lacks
                    if args.list and (r["bullet"], r["bullet_gr"]) not in total:
                        total[(r["bullet"], r["bullet_gr"])] = 1
                        print(f"  {path.name}: {r['bullet']} {r['bullet_gr']} gr: {r['bullet_len_mm']} -> {est * IN:.2f} mm "
                              f"({r['len_source']} -> estimate:{kind})")
                    r["bullet_len_mm"] = f"{est * IN:.2f}"
                    r["len_source"] = "estimate:" + kind
                    changed += 1
                continue
            if r["len_source"] != "database":
                continue
            maker = next((m for m in makers if r["bullet"].lower().startswith(m.lower() + " ") or r["bullet"].lower() == m.lower()), None)
            if not maker:
                continue
            key = (maker, r["bullet"], r["bullet_gr"], r["bullet_dia_mm"])
            if key not in seen:
                lengths = {p["glen"] for p in records.candidates(maker, float(r["bullet_gr"]), float(r["bullet_dia_mm"]))}
                seen[key] = (records.bullet(maker, r["bullet"][len(maker):].strip(), float(r["bullet_gr"]), float(r["bullet_dia_mm"])), lengths)
            (length, source, tail), lengths = seen[key]
            old = float(r["bullet_len_mm"])
            if not any(abs(old - x) < 0.005 for x in lengths) and not (args.stale and lengths):
                continue  # matched another way (product number, other diameter): not this matcher's pick
            if abs(length - old) < 0.005 and source == "database":
                continue
            if args.list and (key, old) not in total:
                total[(key, old)] = 1
                print(f"  {path.name}: {r['bullet']} {r['bullet_gr']} gr: {old:.2f} -> {length:.2f} mm ({source})")
            r["bullet_len_mm"] = f"{length:.2f}"
            r["len_source"] = source
            if "tail_type" in r:
                r["tail_type"] = tail
            changed += 1
        print(f"{path.name}: {changed} of {len(rows)} rows changed")
        total["rows"] += changed
        if changed and args.write:
            args.backup.mkdir(parents=True, exist_ok=True)
            if not (args.backup / path.name).exists():
                shutil.copy2(path, args.backup / path.name)
            with open(path, "w", newline="", encoding="utf-8") as f:
                w = csv.DictWriter(f, fields)
                w.writeheader()
                w.writerows(rows)
    print(f"total: {total['rows']} rows changed" + ("" if args.write else " (dry run)"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
