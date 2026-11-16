#!/usr/bin/env python3
"""Combines every published-load file into one load file per class (rifle, pistol, shotshell), checks the loads and drops the invalid ones.

    build_loads.py [--samples sources/samples] [--out sources/loads] [--simulate <csr_eval>] [--max-dev 45] [--trend 0.15] [--data data] [--measured data/measurements]

1. combine   every `*.csv` of --samples (the files of the extractors: `hodgdon_samples.csv`, `lyman_samples.csv`, `vv_N150.csv`, ...) is read; a load that several
             files hold (same powder, cartridge, bullet weight, kind, charge, velocity, cartridge length, shot mass; shotshell: gauge, shot, primer, wad, pressure) is kept once, with the first file as `source`.
             Also read are the own measured series of --measured (`data/measurements/*.csv` with a `powder` and a `kind` column: DEVA and Beschussamt protocols, the
             column `weight` = number of shots is kept, fit_physical_powders.py counts a row that many times); a source stem `deva_loads` etc. They go through the same steps.
             The class comes from the cartridge record (C.I.P. table IV = pistol and revolver, otherwise rifle); loads of the shotshell files (`*shot*`) are shotshell
             loads. Loads of a cartridge the project does not have are dropped (nothing to simulate them with).
2. check     analysis/check_published_loads.py with --apply-hard on the rifle and the pistol file: range (velocity, charge, bullet mass, lengths), energy (muzzle energy
             per powder energy), pressure (above the permitted maximum), duplicate loads with conflicting velocities, a charge read as grains instead of grams is
             converted. The softer flags (start above maximum load, off the table's trend, bullet density) stay in the data, as the refit procedure decided.
             Shotshell loads get a range check here (velocity, charge, shot mass, pressure).
   Copied loads (near_duplicates: the same load in another table or source within rounding) are kept once, so a guide that copies another one
             does not count its loads twice.
   Every rifle and pistol load gets the start pressure gpressure and extra friction gfriction of its bullet as the program uses them (the matching record's, else the construction
             class of start_pressure.py), so the fits and comparisons simulate what the program does; measured rows that have one keep it.
3. simulate  (with --simulate) analysis/filter_published_outliers.py: loads whose simulated peak pressure is more than --max-dev percent off the published one.
Output in --out: loads_rifle.csv, loads_pistol.csv, loads_shot.csv (the clean loads, columns of the sources that have them), removed.csv (every dropped load with
the step and the reason) and report.txt (counts per class, source and reason). The sample files are not changed; run this script again after a source is added or
changed. analysis/refit.sh runs it and then the refit.
"""
import argparse
import collections
import csv
import json
import math
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE))
from by_maker import ALIASES  # noqa: E402
from samples_common import Records, start_class  # noqa: E402

KEY = ("powder", "caliber", "bullet_gr", "kind", "charge_g", "velocity_ms", "col_mm", "shot_mass_g")
SHOT_KEY = ("gauge", "shot_g", "primer", "wad", "pressure_bar")  # shotshell loads of one charge and velocity differ in these (the files have no caliber or bullet columns)
SHOT_RANGE = {"velocity_ms": (100, 600), "charge_g": (0.3, 6.0), "shot_g": (5, 100), "pressure_bar": (100, 1500)}


def num(v: str | None) -> float | None:
    try:
        x = float(v or "")
        return x if math.isfinite(x) else None
    except ValueError:
        return None


def write(path: Path, rows: list[dict], always: tuple = ("powder", "kind", "source")) -> None:
    """The columns that have a value (csr_eval reads shotgun columns of a rifle file as a shotshell load)."""
    cols = list(dict.fromkeys(k for r in rows for k in r if not k.startswith("_")))
    used = [c for c in cols if c in always or any(r.get(c) not in (None, "") for r in rows)]
    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, used, extrasaction="ignore")
        w.writeheader()
        w.writerows(rows)


def split_kept(rows: list[dict], kept: list[dict]) -> tuple[list[dict], int]:
    """(rows that a later step dropped, number of kept rows that it changed): `kept` is a subsequence of `rows`, a repaired charge changes a kept row."""
    ident = lambda r: tuple(r.get(k) or "" for k in KEY if k != "charge_g")  # noqa: E731
    dropped, changed, j = [], 0, 0
    for r in rows:
        if j < len(kept) and ident(kept[j]) == ident(r):
            changed += (kept[j].get("charge_g") or "") != (r.get("charge_g") or "")
            j += 1
        else:
            dropped.append(r)
    return dropped, changed


def near_duplicates(rows: list[dict], measured: set[str]) -> tuple[list[dict], list[dict]]:
    """Loads that are copies of each other: same powder, cartridge, bullet weight and kind, charge within 0.3 %, velocity within 0.1 % and, when
    both have one, pressure within 0.5 % (the bullet name, cartridge length and barrel may differ: manuals list one test under several bullets of a
    weight, and guides copy each other's tables, converting units). Independent tests of the same charge differ by about 2 % in velocity and 6 % in
    pressure; among the published loads the velocity differences of same-charge pairs peak sharply below 0.05 % (2,436 rifle pairs, 2026-11-16) and
    spread evenly above. Each group keeps one load: an own measurement first, then one with a pressure, then the first file's.
    Returns (kept rows in their order, removed rows)."""
    def rank(r):
        return (0 if r["source"] in measured else 1, 0 if r.get("pressure_bar") else 1)
    groups: dict[tuple, list[int]] = collections.defaultdict(list)
    for i, r in enumerate(rows):
        try:
            groups[(r["powder"].upper(), r["caliber"], round(float(r["bullet_gr"]), 1), r["kind"])].append(i)
        except (KeyError, ValueError):
            pass
    drop: dict[int, str] = {}
    for idx in groups.values():
        kept: list[int] = []
        for i in sorted(idx, key=lambda i: (rank(rows[i]), i)):
            r = rows[i]
            try:
                c, v = float(r["charge_g"]), float(r["velocity_ms"])
            except ValueError:
                kept.append(i)
                continue
            p = float(r["pressure_bar"]) if r.get("pressure_bar") else None
            twin = None
            for k in kept:
                o = rows[k]
                try:
                    if abs(float(o["charge_g"]) / c - 1) > 0.003 or abs(float(o["velocity_ms"]) / v - 1) > 0.001:
                        continue
                except ValueError:
                    continue
                if p and o.get("pressure_bar") and abs(float(o["pressure_bar"]) / p - 1) > 0.005:
                    continue
                twin = o
                break
            if twin is None:
                kept.append(i)
            else:
                drop[i] = f"copy of a load of {twin['source']}"
    return ([r for i, r in enumerate(rows) if i not in drop],
            [{**rows[i], "step": "combine", "reason": "near duplicate", "detail": why} for i, why in drop.items()])


def start_values(records: Records, row: dict, cls: str) -> tuple[str, str]:
    """The bullet start pressure (gpressure, bar) and extra friction (gfriction, bar) the program uses for this load: the matching record's
    (samples_common.Records.match), else the bullet's construction class (start_pressure.py: lead 150, jacketed 250, brass 290, copper 350) and no
    friction. Until 2026-11-16 the fits ran every load with the solver's default 250 bar while the program took the records' values."""
    maker, btype = records.split(row.get("bullet", ""))
    try:
        rec = records.match(maker, btype, float(row["bullet_gr"]), float(row["bullet_dia_mm"])) if maker else None
    except (KeyError, ValueError):
        rec = None
    if rec and rec.get("gpressure"):
        return f"{rec['gpressure']:g}", f"{rec['gfriction']:g}" if rec.get("gfriction") else ""
    return f"{start_class(maker or '', btype, handgun=cls == 'pistol')[0]:g}", ""


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--samples", type=Path, default=HERE / "sources" / "samples")
    ap.add_argument("--out", type=Path, default=HERE / "sources" / "loads")
    ap.add_argument("--simulate", type=Path, help="csr_eval: also drop loads whose simulated pressure is far off the published one")
    ap.add_argument("--max-dev", type=float, default=45.0)
    ap.add_argument("--trend", type=float, default=0.15)
    ap.add_argument("--data", type=Path, default=ROOT / "data")
    ap.add_argument("--measured", type=Path, default=ROOT / "data" / "measurements", help="directory of the own measured series (empty or missing: none)")
    ap.add_argument("--python", default=sys.executable)
    args = ap.parse_args()
    records = Records(args.data)

    classes = {}
    for rec in {id(c): c for c in records.calibers.values()}.values():
        classes[rec["cipname"]] = "pistol" if str(rec.get("ciptype", "")).startswith("4") else "rifle"
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="build_loads_"))
    removed: list[dict] = []
    seen: set[tuple] = set()
    by_class: dict[str, list[dict]] = {"rifle": [], "pistol": [], "shot": []}
    files = sorted(p for p in args.samples.glob("*.csv") if "lengths" not in p.name)
    if args.measured.is_dir():
        files = sorted(args.measured.glob("*.csv")) + files  # first: a measured load wins over a published duplicate
    n_in = 0
    for path in files:
        with path.open(newline="", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            if not reader.fieldnames or "powder" not in reader.fieldnames or (path.parent == args.measured and "kind" not in reader.fieldnames):
                continue  # (data/measurements/own_chrono_loads.csv is a velocity-only file in another format)
            for row in reader:
                n_in += 1
                row["powder"] = ALIASES.get(row["powder"], row["powder"])
                row["source"] = path.stem
                key = tuple((row.get(k) or "").upper() if k == "powder" else row.get(k) or "" for k in KEY + (SHOT_KEY if "shot" in path.stem else ()))
                if key in seen:
                    continue
                seen.add(key)
                if "shot" in path.stem:
                    by_class["shot"].append(row)
                    continue
                cls = classes.get(row.get("caliber", ""))
                if cls is None:
                    removed.append({**row, "step": "combine", "reason": "cartridge not in the project data"})
                    continue
                if not row.get("gpressure"):
                    row["gpressure"], row["gfriction"] = start_values(records, row, cls)
                by_class[cls].append(row)
    print(f"combine: {n_in} loads of {len(files)} files -> " + ", ".join(f"{len(v)} {k}" for k, v in by_class.items()) + f" ({len(seen)} different, {len(removed)} unknown cartridge)")
    measured = {p.stem for p in files if p.parent == args.measured}
    for cls in ("rifle", "pistol"):
        by_class[cls], copies = near_duplicates(by_class[cls], measured)
        removed += copies
        print(f"combine: {cls}: {len(copies)} copied loads removed (same load in another table or source, within rounding)")

    # check (rifle, pistol): analysis/check_published_loads.py
    combined = []
    for cls in ("rifle", "pistol"):
        if by_class[cls]:
            write(work / f"loads_{cls}.csv", by_class[cls])
            combined.append(str(work / f"loads_{cls}.csv"))
    checked = work / "checked"
    res = subprocess.run([args.python, str(ROOT / "analysis" / "check_published_loads.py"), *combined, "--out", str(checked), "--trend", str(args.trend), "--apply-hard"],
                         capture_output=True, text=True)
    if res.returncode:
        print(res.stdout, res.stderr, file=sys.stderr)
        return 1
    print("check:", res.stdout.strip().replace("\n", "\n       "))
    reasons: dict[tuple, str] = {}
    with (checked / "flagged.csv").open(newline="", encoding="utf-8") as f:
        for r in csv.DictReader(f):
            chk = r["checks"]
            if any(c in chk for c in ("range", "energy", "pressure", "duplicate")):
                reasons[tuple(r.get(k) or "" for k in KEY)] = f"{chk}: {r['detail']}"
    stages, repaired = {}, {}
    for cls in ("rifle", "pistol"):
        if not by_class[cls]:
            continue
        with (checked / f"loads_{cls}.csv").open(newline="", encoding="utf-8") as f:
            kept = list(csv.DictReader(f))
        dropped, changed = split_kept(by_class[cls], kept)
        for r in dropped:
            removed.append({**r, "step": "check", "reason": reasons.get(tuple(r.get(c) or "" for c in KEY), "hard flag")})
        repaired[cls] = changed
        stages[cls] = kept
    # shotshell: ranges
    kept_shot = []
    for r in by_class["shot"]:
        bad = [f"{k} {r.get(k)}" for k, (lo, hi) in SHOT_RANGE.items() if num(r.get(k)) is not None and not lo <= num(r.get(k)) <= hi]
        if bad or num(r.get("velocity_ms")) is None or num(r.get("charge_g")) is None:
            removed.append({**r, "step": "check", "reason": "range: " + (", ".join(bad) or "velocity or charge missing")})
        else:
            kept_shot.append(r)
    stages["shot"] = kept_shot

    # simulate (rifle, pistol): analysis/filter_published_outliers.py
    if args.simulate:
        sim = work / "sim"
        files_in = []
        for cls in ("rifle", "pistol"):
            if stages.get(cls):
                write(work / f"s_{cls}.csv", stages[cls])
                files_in.append(str(work / f"s_{cls}.csv"))
        res = subprocess.run([args.python, str(ROOT / "analysis" / "filter_published_outliers.py"), str(args.simulate), *files_in, "--max-dev", str(args.max_dev),
                              "--out", str(sim)], capture_output=True, text=True)
        if res.returncode:
            print(res.stdout, res.stderr, file=sys.stderr)
            return 1
        print("simulate:", res.stdout.strip().splitlines()[-1] if res.stdout.strip() else "")
        for cls in ("rifle", "pistol"):
            if not stages.get(cls):
                continue
            with (sim / f"s_{cls}.csv").open(newline="", encoding="utf-8") as f:
                kept = list(csv.DictReader(f))
            dropped, _ = split_kept(stages[cls], kept)
            removed.extend({**r, "step": "simulate", "reason": f"simulated pressure more than {args.max_dev:g} % off the published one"} for r in dropped)
            stages[cls] = kept

    lines = []
    for cls, rows in stages.items():
        write(args.out / f"loads_{cls}.csv", rows)
        lines.append(f"loads_{cls}.csv: {len(rows)} loads, {len({r['powder'] for r in rows})} powders, {len({r['source'] for r in rows})} sources, "
                     f"{sum(1 for r in rows if num(r.get('pressure_bar')))} with a pressure")
    if removed:
        write(args.out / "removed.csv", removed, always=("powder", "kind", "source", "step", "reason"))
    lines.append(f"removed.csv: {len(removed)} loads; charges repaired (grains read as grams): " + ", ".join(f"{k} {v}" for k, v in repaired.items()))
    props = [p for f in (args.data / "propellants").glob("*.json") for p in json.loads(f.read_text(encoding="utf-8"))]
    names = collections.Counter(p["pname"].upper() for p in props)
    known = {f"{p['mname']} {p['pname']}".upper() for p in props} | {n for n, c in names.items() if c == 1}  # a bare name counts when it is unique (N150)
    other = collections.Counter(r["powder"] for rows in stages.values() for r in rows if r["powder"].upper() not in known)
    lines.append(f"  loads of powders without a project record (kept, no refit possible until one exists): {sum(other.values())} of {len(other)} powders; most: "
                 + ", ".join(f"{k} {v}" for k, v in other.most_common(8)))
    for step in ("combine", "check", "simulate"):
        c = collections.Counter(re.split(r"[:(]", r["reason"])[0].strip() for r in removed if r["step"] == step)
        if c:
            lines.append(f"  {step}: " + ", ".join(f"{k} {v}" for k, v in c.most_common()))
    per_source = collections.Counter(r["source"] for r in removed)
    lines.append("  removed per source: " + ", ".join(f"{k} {v}" for k, v in per_source.most_common(12)))
    report = "\n".join(lines)
    (args.out / "report.txt").write_text(report + "\n", encoding="utf-8")
    print(report)
    shutil.rmtree(work, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
