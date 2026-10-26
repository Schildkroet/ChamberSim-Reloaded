#!/usr/bin/env python3
"""Replaces the default bulk density (pcd = 900 kg/m3) of project propellant records by a sourced value.

    apply_bulk_densities.py <powders.json of github.com/fbastin/tireur_reloaded (MIT)> [--db tools/GordonsReloadingTool.plain.db] [--dry-run]

Per record with pcd = 900, in this order: the density with a maker source in that file (pcd_src: Vihtavuori, Shooters World, Lee, Nosler ...),
else the density of the original program's record of the powder, else the density of that file without a source (its own source is the
original program's records and ammoreference.com). Records without any of them keep 900. The source goes into `descr`."""
import argparse
import json
import re
import sqlite3
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
import recordfiles  # noqa: E402


def norm(s: str) -> str:
    return re.sub(r"[^a-z0-9]", "", s.lower())


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("powders", type=Path)
    ap.add_argument("--db", type=Path, default=HERE.parents[2] / "tools" / "GordonsReloadingTool.plain.db")
    ap.add_argument("--data", type=Path, default=HERE.parents[2] / "data")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    ext = {norm(v["mfg"] + v["name"]): v for v in json.loads(args.powders.read_text(encoding="utf-8"))["powders"].values() if v.get("pcd")}
    con = sqlite3.connect(f"file:{args.db}?mode=ro", uri=True)
    orig: dict[str, float] = {}
    for m, n, p in con.execute("SELECT mname, pname, pcd FROM propellant WHERE geloescht = 0 AND pcd > 0 ORDER BY id"):
        orig[norm(m + n)] = p  # the newest record of the powder
    counts = {"maker source": 0, "original program": 0, "unsourced": 0, "kept": 0}
    for path in sorted((args.data / "propellants").glob("*.json")):
        records = recordfiles.records_of(path)
        changed = False
        for r in records:
            if r.get("pcd") != 900.0:
                continue
            k = norm(r["mname"] + r["pname"])
            e = ext.get(k)
            if e and e.get("pcd_src"):
                pcd, how, note = e["pcd"], "maker source", f"Bulk density {e['pcd']:g} kg/m3: {e['pcd_src']}"
            elif k in orig:
                pcd, how, note = orig[k], "original program", f"Bulk density {orig[k]:g} kg/m3 of the original program's record"
            elif e:
                pcd, how, note = e["pcd"], "unsourced", f"Bulk density {e['pcd']:g} kg/m3 as listed by tireur.org (original program's records, ammoreference.com)"
            else:
                counts["kept"] += 1
                continue
            counts[how] += 1
            if pcd != 900.0:
                print(f"{r['mname']} {r['pname']}: pcd 900 -> {pcd:g} ({how})")
            r["pcd"] = float(pcd)
            r["descr"] = (r.get("descr", "").rstrip() + " " + note + ".").strip()
            changed = True
        if changed and not args.dry_run:
            path.write_text(json.dumps(records, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(counts, file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
