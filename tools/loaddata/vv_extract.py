#!/usr/bin/env python3
"""Turns the Vihtavuori load database into fit samples (CSV) for one powder.

    vv_extract.py N150 [-o vv_N150.csv] [--json file] [--data data] [--cache dir]

Vihtavuori publishes its whole reloading database as one JSON file (the data of the reloading data tool on vihtavuori.com):
cartridges with test barrel, bullets and, per powder, the start and maximum charge with their velocities.
For every load line the script finds the caliber record of the project (data/calibers) and the bullet (data/projectiles, else
a length estimated from mass, diameter and type) and writes two samples (start load, maximum load) with the data that
tools/grt_fitdata needs. Cartridges and bullets that cannot be matched are listed on stderr and skipped.
"""
import argparse
import csv
import json
import re
import sys
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "bulletdata"))
from estimate_length import length_in  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import recordfiles  # noqa: E402

PAGE = "https://vihtavuori.com/reloading-data/reloading-data-tool/"
GRAIN_G = 0.06479891
IN = 25.4


def num(s) -> float:
    try:
        return float(str(s).replace(",", "."))
    except ValueError:
        return 0.0


def norm(name: str) -> str:
    s = name.lower().replace(",", ".").replace("×", "x").replace("mm", "")
    for a, b in [("winchester", "win"), ("remington", "rem"), ("magnum", "mag"), ("weatherby", "weath"), ("creedmoor", "creed"),
                 ("springfield", "spring"), ("short", "short"), ("auto", "auto"), ("parabellum", "luger")]:
        s = s.replace(a, b)
    return re.sub(r"[^a-z0-9]", "", s).lstrip("0")


def load_json(path: Path | None, cache: Path | None) -> dict:
    if path:
        return json.loads(path.read_text(encoding="utf-8"))
    cached = cache / "vv_data.json" if cache else None
    if cached and cached.exists():
        return json.loads(cached.read_text(encoding="utf-8"))
    req = urllib.request.Request(PAGE, headers={"User-Agent": "Mozilla/5.0 (GRTReloaded data collection)"})
    url = re.search(r'dataUrl":"([^"]+)"', urllib.request.urlopen(req, timeout=60).read().decode()) [1].replace("\\/", "/")
    text = urllib.request.urlopen(urllib.request.Request(url, headers=req.headers), timeout=120).read().decode("utf-8")
    if cached:
        cached.parent.mkdir(parents=True, exist_ok=True)
        cached.write_text(text, encoding="utf-8")
    return json.loads(text)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("powder")
    ap.add_argument("-o", "--output", type=Path)
    ap.add_argument("--json", type=Path)
    ap.add_argument("--cache", type=Path)
    ap.add_argument("--data", type=Path, default=Path(__file__).resolve().parents[2] / "data")
    args = ap.parse_args()
    db = load_json(args.json, args.cache)

    calibers = {}
    for f in (args.data / "calibers").glob("*.json"):
        rec = json.loads(f.read_text(encoding="utf-8"))
        for n in [rec["cipname"], *rec.get("altname", "").split(",")]:
            calibers.setdefault(norm(n), rec)
    projectiles = [r for _, r in recordfiles.read(args.data / "projectiles")]
    bullets = {b["BulletId"]: b for b in db["Bulletdata"]}
    carts = {c["CartridgeID"]: c for c in db["CartridgeData"]}

    rows, miss_cart, miss_bullet = [], {}, {}
    for r in db["Relodata"]:
        if r["PowderType"] != args.powder:
            continue
        cart = carts[r["CartridgeID"]]
        cal = calibers.get(norm(cart["Cartridge"]))
        if not cal:
            miss_cart[cart["Cartridge"]] = miss_cart.get(cart["Cartridge"], 0) + 1
            continue
        b = bullets.get(r["BulletID"], {})
        grains = num(r["BulletWeightGrains"])
        dia = cal.get("G1") or 0
        if not dia or not grains or not num(r["ColMM"]):
            continue
        # the bullet: a record of the project with the same maker, weight and diameter, else an estimated length
        match = [p for p in projectiles if p["mname"].lower() == b.get("Bulletmfg", "").lower() and abs(p["gmass"] - grains) < 0.6
                 and abs(p["gdia"] - dia) < 0.06]
        name = b.get("Bulletname", "").lower()
        if match:
            best = max(match, key=lambda p: sum(w in p["pname"].lower() for w in re.findall(r"[a-z0-9]+", name)))
            length, source, tail = best["glen"], "database", best.get("gtailtype", 0)
        else:
            est, kind = length_in(grains, dia / IN, f"{name} {b.get('Bulletmfg', '')}")
            length, source, tail = est * IN, "estimate:" + kind, 2 if re.search(r"bt|boat|a-?max|match|scenar|hpbt|otm|vld", name) else 0
            miss_bullet[(b.get("Bulletmfg"), b.get("Bulletname"))] = miss_bullet.get((b.get("Bulletmfg"), b.get("Bulletname")), 0) + 1
        barrel = re.search(r"([\d,.]+)\s*mm", cart.get("Barrel", ""))
        twist = re.search(r"1 in ([\d.,]+)", cart.get("Barrel", ""))
        barrel_mm = num(r["BarrelLength"]) or (num(barrel[1]) if barrel else 0)
        for kind in ("start", "max"):
            g, ms = num(r["StartloadGrams" if kind == "start" else "MaxloadGrams"]), num(r["StartloadMs" if kind == "start" else "MaxloadMs"])
            if g > 0 and ms > 0:
                rows.append({"powder": args.powder, "caliber": cal["cipname"], "pmax_bar": cal.get("Pmax", ""), "bullet": f"{b.get('Bulletmfg', '')} {b.get('Bulletname', '')}".strip(),
                             "bullet_gr": grains, "bullet_dia_mm": dia, "bullet_len_mm": f"{length:.2f}", "len_source": source, "tail_type": tail,
                             "col_mm": num(r["ColMM"]), "barrel_mm": barrel_mm, "twist_mm": f"{num(twist[1]) * IN:.0f}" if twist else "",
                             "kind": kind, "charge_g": g, "velocity_ms": ms})
    out = args.output or Path(f"vv_{args.powder}.csv")
    with open(out, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)
    print(f"{len(rows)} samples of {args.powder} -> {out}", file=sys.stderr)
    print("cartridges not found:", ", ".join(f"{k} ({v})" for k, v in sorted(miss_cart.items())), file=sys.stderr)
    print(f"bullets with estimated length: {sum(miss_bullet.values())} load lines", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
