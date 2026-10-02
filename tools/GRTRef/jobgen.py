"""Helpers shared by the job generators of the reference runner (make_jobs_slow.py ...): editing a .grtload,
building a load of a database caliber / powder on the 8x57 template, and setting the charge by the LOAD RATIO the
solver reports (grt_compare --loads): the bullet seated in the case reduces the usable volume, so a plain fraction
of the bulk-density capacity gives loads that are far too full."""
import re, os, sqlite3, subprocess, tempfile, shutil, urllib.parse
HERE = os.path.dirname(os.path.abspath(__file__))
BASE8 = os.path.expanduser("~/Downloads/GordonsReloadingTool-2021.2040-NIGHTLY-linux/loads/8x57is.grtload")
DB = os.path.join(HERE, "..", "GordonsReloadingTool.plain.db")
EXE = os.environ.get("GRT_COMPARE", os.path.join(HERE, "..", "..", "build", "release", "grt_compare"))
OUT = os.path.join(HERE, "jobs")
GRAIN = 0.06479891
BARREL = 150  # mm, pistol / revolver loads
PROBE = 0.25  # fraction of the bulk-density capacity of the probe loads (small enough to be accepted)
CAL = {"9mm": 499, "45acp": 244, "38spl": 203, "357mag": 176, "40sw": 214, "44mag": 238}
POW = {"titegroup": 199, "n320": 678, "n340": 593, "bullseye": 57, "hp38": 166, "n350": 595}

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

def load_ratios(pairs):
    """Load ratio per unit of `fill` for (caliber, powder) pairs, from the solver (needs a Release grt_compare)."""
    tmp = tempfile.mkdtemp()
    try:
        for cn, pn in pairs:
            open(os.path.join(tmp, f"exp_{cn}_{pn}.grtload"), "w", encoding="utf-8").write(build(cn, pn, PROBE)[0])
        out = {}
        for line in subprocess.run([EXE, "--loads", tmp], capture_output=True, text=True, check=True).stdout.splitlines():
            m = re.match(r"exp_(\S+)\s+\S+\s+pmax\s+\S+\s+v0\s+\S+\s+burnt\s+\S+\s+ratio\s+([\d.]+)", line)
            if m and float(m.group(2)) > 0: out[m.group(1)] = float(m.group(2)) / PROBE
            elif m: print("probe not accepted by the solver:", line)
        return out
    finally:
        shutil.rmtree(tmp)

class Writer:
    """Writes jobs/exp_<name>.grtload (the old jobs are removed first: they are archived in jobs_all/)."""
    def __init__(self):
        os.makedirs(OUT, exist_ok=True)
        for f in os.listdir(OUT):
            if f.endswith(".grtload"): os.remove(os.path.join(OUT, f))
        self.n = 0
    def __call__(self, name, s):
        open(os.path.join(OUT, f"exp_{name}.grtload"), "w", encoding="utf-8").write(s)
        self.n += 1
