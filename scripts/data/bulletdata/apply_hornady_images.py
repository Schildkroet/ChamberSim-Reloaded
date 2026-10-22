#!/usr/bin/env python3
"""Puts the lengths measured on the Hornady illustrations (hornady_images.py, measure_side_photos.py) into the Hornady records.

    apply_hornady_images.py measured.csv manifest.csv [--max-deviation 0.30] [--data DIR] [--dry-run]

A record (named "<line> <sku>") whose length is only an estimate (descr "Length estimated ...") gets the measured one when the measurement has no
flag and lies within --max-deviation of the estimate (a picture that disagrees more is not trusted). Validated records and records with a
measured or published length are not touched. Compared with the 54 validated records the illustrations are 1 % too long on average (5 % rms)."""
import argparse
import csv
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import recordfiles  # noqa: E402

NOTE = "Length measured on the illustration of hornady.com (side view of the bullet, length / width x diameter), about 5 % uncertain"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("measured", type=Path)
    ap.add_argument("manifest", type=Path)
    ap.add_argument("--max-deviation", type=float, default=0.30)
    ap.add_argument("--data", type=Path, default=HERE.parents[2] / "data")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    sku = {r["file"]: r["sku"] for r in csv.DictReader(open(args.manifest, newline="", encoding="utf-8"))}
    target = args.data / "projectiles" / "hornady.json"
    records = recordfiles.records_of(target)
    used = far = flagged = 0
    for m in csv.DictReader(open(args.measured, newline="", encoding="utf-8")):
        if m["flag"]:
            flagged += 1
            continue
        rec = next((r for r in records if r["pname"].endswith(" " + sku.get(m["file"], "\0")) and abs(r["gmass"] - float(m["weight_gr"])) < 0.6), None)
        if not rec or "estimated" not in rec.get("descr", ""):
            continue
        length = float(m["length_mm"])
        if abs(length / rec["glen"] - 1) >= args.max_deviation:
            far += 1
            print(f"too far: {rec['pname']} {rec['glen']} -> {length}")
            continue
        rec["glen"], rec["descr"] = round(length, 1), NOTE
        used += 1
    print(f"{used} lengths replaced, {far} too far from the estimate, {flagged} measurements flagged", file=sys.stderr)
    if not args.dry_run:
        target.write_text(json.dumps(sorted(records, key=recordfiles.key), indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
