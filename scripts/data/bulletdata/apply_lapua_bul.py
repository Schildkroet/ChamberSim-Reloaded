#!/usr/bin/env python3
"""Merges Lapua's QuickLOAD bullet file (lapua.bul, 3-Feb-15) into data/projectiles/lapua.json.

    apply_lapua_bul.py lapua.bul [--dry-run]

Lapua's records are named after the product code of lapua.com (E539, GB544, ...), so a bullet of the file is matched by that code and the weight.
A match gets the length, base data and BC of the file (a former estimate or picture measurement is replaced, the name and the start pressure stay);
a bullet without a record is added as `<line> <diameter> <grains> gr`. The G7 rows of the file (separate lines) are merged into the G1 row."""
import argparse
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
sys.path.insert(0, str(HERE))
import recordfiles  # noqa: E402
from bul_to_json import caliber_names, records  # noqa: E402

SOURCE = "bullet file of Lapua (lapua.bul, 3-Feb-15)"
CODE = re.compile(r"\b([A-Z]{1,2}u?\d{3})\b", re.I)


def code_of(name: str) -> str | None:
    m = CODE.findall(name)
    return m[-1].upper() if m else None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("bul", type=Path)
    ap.add_argument("--data", type=Path, default=HERE.parents[2] / "data" / "projectiles")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    target = args.data / "lapua.json"
    existing = recordfiles.records_of(target)
    merged: list[dict] = []
    for new in records(args.bul, "Lapua", SOURCE, lambda line: 350.0, caliber_names(args.data), r"^Lapua\s+"):
        words = new["pname"].split()
        line, nominal, grains = " ".join(words[:-3]), words[-3], words[-2]
        g7 = bool(re.search(r"\bG7\b|\d{4}G7\b", line))
        line = re.sub(r"\s+\d{4}(?:G7)?\b|\s+G7\b", "", line).strip()  # product number and G7 mark are not part of the name
        if g7 and "g1bc" in new:  # the file leaves the BC type empty in some G7 rows
            new["g7bc"] = new.pop("g1bc")
        new["line"], new["nominal"], new["code"], new["gr"] = line, nominal, code_of(line), grains
        twin = next((m for m in merged if (m["caliber"], m["gmass"], m["line"], m["glen"]) == (new["caliber"], new["gmass"], line, new["glen"])), None)
        if twin and g7:
            twin["g7bc"] = new["g7bc"]
        elif twin and "g7bc" in twin and "g1bc" not in twin:  # the G7 row came first
            twin["g1bc"] = new["g1bc"]
        elif not twin:
            merged.append(new)
    originals, taken = list(existing), set()
    updated = added = 0
    for new in merged:
        old = next((r for r in originals if id(r) not in taken and new["code"] and code_of(r["pname"]) == new["code"]
                    and r["caliber"] == new["caliber"] and abs(r["gmass"] - new["gmass"]) < 1.5), None)
        out = {k: v for k, v in new.items() if k not in ("line", "nominal", "code", "gr")}
        out["gValid"] = True
        out.pop("descr", None)  # the records of this file carry no description
        if old:
            taken.add(id(old))
            keep = {k: old[k] for k in ("pname", "gpressure", "gfriction") if k in old}
            if "gtailtype" not in out and "gtailtype" in old:
                keep |= {k: v for k, v in old.items() if k.startswith("gtail")}
            print(f"update {old['pname']:34} {old['glen']:6.2f} -> {out['glen']:6.2f} mm")
            old.clear()
            old.update({**out, **keep})
            updated += 1
        else:
            out["pname"] = f"{new['line']} {new['nominal']} {new['gr']} gr"
            if any(r["pname"] == out["pname"] and r["caliber"] == out["caliber"] for r in existing):
                out["pname"] += f" {out['glen']:g}"  # the file lists two versions of the bullet
            existing.append(out)
            added += 1
    print(f"{updated} records updated, {added} added, {len(existing)} in total", file=sys.stderr)
    if not args.dry_run:
        target.write_text(json.dumps(sorted(existing, key=recordfiles.key), indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
