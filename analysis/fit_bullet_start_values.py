#!/usr/bin/env python3
"""Start pressure (gpressure) and extra friction (gfriction) of the bullet records from the published loads with pressures.

    fit_bullet_start_values.py <csr_eval> samples.csv [samples2.csv ...] [--min-loads 8] [--validated-only] [--holdout] [--write]

For every published load with a pressure the physical model is run with the record's values; loads more than 5 % off in velocity or pressure
are swept over gpressure 50..500 bar (coarse, then 5 bar steps) and gfriction -100..150 bar (coarse, then 5 bar steps) for the smallest
(v / 3 %)^2 + (p / 13 %)^2 (the noise of the tables). A rifle bullet record with at least --min-loads loads (maker, name and mass of the
record the load's bullet matches) gets the median of the best values of its loads (the record's own values for loads within 5 %), rounded to 5 bar.
Pistol and revolver cartridges are not fitted: their start pressure is set by the solver (handgunStartFraction), and bullet records
that also appear in handgun loads keep their values (fitted on rifle loads they made the handgun loads worse: velocity 8.9 -> 9.7 %). csr_eval needs the per-row
columns gpressure and gfriction.
Checked on held-out powders (values from half of a bullet's powders, errors on the other half, 2026-10-15, 553 bullets with at least 8 loads):
velocity error 3.87 -> 3.45 %, pressure 25.1 -> 23.7 %, loads over 5 % in pressure 68 -> 59 %. With at least 15 / 40 loads the gains are similar.
2026-11-16: the values written then were reset to the construction classes of scripts/data/bulletdata/start_pressure.py and the friction removed.
They had been fitted against bullet lengths of the sample files that were out of date (about 21,000 rows, see rematch_lengths.py --stale) and partly
made up for them: with the corrected lengths they made the rifle loads worse (shipped powders: velocity 4.17 -> 4.25 %, pressure 12.73 -> 12.88 %
against the classes). Since then the record of a load is samples_common.Records.match's (the extractors' matcher) and a load only counts when its
bullet length is the record's (corrected samples); --validated-only fits only records with a validated length (gValid); --holdout fits on half of
the powders and compares the values with the records' (construction classes) on the other half, both ways; nothing is written.
Run 2026-11-16 with --validated-only on the corrected loads: held-out velocity 3.30 -> 3.19 %, pressure 12.79 -> 11.53 % (4,368 loads); 162 records
written (79 with friction)."""
import argparse, collections, csv, glob, json, os, pickle, re, subprocess, sys, tempfile
import numpy as np
from concurrent.futures import ThreadPoolExecutor
sys.path.insert(0,"analysis"); sys.path.insert(0,"scripts/data"); from sample_filter import inconsistent
import recordfiles
ap = argparse.ArgumentParser()
ap.add_argument("eval_exe"); ap.add_argument("samples", nargs="+")
ap.add_argument("--cache", default="bullet_start_fits.pkl", help="per-load sweep results (kept between runs)")
ap.add_argument("--min-loads", type=int, default=8, help="rifle loads of a bullet needed for its own values")
ap.add_argument("--write", action="store_true", help="write gpressure / gfriction into data/projectiles")
ap.add_argument("--validated-only", action="store_true", help="only records with a validated length (gValid) get own values")
ap.add_argument("--holdout", action="store_true", help="fit on half of the powders, score on the other half (both ways); no writing")
args = ap.parse_args()
exe = args.eval_exe; files = args.samples
new=[r for _,r in recordfiles.read("data/projectiles")]
case={}
for f in glob.glob("data/calibers/*.json"):
    r=json.load(open(f)); case[r["cipname"]]=r
props={}
for _,r in recordfiles.read("data/propellants"): props.setdefault(r["pname"].lower(),[]).append(r["mname"]+" "+r["pname"])
full={(r["mname"]+" "+r["pname"]).lower() for _,r in recordfiles.read("data/propellants")}
sys.path.insert(0,"scripts/data/loaddata"); from samples_common import Records
from pathlib import Path
REC=Records(Path("data"))
def find(label,gr,dia,length):
    """The record of the load's bullet (Records.match) when the load uses its length (else the load's length came from elsewhere)."""
    maker,btype=REC.split(label)
    rec=REC.match(maker,btype,gr,dia) if maker else None
    if rec is None or abs(rec["glen"]-length)>0.05: return None
    if args.validated_only and not rec.get("gValid"): return None
    return rec
rows=[];seen=set()
for f in files:
    rr=list(csv.DictReader(open(f))); dr=inconsistent(rr,0.15)
    for i,r in enumerate(rr):
        if i in dr or not r.get("pressure_bar"): continue
        if r["powder"].lower() not in full and len(props.get(r["powder"].lower(),[]))==1: r["powder"]=props[r["powder"].lower()][0]
        k=(r["powder"].upper(),r["caliber"],r["bullet_gr"],r["kind"],r["charge_g"],r["velocity_ms"],r["col_mm"])
        if k in seen: continue
        seen.add(k)
        rec=find(r["bullet"],float(r["bullet_gr"]),float(r["bullet_dia_mm"]),float(r["bullet_len_mm"] or 0))
        r["_ps0"]=(rec or {}).get("gpressure") or 250.0; r["_fr0"]=(rec or {}).get("gfriction") or 0.0
        r["_key"]=(rec["mname"],rec["pname"],rec["gmass"]) if rec else None
        r["_cls"]="pistol" if "/tab-iv/" in case.get(r["caliber"],{}).get("cippdf","") else "rifle"
        rows.append(r)
print(len(rows),"loads with pressure",flush=True)
header=sorted(set().union(*[set(k for k in r if not k.startswith("_")) for r in rows])|{"gpressure","gfriction"})
def evalrows(items):
    # items: list of (row, ps, fr) -> arrays ev, ep
    with tempfile.NamedTemporaryFile("w",suffix=".csv",newline="",delete=False) as f:
        w=csv.DictWriter(f,header,restval=""); w.writeheader()
        for r,ps,fr in items:
            d={k:v for k,v in r.items() if not k.startswith("_")}; d["gpressure"]=ps; d["gfriction"]=fr; w.writerow(d)
    out=subprocess.run([exe,f.name,"--profile","physical"],capture_output=True,text=True,env={**os.environ,"QT_QPA_PLATFORM":"offscreen"}).stdout; os.unlink(f.name)
    ev=np.full(len(items),np.nan); ep=np.full(len(items),np.nan)
    for l in out.splitlines():
        a=l.split(","); i=int(a[0]); r=items[i][0]
        ev[i]=np.log(float(a[1])/float(r["velocity_ms"])); ep[i]=np.log(float(a[2])/float(r["pressure_bar"]))
    return ev,ep
SV,SP=0.03,0.13
J=lambda ev,ep:(ev/SV)**2+(ep/SP)**2
PS1=list(range(50,501,50)); FR1=[-100,-50,0,50,100,150]
def work(batch):
    res=[]
    base=evalrows([(r,r["_ps0"],r["_fr0"]) for r in batch])
    flag=[i for i in range(len(batch)) if not np.isnan(base[0][i]) and (abs(base[0][i])>0.05 or abs(base[1][i])>0.05)]
    items=[(batch[i],ps,fr) for i in flag for ps in PS1 for fr in FR1]
    if items:
        ev,ep=evalrows(items); j=np.where(np.isnan(ev)|np.isnan(ep),1e9,J(ev,ep)).reshape(len(flag),-1)
        best1=[(PS1[int(np.argmin(row))//len(FR1)],FR1[int(np.argmin(row))%len(FR1)]) for row in j]
        items2=[];meta=[]
        for k,i in enumerate(flag):
            ps0,fr0=best1[k]
            for ps in range(max(50,ps0-25),min(500,ps0+25)+1,5):
                for fr in range(max(-100,fr0-25),min(150,fr0+25)+1,5):
                    items2.append((batch[i],ps,fr)); meta.append(k)
        ev2,ep2=evalrows(items2); j2=np.where(np.isnan(ev2)|np.isnan(ep2),1e9,J(ev2,ep2)); meta=np.array(meta)
        for k,i in enumerate(flag):
            idx=np.where(meta==k)[0]; b=idx[int(np.argmin(j2[idx]))]
            res.append((i,items2[b][1],items2[b][2],ev2[b],ep2[b]))
    return batch,base,res
batches=[rows[i:i+150] for i in range(0,len(rows),150)]
out=[]
if os.path.exists(args.cache):
    out = pickle.load(open(args.cache, "rb")); batches = []; print("per-load results from", args.cache)
with ThreadPoolExecutor(16) as p:
    for n,(batch,base,res) in enumerate(p.map(work,batches)):
        fitted={i:(ps,fr,ev,ep) for i,ps,fr,ev,ep in res}
        for i,r in enumerate(batch):
            out.append(dict(label=r["bullet"],gr=float(r["bullet_gr"]),dia=float(r["bullet_dia_mm"]),cls=r["_cls"],cal=r["caliber"],powder=r["powder"],kind=r["kind"],key=r["_key"],ps0=r["_ps0"],fr0=r["_fr0"],
                ev0=base[0][i],ep0=base[1][i],fit=fitted.get(i)))
        if n%10==0: print("batch",n,"of",len(batches),flush=True)
if batches: pickle.dump(out, open(args.cache, "wb"))
for r, d in zip(rows, out): r["_d"] = d
groups = collections.defaultdict(list)
handgun = {r["_d"]["key"] for r in rows if r["_cls"] == "pistol"}  # records also used in handgun loads keep their values (the fit is rifle only)
for r in rows:
    d = r["_d"]
    if r["_cls"] == "rifle" and d["key"] not in handgun and d["key"] and not np.isnan(d["ev0"]): groups[d["key"]].append(r)
def values(keep=lambda r: True):
    vals = {}
    for k, v in groups.items():
        v = [r for r in v if keep(r)]
        if len(v) < args.min_loads: continue
        votes = [(r["_d"]["fit"][0], r["_d"]["fit"][1]) if r["_d"]["fit"] else (r["_ps0"], r["_fr0"]) for r in v]
        ps = int(round(np.median([a for a, _ in votes]) / 5) * 5); fr = int(round(np.median([b for _, b in votes]) / 5) * 5)
        vals[k] = (min(500, max(50, ps)), min(150, max(-100, fr)))
    return vals
if args.holdout:
    powders = sorted({r["powder"] for v in groups.values() for r in v}); half = set(powders[::2])
    tot = {"records": [], "own": []}
    for side in (True, False):
        vals = values(lambda r: (r["powder"] in half) == side)
        test = [r for k, v in groups.items() if k in vals for r in v if (r["powder"] in half) != side]
        a = evalrows([(r, r["_ps0"], r["_fr0"]) for r in test]); b = evalrows([(r, *vals[r["_key"]]) for r in test])
        print(f"half {int(side)}: {len(vals)} bullets fitted, {len(test)} held-out loads", flush=True)
        tot["records"].append(a); tot["own"].append(b)
    for name, parts in tot.items():
        ev = np.concatenate([p[0] for p in parts]); ep = np.concatenate([p[1] for p in parts]); m = ~(np.isnan(ev) | np.isnan(ep))
        print(f"  held-out loads with {name:7s} values: n={m.sum()}  velocity rms {100*np.sqrt(np.mean(ev[m]**2)):.2f} % (bias {100*np.mean(ev[m]):+.2f})"
              f"  pressure rms {100*np.sqrt(np.mean(ep[m]**2)):.2f} % (bias {100*np.mean(ep[m]):+.2f})")
    sys.exit(0)
vals = values()
print(len(vals), "bullet records with their own values")
if args.write:
    recs = [r for _, r in recordfiles.read("data/projectiles")]
    for r in recs:
        v = vals.get((r["mname"], r["pname"], r["gmass"]))
        if not v: continue
        r["gpressure"] = float(v[0])
        if v[1]: r["gfriction"] = float(v[1])
        else: r.pop("gfriction", None)
    recordfiles.write("data/projectiles", recs); print("written data/projectiles")
