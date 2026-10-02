#!/usr/bin/env python3
"""Fast-powder experiments, batch 7. Output: jobs/exp_*.grtload (the old jobs/ content is archived in jobs_all/).

The remaining solver error is concentrated in fast powders (pw_258 -4 %, pw_30 -8 %, pw_595 -6 % in peak pressure).
Finer solver steps do not help (analysis in the memory notes), so the cause is the original's own burn scheme. Fast
powders belong in small cases with short barrels, so the controlled one-factor-at-a-time probes use a 9 mm Luger load
as the base (the 8x57 + Ba 3..7 combination of the earlier batches burns out within a few steps of the 2000 point
grid, which no real load does). Groups:
  fa_Ba<Ba>[_xe/_mp] : Ba 1..32: the burnt fraction z of the FIRST points shows the burn increment of one step directly
                       (it overshoots 1 and is clamped); varied with the step length (barrel length) and the bullet
                       mass (time per step)
  fb*_...            : Ba 3 / 6 with extreme form-function parameters (z1, z2, a0): which phase the step error hits
  fc*_ps<bar>        : start pressure (the initial gas z0)
  fd3_...            : explosion heat, ratio of specific heats, material density (gas model of fast powders)
  fe*_...            : charge and case volume (loading density trend of fast powders)
  fp_<caliber>_<powder>[_hi] : realistic pistol / revolver loads with fast powders of the database
  x8_...             : the same Ba / barrel length probes on the 8x57 IS (comparison with the earlier batches)
The first job (exp_aa_warmup) only absorbs the original's empty-first-result glitch.
Needs a Release build of grt_compare (GRT_COMPARE or build/release/grt_compare): the charges are set by the load ratio."""
import re, os, json, sqlite3, subprocess, tempfile, shutil, urllib.parse
HERE = os.path.dirname(os.path.abspath(__file__))
BASE8 = os.path.expanduser("~/Downloads/GordonsReloadingTool-2021.2040-NIGHTLY-linux/loads/8x57is.grtload")
DB = os.path.join(HERE, "..", "GordonsReloadingTool.plain.db")
EXE = os.environ.get("GRT_COMPARE", os.path.join(HERE, "..", "..", "build", "release", "grt_compare"))
OUT = os.path.join(HERE, "jobs")
GRAIN = 0.06479891
LOW, HIGH = 0.45, 0.70  # load ratios of the pistol loads
PROBE = 0.25            # fraction of the bulk-density capacity of the probe loads (small enough to be accepted)
BARREL = 150            # mm

def setv(s, section, name, value):
    m = re.search(rf"<{section}>.*?</{section}>", s, re.S); blk = m.group(0)
    new, n = re.subn(rf'(<input name="{name}" value=")[^"]*(")', lambda mm: mm.group(1) + str(value) + mm.group(2), blk)
    assert n == 1, (section, name, n)
    return s[:m.start()] + new + s[m.end():]
def getv(s, section, name):
    blk = re.search(rf"<{section}>.*?</{section}>", s, re.S).group(0)
    return float(re.search(rf'<input name="{name}" value="([^"]*)"', blk).group(1))
def setvar(s, name, value):
    return re.subn(rf'(<var name="{name}" value=")[^"]*(")', lambda mm: mm.group(1) + str(value) + mm.group(2), s)[0]

base8 = open(BASE8, encoding="utf-8").read()
db = sqlite3.connect(f"file:{DB}?mode=ro", uri=True); db.row_factory = sqlite3.Row
CAL = {"9mm": 499, "45acp": 244, "38spl": 203, "357mag": 176, "40sw": 214, "44mag": 238}
POW = {"titegroup": 199, "n320": 678, "n340": 593, "bullseye": 57, "hp38": 166, "n350": 595}
proj = [dict(r) for r in db.execute("select * from projectile where geloescht=0 and gdia>0 and glen>0 and gmass>0")]

def build(cn, pn, fill):
    """A load of caliber `cn` with powder `pn` (names of CAL / POW) and a charge of `fill` x bulk-density capacity."""
    cal = dict(db.execute("select * from caliber where id=?", (CAL[cn],)).fetchone())
    w = dict(db.execute("select * from propellant where id=?", (POW[pn],)).fetchone())
    Z = cal["c_Z"]
    cand = sorted([p for p in proj if abs(p["gdia"] - Z) < 0.25], key=lambda p: p["gmass"])
    p = cand[len(cand) // 2]
    vol = cal["V"] * GRAIN
    depth = min(max(cal["L3"] + p["glen"] - cal["L6"], 2.0), 0.8 * p["glen"])
    oal = cal["L3"] + p["glen"] - depth
    s = base8
    for key, val in (("CaliberName", urllib.parse.quote(cal["cipname"])), ("oal", round(oal, 3)), ("caselen", cal["L3"]), ("casevol", round(vol, 6)),
                     ("Dz", Z), ("Aeff", cal["c_Q"]), ("pMaxZul", cal["Pmax"]), ("sebert", cal["sebert"]), ("ps", p["gpressure"] or 250)):
        s = setv(s, "caliber", key, val)
    s = setv(s, "gun", "xe", BARREL)
    for key, val in (("ProjectileName", urllib.parse.quote(f'{p["mname"]}, {p["pname"]}')), ("mname", urllib.parse.quote(p["mname"])), ("pname", urllib.parse.quote(p["pname"])),
                     ("Dbul", p["gdia"]), ("mp", repr(p["gmass"] * GRAIN)), ("gdepth", round(depth, 3)), ("glen", p["glen"]), ("gpressure", p["gpressure"] or 250),
                     ("gtailh", p["gtailh"]), ("gTailDiaA", p["gtaildiaA"]), ("gTailDiaB", p["gtaildiaB"]), ("gTailType", p["gtailtype"]), ("g1bc", p["g1bc"]), ("g7bc", p["g7bc"])):
        s = setv(s, "projectile", key, val)
    for key, val in (("mname", urllib.parse.quote(w["mname"])), ("pname", urllib.parse.quote(w["pname"])), ("Ba", w["Ba"]), ("Qex", w["Qex"]), ("k", w["k"]), ("a0", w["a0"]),
                     ("a1", w["a1"]), ("z1", w["z1"]), ("z2", w["z2"]), ("eta", w["eta"]), ("pc", w["pc"]), ("pcd", w["pcd"]),
                     ("mc", repr(fill * w["pcd"] * 1e-3 * vol))):
        s = setv(s, "propellant", key, val)
    for key in ("L0","L1","L2","L3","L4","L5","L6","R","R1","R3","E","E1","Emin","FG","P0","P1","P2","S","r1min","r2","H1","H2","G1","G2","f","L3G","Pmax","PK","PE","EE","V","sebert"):
        if key in cal: s = setvar(s, key, cal[key])
    return setvar(s, "cipname", urllib.parse.quote(cal["cipname"])), p

# The charge is set by the LOAD RATIO the solver reports (the bullet seated in the case reduces the usable volume, so a
# plain fraction of the bulk-density capacity gives loads that are far too full): the probe loads (charge PROBE x
# capacity) are checked with grt_compare --loads, the ratio is proportional to the charge.
PAIRS = [("9mm", "titegroup"), ("9mm", "n320"), ("45acp", "titegroup"), ("45acp", "bullseye"), ("38spl", "hp38"), ("38spl", "n340"),
         ("357mag", "n340"), ("357mag", "n350"), ("40sw", "n320"), ("40sw", "hp38"), ("44mag", "n350"), ("44mag", "n340"),
         ("9mm", "hp38"), ("45acp", "n320")]
tmp = tempfile.mkdtemp()
try:
    for cn, pn in PAIRS:
        open(os.path.join(tmp, f"exp_{cn}_{pn}.grtload"), "w", encoding="utf-8").write(build(cn, pn, PROBE)[0])
    ratios = {}
    for line in subprocess.run([EXE, "--loads", tmp], capture_output=True, text=True, check=True).stdout.splitlines():
        m = re.match(r"exp_(\S+)\s+\S+\s+pmax\s+\S+\s+v0\s+\S+\s+burnt\s+\S+\s+ratio\s+([\d.]+)", line)
        if m and float(m.group(2)) > 0: ratios[m.group(1)] = float(m.group(2))
        elif m: print("probe not accepted by the solver:", line)
finally:
    shutil.rmtree(tmp)
def fill_for(cn, pn, ratio): return ratio / (ratios[f"{cn}_{pn}"] / PROBE)

os.makedirs(OUT, exist_ok=True)
for f in os.listdir(OUT):  # the jobs of an earlier batch are archived in jobs_all
    if f.endswith(".grtload"): os.remove(os.path.join(OUT, f))
n = 0
def write(name, s):
    global n
    open(os.path.join(OUT, f"exp_{name}.grtload"), "w", encoding="utf-8").write(s); n += 1
def P(s, key, v): return setv(s, "propellant", key, v)
def ba(s, v): return P(s, "Ba", v)

write("aa_warmup", base8)

# the probe base: 9 mm Luger, Titegroup (its Ba is replaced), load ratio 0.45
base, _ = build("9mm", "titegroup", fill_for("9mm", "titegroup", LOW))
mc, mp, cv = getv(base, "propellant", "mc"), getv(base, "projectile", "mp"), getv(base, "caliber", "casevol")

# A: first-step probes
for v in (1, 2, 4, 8, 16, 32):
    write(f"fa_Ba{v}", ba(base, v))
for v in (4, 16):
    for xe in (50, 600):
        write(f"fa_Ba{v}_xe{xe}", setv(ba(base, v), "gun", "xe", xe))
    for f in (0.5, 2.0):
        write(f"fa_Ba{v}_mp{f}", setv(ba(base, v), "projectile", "mp", repr(mp * f)))

# B: form function at Ba 3 / 6
b3, b6 = ba(base, 3.0), ba(base, 6.0)
for z1, z2 in ((0.05, 0.12), (0.3, 0.45), (0.9, 0.95)):
    write(f"fb3_z{z1}_{z2}", P(P(b3, "z1", z1), "z2", z2))
for a0 in (1.0, 5.0):
    write(f"fb3_a0_{a0}", P(b3, "a0", a0))
for a0 in (0.0, 3.5):
    write(f"fb6_a0_{a0}", P(b6, "a0", a0))
write("fb6_z0.05_0.12", P(P(b6, "z1", 0.05), "z2", 0.12))

# C: start pressure
for ps in (50, 500, 1500):
    write(f"fc3_ps{ps}", setv(b3, "caliber", "ps", ps))
for ps in (50, 1000):
    write(f"fc8_ps{ps}", setv(ba(base, 8.0), "caliber", "ps", ps))

# D: gas properties at Ba 3
for q in (2800, 5000):
    write(f"fd3_qex{q}", P(b3, "Qex", q))
for k in (1.15, 1.3):
    write(f"fd3_k{k}", P(b3, "k", k))
for pc in (1300, 1900):
    write(f"fd3_pc{pc}", P(b3, "pc", pc))

# E: charge and case volume at Ba 7 and 2
b7, b2 = ba(base, 7.0), ba(base, 2.0)
for f in (0.5, 1.3):
    write(f"fe7_mc_x{f}", P(b7, "mc", repr(mc * f)))
write("fe7_cv_x1.4", setv(b7, "caliber", "casevol", repr(cv * 1.4)))
for f in (0.5, 1.3):
    write(f"fe2_mc_x{f}", P(b2, "mc", repr(mc * f)))

# F: realistic pistol loads
meta = {}
for cn, pn in PAIRS:
    key = f"{cn}_{pn}"
    s, p = build(cn, pn, fill_for(cn, pn, LOW))
    w = dict(db.execute("select * from propellant where id=?", (POW[pn],)).fetchone())
    write(f"fp_{key}", s)
    meta[f"fp_{key}"] = dict(caliber=cn, powder=f'{w["mname"]} {w["pname"]}', Ba=w["Ba"], bullet=f'{p["mname"]} {p["pname"]}', mp_gr=p["gmass"], ratio=LOW)
    write(f"fp_{key}_hi", build(cn, pn, fill_for(cn, pn, HIGH))[0])  # a heavy charge of the same load

# X: the 8x57 IS comparison (earlier batches): same Ba and barrel length variations
for v in (4, 16):
    write(f"x8_Ba{v}", ba(base8, v))
    write(f"x8_Ba{v}_xe150", setv(ba(base8, v), "gun", "xe", 150))
json.dump(meta, open(os.path.join(HERE, "fast_jobs.json"), "w"), indent=1)
print(n, "jobs written to", OUT)
