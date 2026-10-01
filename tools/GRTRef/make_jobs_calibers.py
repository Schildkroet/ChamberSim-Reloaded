#!/usr/bin/env python3
"""Caliber sweep: loads for many cartridges built from the database (caliber + matching bullet + powder).
Output: jobs/exp_cal_<n>.grtload and calibers.json (what each job contains)."""
import re, os, sys, json, sqlite3, random, urllib.parse
HERE = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.expanduser("~/Downloads/GordonsReloadingTool-2021.2040-NIGHTLY-linux/loads/8x57is.grtload")
DB = os.path.join(HERE, "..", "GordonsReloadingTool.plain.db"); OUT = os.path.join(HERE, "jobs")
N = int(sys.argv[1]) if len(sys.argv) > 1 else 36
GRAIN = 0.06479891

def setv(s, section, name, value):
    m = re.search(rf"<{section}>.*?</{section}>", s, re.S); blk = m.group(0)
    new, n = re.subn(rf'(<input name="{name}" value=")[^"]*(")', lambda mm: mm.group(1) + str(value) + mm.group(2), blk)
    assert n == 1, (section, name, n)
    return s[:m.start()] + new + s[m.end():]
def setvar(s, name, value):
    new, n = re.subn(rf'(<var name="{name}" value=")[^"]*(")', lambda mm: mm.group(1) + str(value) + mm.group(2), s)
    return new

c = sqlite3.connect(f"file:{DB}?mode=ro", uri=True); c.row_factory = sqlite3.Row
cal = [dict(r) for r in c.execute("select * from caliber where geloescht=0 and c_Q>1 and V>0 and L3>0 and L6>0 and Pmax>0 and c_Z>0")]
for r in cal: r["vol"] = r["V"] * GRAIN
cal.sort(key=lambda r: r["vol"])
random.seed(11)
step = len(cal) / N
chosen = [cal[min(int(i * step + random.random() * step), len(cal) - 1)] for i in range(N)]
proj = [dict(r) for r in c.execute("select * from projectile where geloescht=0 and gdia>0 and glen>0 and gmass>0")]
pw = {k: dict(v) for k, v in json.load(open(os.path.join(HERE, "powders.json"))).items()}
fast, medium, slow = ["595", "587", "590", "198"], ["540", "606", "443", "675"], ["562", "599", "400", "73", "31"]
base = open(BASE, encoding="utf-8").read()
os.makedirs(OUT, exist_ok=True); meta = {}
for i, r in enumerate(chosen):
    Z = r["c_Z"]
    cand = [p for p in proj if abs(p["gdia"] - Z) < 0.25]
    if not cand: continue
    cand.sort(key=lambda p: p["gmass"]); p = cand[len(cand) // 2]
    vol = r["vol"]; typ = (r["ciptype"] or "").lower()
    pid = random.choice(fast if vol < 1.3 else medium if vol < 3.0 else slow)
    w = pw[pid]
    barrel = 150 if "pistol" in typ or "revolver" in typ else 400 if "rimfire" in typ else 600
    mp = p["gmass"] * GRAIN
    depth = min(max(r["L3"] + p["glen"] - r["L6"], 2.0), 0.8 * p["glen"])
    oal = r["L3"] + p["glen"] - depth
    mc = 0.72 * w["pcd"] * 1e-3 * vol            # g: ~72 % of the bulk-density capacity
    s = base
    for key, val in (("CaliberName", urllib.parse.quote(r["cipname"])), ("oal", round(oal, 3)), ("caselen", r["L3"]), ("casevol", round(vol, 6)),
                     ("Dz", Z), ("Aeff", r["c_Q"]), ("pMaxZul", r["Pmax"]), ("sebert", r["sebert"]), ("ps", p["gpressure"] or 250)):
        s = setv(s, "caliber", key, val)
    s = setv(s, "gun", "xe", barrel)
    for key, val in (("ProjectileName", urllib.parse.quote(f'{p["mname"]}, {p["pname"]}')), ("mname", urllib.parse.quote(p["mname"])), ("pname", urllib.parse.quote(p["pname"])),
                     ("Dbul", p["gdia"]), ("mp", repr(mp)), ("gdepth", round(depth, 3)), ("glen", p["glen"]), ("gpressure", p["gpressure"] or 250),
                     ("gtailh", p["gtailh"]), ("gTailDiaA", p["gtaildiaA"]), ("gTailDiaB", p["gtaildiaB"]), ("gTailType", p["gtailtype"]), ("g1bc", p["g1bc"]), ("g7bc", p["g7bc"])):
        s = setv(s, "projectile", key, val)
    for key, val in (("mname", urllib.parse.quote(w["mname"])), ("pname", urllib.parse.quote(w["pname"])), ("Ba", w["Ba"]), ("Qex", w["Qex"]), ("k", w["k"]), ("a0", w["a0"]),
                     ("a1", w["a1"]), ("z1", w["z1"]), ("z2", w["z2"]), ("eta", w["eta"]), ("pc", w["pc"]), ("pcd", w["pcd"]), ("mc", repr(mc))):
        s = setv(s, "propellant", key, val)
    for key in ("L0","L1","L2","L3","L4","L5","L6","R","R1","R3","E","E1","Emin","FG","P0","P1","P2","S","r1min","r2","H1","H2","G1","G2","f","L3G","Pmax","PK","PE","EE","V","sebert"):
        if key in r: s = setvar(s, key, r[key])
    s = setvar(s, "cipname", urllib.parse.quote(r["cipname"]))
    open(os.path.join(OUT, f"exp_cal_{i:02d}.grtload"), "w", encoding="utf-8").write(s)
    meta[f"{i:02d}"] = dict(caliber=r["cipname"], caliber_id=r["id"], projectile=f'{p["mname"]} {p["pname"]}', projectile_id=p["id"], powder_id=pid, powder=f'{w["mname"]} {w["pname"]}',
                            casevol_cm3=vol, bore_area_mm2=r["c_Q"], Dz=Z, L3=r["L3"], oal=oal, depth=depth, mp_g=mp, mc_g=mc, barrel_mm=barrel, Pmax_bar=r["Pmax"])
json.dump(meta, open(os.path.join(HERE, "calibers.json"), "w"), indent=1)
print(len(meta), "caliber jobs written")
