#!/usr/bin/env python3
"""Adds the usual primer type to the project's caliber records (field primer: SP, SPM, LP, LPM, SR, SRM, LR, LRM or BMG; scripts/data/primers.py).

    caliber_primers.py [--write] [--data data] [--samples scripts/data/loaddata/sources/samples]

The program presets a new load's primer with it. Where the published load tables name a primer for the cartridge, the most frequent type wins:
Hodgdon and Alliant (one vote per test cartridge, i.e. caliber, bullet and primer) and the Vihtavuori reloading data (three votes per cartridge).
A tie, and a cartridge no table names, take the rule: the case's base diameter P1 below 11.0 mm (pistol and revolver, C.I.P. table IV) or
11.25 mm (rifle) is small, else large; belted and other "3 ..." case types are magnum; .50 BMG, 12.7 x 108 and 14.5 mm the BMG primer.
Rimfire cartridges get no primer. CORRECTIONS overrides the tables where they are evidently wrong.
"""
import argparse, collections, csv, glob, json, re, sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from primers import classify  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "loaddata"))
from vv_extract import norm  # noqa: E402


# the tables' primer is not the cartridge's usual one
CORRECTIONS = {
    "6.8 mm Rem. SPC": "SR",  # Hodgdon prints "WIN LR" for all 6.8 SPC loads; the case has a small rifle primer pocket
}


def rule(record: dict) -> str | None:
    """The primer type from the case: size from the base diameter, magnum from the case type."""
    ciptype = record.get("ciptype", "")
    if ciptype.startswith("5"): return None  # rimfire
    name = record.get("cipname", "")
    if re.search(r"\b(12[.,]7 x (99|108)|50 BMG|14[.,]5 x 114)", name, re.I): return "BMG"
    pistol = "/tab-iv/" in record.get("cippdf", "") or ciptype.startswith("4")
    p1 = float(record.get("P1") or record.get("R1") or 0)
    small = 0 < p1 < (11.0 if pistol else 11.25)
    code = ("S" if small else "L") + ("P" if pistol else "R")
    return code + "M" if ciptype.startswith("3") else code


def votes(samples: Path, calibers: dict) -> dict:
    out = collections.defaultdict(collections.Counter)
    for name in ("hod_samples.csv", "alliant_web_samples.csv"):
        f = samples / name
        if not f.exists(): continue
        seen = set()
        for r in csv.DictReader(open(f, encoding="utf-8")):
            t = classify(r.get("primer", ""))
            key = (name, r["caliber"], r["bullet"], r["bullet_gr"], t)
            if t and key not in seen:
                seen.add(key)
                out[r["caliber"]][t] += 1
    vv = samples.parent / "vv_cache" / "vv_data.json"
    if vv.exists():
        for c in json.load(open(vv, encoding="utf-8"))["CartridgeData"]:
            cal = next((calibers[n] for n in map(norm, [c["Cartridge"], *c["Cartridge"].split("/")]) if n in calibers), None)
            t = classify(c.get("Primers", ""))
            if cal and t: out[cal["cipname"]][t] += 3
    return out


def main() -> int:
    root = Path(__file__).resolve().parents[3]
    ap = argparse.ArgumentParser()
    ap.add_argument("--write", action="store_true")
    ap.add_argument("--data", type=Path, default=root / "data")
    ap.add_argument("--samples", type=Path, default=root / "scripts" / "data" / "loaddata" / "sources" / "samples")
    args = ap.parse_args()
    files = sorted((args.data / "calibers").glob("*.json"))
    records = {f: json.loads(f.read_text(encoding="utf-8")) for f in files}
    by_name = {}
    for r in records.values():
        for n in [r["cipname"], *r.get("altname", "").split(",")]: by_name.setdefault(norm(n), r)
    v = votes(args.samples, by_name)
    stats = collections.Counter()
    for f, record in records.items():
        fallback = rule(record)
        counts = v.get(record["cipname"])
        if record["cipname"] in CORRECTIONS:
            primer, source = CORRECTIONS[record["cipname"]], "fixed"
        elif counts:
            top = max(counts.values())
            best = [t for t, n in counts.items() if n == top]
            primer, source = (fallback if fallback in best else sorted(best)[0]), "tables"
        else:
            primer, source = fallback, "rule"
        stats[source if primer else "none"] += 1
        print(f"{record['cipname']:32s} {primer or '-':4s} {source:6s} {dict(counts) if counts else ''}")
        if args.write and primer and record.get("primer") != primer:
            out = {}
            for k, val in record.items():  # after the case type, in the records' format (indent 2)
                if k != "primer": out[k] = val
                if k == "ciptype": out["primer"] = primer
            out.setdefault("primer", primer)
            f.write_text(json.dumps(out, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(dict(stats), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
