#!/usr/bin/env python3
"""Merges a QuickLOAD bullet file (.bul) into the maker's existing projectile records instead of replacing the maker's file (bul_to_json.py).

    apply_bul_lengths.py FILE.bul --maker Fox --source "..." --line "Classic Hunter Bleifrei" [--validated] [--dry-run]

Every bullet of the file gets the record `<--line> <diameter> <grains> gr` of the maker: the length, base data and BC of the file replace the
record's (a former estimate or photo measurement), a bullet without a record is added. Other records of the maker (other lines) stay."""
import argparse
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
sys.path.insert(0, str(HERE))
import recordfiles  # noqa: E402
from bul_to_json import caliber_names, records  # noqa: E402


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("bul", type=Path)
    ap.add_argument("--maker", required=True)
    ap.add_argument("--source", required=True)
    ap.add_argument("--line", required=True, help="the line's name in the records")
    ap.add_argument("--pressure", type=float, default=350.0)
    ap.add_argument("--data", type=Path, default=HERE.parents[2] / "data" / "projectiles")
    ap.add_argument("--validated", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    target = args.data / (recordfiles.slug(args.maker) + ".json")
    existing = recordfiles.records_of(target)
    updated = added = 0
    for new in records(args.bul, args.maker, args.source, lambda line: args.pressure, caliber_names(args.data), ""):
        stem = new["pname"].split(" ", new["pname"].count(" ") - 2)[-1]  # "<diameter> <grains> gr"
        name = f"{args.line} {'0' if stem.startswith('.') else ''}{stem}"  # the records say 0.224, the file .224
        new["pname"] = name
        if args.validated:
            new["gValid"] = True
        old = next((r for r in existing if r["pname"] == name), None)
        if old:
            keep = {k: old[k] for k in ("gpressure", "gfriction") if k in old}
            old.clear()
            old.update({**new, **keep})
            updated += 1
        else:
            existing.append(new)
            added += 1
            print("new:", name, new["glen"], "mm")
    print(f"{updated} records updated, {added} added", file=sys.stderr)
    if not args.dry_run:
        target.write_text(json.dumps(sorted(existing, key=recordfiles.key), indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
