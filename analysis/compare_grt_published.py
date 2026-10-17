#!/usr/bin/env python3
"""Compares the model of the original program with the pressures manufacturers publish, load by load, with the ORIGINAL's own records.

    compare_grt_published.py <csr_dbeval> [--db tools/GordonsReloadingTool.plain.db] [--out DIR] [--lots best|all] [--profile original|published]
                             [--set name=value ...] [--top 6] [samples.csv ...]

Published loads with a pressure (scripts/data/loaddata/sources/samples: Hodgdon manual, Lovex, Reload Swiss, ADI) are matched to records of the
original's database: the same caliber, the same projectile (maker, type, mass, diameter) and the same powder (maker and name). Only loads whose
three components exist there are simulated, with csr_dbeval (profile "original" = the GRT Original model of this program). The project's own
records and their fitted values are not involved, so this measures the original's model and its records against the manufacturers' data.

Powders with several lots in the original's database: --lots best takes one lot (highest Qlty, released lots before development lots, lowest id),
--lots all simulates every lot (the CSV has one row per lot, the lot's id and name are columns).
Published pressures outside 500..9000 bar are dropped (misread columns). Rifle and pistol (C.I.P. table IV) are reported separately.
Writes DIR/grt_vs_published.csv (all simulated loads) and prints the statistics and the loads that deviate most in both directions.
The CSV is derived from the original's database: keep it local (reference/ is git-ignored).
"""
import argparse, collections, csv, os, re, sqlite3, subprocess, sys, tempfile
import numpy as np

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
SAMPLES = os.path.join(ROOT, "scripts", "data", "loaddata", "sources", "samples")
DEFAULT_FILES = [("lovex2020_samples.csv", "lovex", "Lovex"), ("lovex_samples.csv", "lovex", "Lovex"), ("rs_samples.csv", "rs", "Reload Swiss"),
                 ("hod_samples.csv", "hod", "Hodgdon manual"), ("adi_rifle_samples.csv", "adi", "ADI"), ("adi_pistol_samples.csv", "adi", "ADI")]
KIND = {"lovex2020_samples.csv": "lovex", "lovex_samples.csv": "lovex", "rs_samples.csv": "rs", "adi_rifle_samples.csv": "adi", "adi_pistol_samples.csv": "adi"}
PUBLISHER = {"lovex": "Lovex", "rs": "Reload Swiss", "adi": "ADI", "hod": "Hodgdon manual"}

ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
ap.add_argument("dbeval"); ap.add_argument("samples", nargs="*")
ap.add_argument("--db", default=os.path.join(ROOT, "tools", "GordonsReloadingTool.plain.db"))
ap.add_argument("--out", default=os.path.join(ROOT, "reference", "grt_vs_published"))
ap.add_argument("--lots", choices=["best", "all"], default="best")
ap.add_argument("--profile", choices=["original", "published"], default="original")
ap.add_argument("--set", action="append", default=[], help="calibration constant name=value (repeatable)")
ap.add_argument("--top", type=int, default=6)
args = ap.parse_args()

db = sqlite3.connect(args.db); db.row_factory = sqlite3.Row
norm = lambda s: re.sub(r"[^a-z0-9.]", "", s.lower())
toks = lambda s: [t for t in re.split(r"[^a-z0-9.]+", s.lower()) if t]

props = [dict(r) for r in db.execute("select * from propellant where geloescht=0")]
projs = collections.defaultdict(list)
for r in db.execute("select * from projectile where geloescht=0"): projs[r["mname"].lower()].append(dict(r))
makers = sorted(projs, key=len, reverse=True)
cals = {}
for r in db.execute("select * from caliber where geloescht=0"): cals.setdefault(norm(r["cipname"]).lstrip("."), dict(r))
handgun = {r["cipname"] for r in db.execute("select cipname, cippdf from caliber") if r["cippdf"] and "/tab-iv/" in r["cippdf"]}


def find_caliber(name): return cals.get(norm(name).lstrip("."))


def find_bullet(name, gr, dia):
    t = name.split(); mk = None
    for m in makers:
        mt = m.split()
        if [x.lower() for x in t[:len(mt)]] == mt: mk = m; t = t[len(mt):]; break
    if mk is None: return []
    if t and (t[0].isupper() and len(t[0]) == 3): t = t[1:]  # maker code of the manuals: HDY, SIE, NOS, ...
    need = set(toks(" ".join(t)))
    out = [b for b in projs[mk] if abs(b["gmass"] - gr) <= 0.6 and abs(b["gdia"] - dia) <= 0.04 and need and need <= set(toks(b["pname"]))]
    return sorted(out, key=lambda b: (abs(b["gmass"] - gr), abs(b["gdia"] - dia), b["id"]))


def find_powders(kind, name):
    if kind == "lovex": maker, pn = "lovex", name
    elif kind == "rs": maker, pn = "reloadswiss", name.replace("RS", "RS ")
    elif kind == "adi": maker, pn = "adi", name[4:].replace("Bench Mark ", "BM ").replace("AR", "AR ", 1) if name.startswith("ADI AR") else name[4:].replace("Bench Mark ", "BM ")
    else: maker, pn = (name.split(" ", 1) + [""])[:2]
    out = []
    for x in props:
        if norm(x["mname"]) != norm(maker): continue
        n = re.sub(r"-0\d$", "", x["pname"]) if kind == "lovex" else x["pname"]
        if norm(n) == norm(pn): out.append(x)
    return out


def best_lot(p):  # highest quality, released before development lots, then the lowest id
    return sorted(p, key=lambda x: (-(x["Qlty"] or 0), bool(re.search(r"DEV|Work Up", x["lotid"] or "")), x["id"]))[0]


files = [(f, KIND.get(os.path.basename(f), "hod"), PUBLISHER[KIND.get(os.path.basename(f), "hod")]) for f in args.samples] or \
        [(os.path.join(SAMPLES, f), k, p) for f, k, p in DEFAULT_FILES]
jobs, seen = [], set()
for path, kind, publisher in files:
    for r in csv.DictReader(open(path)):
        if not r["pressure_bar"] or not r["barrel_mm"] or not 500 <= float(r["pressure_bar"]) <= 9000: continue
        cal = find_caliber(r["caliber"]); bl = find_bullet(r["bullet"], float(r["bullet_gr"]), float(r["bullet_dia_mm"])); pw = find_powders(kind, r["powder"])
        if not (cal and bl and pw): continue
        key = (kind, r["powder"], r["caliber"], r["bullet"], r["bullet_gr"], r["kind"], r["charge_g"], r["col_mm"], r["barrel_mm"])
        if key in seen: continue
        seen.add(key)
        for lot in (pw if args.lots == "all" else [best_lot(pw)]):
            jobs.append(dict(r, publisher=publisher, cal=cal, bullet=bl[0], powder=lot, lots=len(pw)))

tmp = tempfile.mkdtemp(); jobfile = os.path.join(tmp, "jobs.csv")
with open(jobfile, "w") as o:
    for i, j in enumerate(jobs): o.write(f"{i},{j['cal']['id']},{j['bullet']['id']},{j['powder']['id']},{j['charge_g']},{j['barrel_mm']},{j['col_mm']},{j['twist_mm']}\n")
cmd = [args.dbeval, "--db", args.db, "--profile", args.profile] + [x for s in args.set for x in ("--set", s)] + [jobfile]
res = subprocess.run(cmd, capture_output=True, text=True)
if res.returncode: sys.exit(f"csr_dbeval failed ({res.returncode}): {res.stderr}")
rows = []
for line in res.stdout.splitlines():
    i, status, v, p = line.split(",")
    if status != "ok": continue
    j = jobs[int(i)]; pub_p, pub_v = float(j["pressure_bar"]), float(j["velocity_ms"])
    rows.append(dict(publisher=j["publisher"], cls="pistol" if j["cal"]["cipname"] in handgun else "rifle", caliber=j["cal"]["cipname"],
                     bullet=f"{j['bullet']['mname']} {j['bullet']['pname']}", bullet_gr=j["bullet_gr"], powder=f"{j['powder']['mname']} {j['powder']['pname']}",
                     powder_id=j["powder"]["id"], lot=j["powder"]["lotid"], lots=j["lots"], kind=j["kind"], charge_g=j["charge_g"], barrel_mm=j["barrel_mm"],
                     col_mm=j["col_mm"], cip_limit_bar=j["pmax_bar"], pub_velocity_ms=pub_v, sim_velocity_ms=float(v), velocity_dev_pct=round(100 * (float(v) / pub_v - 1), 2),
                     pub_pressure_bar=pub_p, sim_pressure_bar=float(p), pressure_dev_pct=round(100 * (float(p) / pub_p - 1), 2)))
os.makedirs(args.out, exist_ok=True)
outfile = os.path.join(args.out, "grt_vs_published.csv")
rows.sort(key=lambda r: r["pressure_dev_pct"])
with open(outfile, "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(rows[0])); w.writeheader(); w.writerows(rows)
print(f"{len(jobs)} loads with all components in the original's database ({args.lots} lot), {len(rows)} simulated -> {outfile}\n")

print(f"{'class':6} {'publisher':15} {'n':>5}  pressure dev. mean / rms / median|dev|   velocity dev. mean / rms")
for cls in ("rifle", "pistol"):
    for pub in sorted({r["publisher"] for r in rows}):
        sel = [r for r in rows if r["cls"] == cls and r["publisher"] == pub]
        if not sel: continue
        dp = np.array([r["pressure_dev_pct"] for r in sel]); dv = np.array([r["velocity_dev_pct"] for r in sel])
        print(f"{cls:6} {pub:15} {len(sel):5}  {dp.mean():+6.1f} % / {np.sqrt((dp**2).mean()):5.1f} % / {np.median(abs(dp)):5.1f} %        {dv.mean():+5.1f} % / {np.sqrt((dv**2).mean()):4.1f} %")


def show(title, sel):
    print(f"\n{title}")
    for r in sel:
        print(f"{r['pressure_dev_pct']:+7.1f} %  p {r['sim_pressure_bar']:5.0f} vs {r['pub_pressure_bar']:5.0f} bar | v {r['sim_velocity_ms']:4.0f} vs {r['pub_velocity_ms']:4.0f} m/s ({r['velocity_dev_pct']:+.1f} %)"
              f" | {r['cls']} {r['publisher']} | {r['caliber']} | {r['bullet']} {r['bullet_gr']} gr | {r['powder']} (id {r['powder_id']}, {r['lot']}) | {r['kind']} {float(r['charge_g']):.3f} g, barrel {r['barrel_mm']}")


show(f"simulated too LOW ({args.top} largest)", rows[:args.top])
show(f"simulated too HIGH ({args.top} largest)", rows[::-1][:args.top])
