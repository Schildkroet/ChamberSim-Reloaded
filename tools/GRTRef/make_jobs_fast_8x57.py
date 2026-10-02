#!/usr/bin/env python3
"""Fast-powder experiments, batch 7. Output: jobs/exp_*.grtload (the old jobs/ content is archived in jobs_all/).

The remaining solver error is concentrated in fast powders (pw_258 -4 %, pw_30 -8 %, pw_595 -6 % in peak pressure).
Finer solver steps do not help (analysis in the memory notes), so the cause is the original's own burn scheme. This
batch tries to read that scheme from the curves of the original, one change at a time on the 8x57 IS base load:
  fa_Ba<Ba>[_xe/_mp]  : very fast powders (Ba 2..32): the burnt fraction z of the FIRST points shows the burn
                        increment of one step directly (it overshoots 1 and is clamped); varied with the step length
                        (barrel length) and the bullet mass (time per step)
  fb_...              : Ba 3 / 6 with extreme form-function parameters (z1, z2, a0): which phase the step error hits
  fc_ps<bar>          : start pressure (the initial gas z0) at Ba 3 / 8
  fd_...              : explosion heat, ratio of specific heats, material density at Ba 3 (gas model of fast powders)
  fe_...              : charge and case volume at Ba 7 and 2 (loading density trend of fast powders)
  fp_<caliber>_<powder>: realistic pistol loads (fast powders of the database in pistol and revolver cartridges)
The first job (exp_aa_warmup) only absorbs the original's empty-first-result glitch."""
import re, os, sys, json, sqlite3, subprocess, tempfile, shutil, urllib.parse
HERE = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.expanduser("~/Downloads/GordonsReloadingTool-2021.2040-NIGHTLY-linux/loads/8x57is.grtload")
DB = os.path.join(HERE, "..", "GordonsReloadingTool.plain.db")
OUT = os.path.join(HERE, "jobs")
GRAIN = 0.06479891

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

base = open(BASE, encoding="utf-8").read()
mc, mp, cv = getv(base, "propellant", "mc"), getv(base, "projectile", "mp"), getv(base, "caliber", "casevol")
os.makedirs(OUT, exist_ok=True)
for f in os.listdir(OUT):  # the jobs of an earlier batch are archived in jobs_all
    if f.endswith(".grtload"): os.remove(os.path.join(OUT, f))
n = 0
def write(name, s):
    global n
    open(os.path.join(OUT, f"exp_{name}.grtload"), "w", encoding="utf-8").write(s); n += 1
def P(s, key, v): return setv(s, "propellant", key, v)
def ba(s, v): return P(s, "Ba", v)

write("aa_warmup", base)

# A: first-step probes
for v in (2, 4, 8, 16, 32):
    write(f"fa_Ba{v}", ba(base, v))
for v in (4, 16):
    for xe in (150, 2400):
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
for f in (0.5, 1.15):
    write(f"fe7_mc_x{f}", P(b7, "mc", repr(mc * f)))
write("fe7_cv_x1.4", setv(b7, "caliber", "casevol", repr(cv * 1.4)))
for f in (0.5, 1.3):
    write(f"fe2_mc_x{f}", P(b2, "mc", repr(mc * f)))

# F: realistic pistol loads (calibers and powders of the database)
c = sqlite3.connect(f"file:{DB}?mode=ro", uri=True); c.row_factory = sqlite3.Row
CAL = {"9mm": 499, "45acp": 244, "38spl": 203, "357mag": 176, "40sw": 214, "44mag": 238}
POW = {"titegroup": 199, "n320": 678, "n340": 593, "bullseye": 57, "hp38": 166, "n350": 595}
PAIRS = [("9mm", "titegroup"), ("9mm", "n320"), ("45acp", "titegroup"), ("45acp", "bullseye"), ("38spl", "hp38"), ("38spl", "n340"),
         ("357mag", "n340"), ("357mag", "n350"), ("40sw", "n320"), ("40sw", "hp38"), ("44mag", "n350"), ("44mag", "n340"),
         ("9mm", "hp38"), ("45acp", "n320")]  # (.32 S&W Long: the solver rejects the generated load)
proj = [dict(r) for r in c.execute("select * from projectile where geloescht=0 and gdia>0 and glen>0 and gmass>0")]
def build(cal, w, fill):
    Z = cal["c_Z"]
    cand = sorted([p for p in proj if abs(p["gdia"] - Z) < 0.25], key=lambda p: p["gmass"])
    p = cand[len(cand) // 2]
    vol = cal["V"] * GRAIN
    depth = min(max(cal["L3"] + p["glen"] - cal["L6"], 2.0), 0.8 * p["glen"])
    oal = cal["L3"] + p["glen"] - depth
    s = base
    for key, val in (("CaliberName", urllib.parse.quote(cal["cipname"])), ("oal", round(oal, 3)), ("caselen", cal["L3"]), ("casevol", round(vol, 6)),
                     ("Dz", Z), ("Aeff", cal["c_Q"]), ("pMaxZul", cal["Pmax"]), ("sebert", cal["sebert"]), ("ps", p["gpressure"] or 250)):
        s = setv(s, "caliber", key, val)
    s = setv(s, "gun", "xe", 150)
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
# plain fraction of the bulk-density capacity gives loads that are far too full): pass 1 builds the loads with a charge
# of PROBE x capacity, grt_compare --loads reads their ratio, pass 2 scales the charge (the ratio is proportional to it).
EXE = os.environ.get("GRT_COMPARE", os.path.join(HERE, "..", "..", "build", "release", "grt_compare"))
LOW, HIGH = 0.45, 0.70
PROBE = 0.25  # fraction of the bulk-density capacity of the probe loads (small enough to be accepted)
tmp = tempfile.mkdtemp()
try:
    for cn, pn in PAIRS:
        cal = dict(c.execute("select * from caliber where id=?", (CAL[cn],)).fetchone())
        w = dict(c.execute("select * from propellant where id=?", (POW[pn],)).fetchone())
        open(os.path.join(tmp, f"exp_{cn}_{pn}.grtload"), "w", encoding="utf-8").write(build(cal, w, PROBE)[0])
    ratios = {}
    for line in subprocess.run([EXE, "--loads", tmp], capture_output=True, text=True, check=True).stdout.splitlines():
        m = re.match(r"exp_(\S+)\s+\S+\s+pmax\s+\S+\s+v0\s+\S+\s+burnt\s+\S+\s+ratio\s+([\d.]+)", line)
        if m: ratios[m.group(1)] = float(m.group(2))
        if m and float(m.group(2)) == 0.0: print("probe not accepted:", line)
finally:
    shutil.rmtree(tmp)
meta = {}
for cn, pn in PAIRS:
    key = f"{cn}_{pn}"
    r1 = ratios[key] / PROBE  # load ratio per unit of "fill"
    cal = dict(c.execute("select * from caliber where id=?", (CAL[cn],)).fetchone())
    w = dict(c.execute("select * from propellant where id=?", (POW[pn],)).fetchone())
    s, p = build(cal, w, LOW / r1)
    write(f"fp_{key}", s)
    meta[f"fp_{key}"] = dict(caliber=cal["cipname"], powder=f'{w["mname"]} {w["pname"]}', Ba=w["Ba"], bullet=f'{p["mname"]} {p["pname"]}', mp_gr=p["gmass"], ratio=LOW)
    write(f"fp_{key}_hi", build(cal, w, HIGH / r1)[0])  # a heavy charge of the same load
json.dump(meta, open(os.path.join(HERE, "fast_jobs.json"), "w"), indent=1)
print(n, "jobs written to", OUT)
