#!/usr/bin/env python3
"""Powder sweep: put N powders from the DB into the 8x57 IS base load (same fill ratio).
Output: jobs/exp_pw_<id>.grtload  + powders.json (the parameters used)."""
import re, os, sys, json, sqlite3, random, urllib.parse
HERE = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.expanduser("~/Downloads/GordonsReloadingTool-2021.2040-NIGHTLY-linux/loads/8x57is.grtload")
DB = os.path.join(HERE, "..", "GordonsReloadingTool.plain.db")
OUT = os.path.join(HERE, "jobs"); N = int(sys.argv[1]) if len(sys.argv) > 1 else 40

def setv(s, section, name, value):
    m = re.search(rf"<{section}>.*?</{section}>", s, re.S); blk = m.group(0)
    new, n = re.subn(rf'(<input name="{name}" value=")[^"]*(")', lambda mm: mm.group(1) + str(value) + mm.group(2), blk)
    assert n == 1, (section, name, n)
    return s[:m.start()] + new + s[m.end():]

c = sqlite3.connect(f"file:{DB}?mode=ro", uri=True)
rows = c.execute("select id,mname,pname,lotid,Ba,Qex,k,a0,a1,z1,z2,eta,pc,pcd from propellant where geloescht=0 and Ba>0 and pcd>0").fetchall()
rows.sort(key=lambda r: r[7])                       # by a0: take extremes + an even spread
pick = {rows[0][0]: rows[0], rows[-1][0]: rows[-1]}
random.seed(7)
step = len(rows) / (N - 2)
for i in range(N - 2):
    r = rows[min(int(i * step + random.random() * step), len(rows) - 1)]; pick[r[0]] = r
base = open(BASE, encoding="utf-8").read()
mc0 = float(re.search(r'name="mc" value="([^"]*)"', base).group(1)); pcd0 = 890.0
os.makedirs(OUT, exist_ok=True); meta = {}
for r in pick.values():
    pid, mn, pn, lot, Ba, Qex, k, a0, a1, z1, z2, eta, pc, pcd = r
    s = base
    for key, val in (("Ba", Ba), ("Qex", Qex), ("k", k), ("a0", a0), ("a1", a1), ("z1", z1), ("z2", z2), ("eta", eta), ("pc", pc), ("pcd", pcd),
                     ("mc", repr(mc0 * pcd / pcd0)), ("mname", urllib.parse.quote(mn)), ("pname", urllib.parse.quote(pn)), ("lotid", urllib.parse.quote(lot or ""))):
        s = setv(s, "propellant", key, val)
    open(os.path.join(OUT, f"exp_pw_{pid}.grtload"), "w", encoding="utf-8").write(s)
    meta[pid] = dict(mname=mn, pname=pn, Ba=Ba, Qex=Qex, k=k, a0=a0, a1=a1, z1=z1, z2=z2, eta=eta, pc=pc, pcd=pcd, mc_g=mc0 * pcd / pcd0)
json.dump(meta, open(os.path.join(HERE, "powders.json"), "w"), indent=1)
print(len(pick), "powder jobs written")
