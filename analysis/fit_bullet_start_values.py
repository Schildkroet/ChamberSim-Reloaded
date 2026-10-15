#!/usr/bin/env python3
"""Start pressure (gpressure) and extra friction (gfriction) of the bullet records from the published loads with pressures.

    fit_bullet_start_values.py <csr_eval> samples.csv [samples2.csv ...] [--min-loads 8] [--write]

For every published load with a pressure the physical model is run with the record's values; loads more than 5 % off in velocity or pressure
are swept over gpressure 50..500 bar (coarse, then 5 bar steps) and gfriction -100..150 bar (coarse, then 5 bar steps) for the smallest
(v / 3 %)^2 + (p / 13 %)^2 (the noise of the tables). A rifle bullet record with at least --min-loads loads (maker, name and mass of the
record the load's bullet matches) gets the median of the best values of its loads (the record's own values for loads within 5 %), rounded to 5 bar.
Pistol and revolver cartridges are not fitted: their start pressure is set by the solver (handgunStartFraction), and bullet records
that also appear in handgun loads keep their values (fitted on rifle loads they made the handgun loads worse: velocity 8.9 -> 9.7 %). csr_eval needs the per-row
columns gpressure and gfriction.
Checked on held-out powders (values from half of a bullet's powders, errors on the other half, 2026-10-15, 553 bullets with at least 8 loads):
velocity error 3.87 -> 3.45 %, pressure 25.1 -> 23.7 %, loads over 5 % in pressure 68 -> 59 %. With at least 15 / 40 loads the gains are similar."""
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
args = ap.parse_args()
exe = args.eval_exe; files = args.samples
new=[r for _,r in recordfiles.read("data/projectiles")]
case={}
for f in glob.glob("data/calibers/*.json"):
    r=json.load(open(f)); case[r["cipname"]]=r
props={}
for _,r in recordfiles.read("data/propellants"): props.setdefault(r["pname"].lower(),[]).append(r["mname"]+" "+r["pname"])
full={(r["mname"]+" "+r["pname"]).lower() for _,r in recordfiles.read("data/propellants")}
def find(label,gr,dia):
    L=label.lower()
    c=[p for p in new if L.startswith(p["mname"].lower()) and abs(p["gmass"]-gr)<0.8 and abs(p["gdia"]-dia)<0.06]
    return max(c,key=lambda p:sum(w in p["pname"].lower() for w in re.findall(r"[a-z0-9]+",L))) if c else None
rows=[];seen=set()
for f in files:
    rr=list(csv.DictReader(open(f))); dr=inconsistent(rr,0.15)
    for i,r in enumerate(rr):
        if i in dr or not r.get("pressure_bar"): continue
        if r["powder"].lower() not in full and len(props.get(r["powder"].lower(),[]))==1: r["powder"]=props[r["powder"].lower()][0]
        k=(r["powder"].upper(),r["caliber"],r["bullet_gr"],r["kind"],r["charge_g"],r["velocity_ms"],r["col_mm"])
        if k in seen: continue
        seen.add(k)
        rec=find(r["bullet"],float(r["bullet_gr"]),float(r["bullet_dia_mm"]))
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
vals = {}
for k, v in groups.items():
    if len(v) < args.min_loads: continue
    votes = [(r["_d"]["fit"][0], r["_d"]["fit"][1]) if r["_d"]["fit"] else (r["_ps0"], r["_fr0"]) for r in v]
    ps = int(round(np.median([a for a, _ in votes]) / 5) * 5); fr = int(round(np.median([b for _, b in votes]) / 5) * 5)
    vals[k] = (min(500, max(50, ps)), min(150, max(-100, fr)))
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
