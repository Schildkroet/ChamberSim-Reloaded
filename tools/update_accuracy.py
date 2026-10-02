#!/usr/bin/env python3
"""Rewrites the accuracy table in README.md from the current solver.

usage: tools/update_accuracy.py [path/to/grt_compare]      (a Release build of the grt_compare target)

grt_compare runs the solver on every reference run of the original program (tools/GRTRef/jobs_all and
reference/results, both local only, not in the repository) and prints the signed errors per run. The table sits
between the markers <!-- accuracy:start --> and <!-- accuracy:end --> in README.md."""
import re, subprocess, sys, os, datetime
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
exe = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build", "release", "grt_compare")
out = subprocess.run([exe], capture_output=True, text=True, check=True).stdout

GROUPS = [  # (label, name regex)
    ("Other cartridges (caliber sweep)", r"^cal_"),
    ("Powders (40 powders, one cartridge)", r"^pw_"),
    ("Charge, bullet weight, start pressure, case volume", r"^(base|mc_|mp_|ps_|casevol_|lad_)"),
    ("Barrel length, twist, Sebert factor", r"^(xe_|twist_|sebert_)"),
    ("Powder temperature", r"^(pt_|t_|tc_)"),
    ("Bullet friction", r"^(friction|fr_)"),
    ("Almost closed vessel (very heavy bullet)", r"^cv_"),
    ("Fast powders (burn rate, step length, form function)", r"^ba3?_"),
    ("Fast-powder probes on the 8x57 (Ba 1..8, form function, start pressure, gas, charge)", r"^x7_f[abcde]|^x8_"),
    ("Powder probes on a 9 mm Luger (Ba 1..32, form function, start pressure, gas, charge)", r"^f[abcde]"),
    ("Pistol and revolver loads with fast powders", r"^fp_"),
    ("Slow powders at low pressure (9 mm Luger: bullet mass, barrel, charge, start pressure, ...)", r"^g[a-h]_"),
    ("Bullet jump (0.25 ... 4 mm, 8x57 and 9 mm)", r"^j[a-d]_"),
    ("Powder and start-pressure variations of the 8x57 (shotshell batch, flag off)", r"^sh_.*_off$"),
    ("Loading density (charge, case volume, powder density, Qex)", r"^ld"),
]
line = re.compile(r"^(\S+)\s+pmax\s+([+-]?[\d.]+) %\s+vmuz\s+([+-]?[\d.]+) %\s+curve rms\s+([\d.]+) %")
runs = []
for l in out.splitlines():
    m = line.match(l)
    if m: runs.append((m.group(1), abs(float(m.group(2))), abs(float(m.group(3))), float(m.group(4))))

def row(label, rs):
    n = len(rs)
    mean = lambda i: sum(r[i] for r in rs) / n
    return f"| {label} | {n} | {mean(1):.1f} % | {max(r[1] for r in rs):.1f} % | {mean(2):.1f} % | {max(r[2] for r in rs):.1f} % | {mean(3):.1f} % |"

rows, used = [], set()
for label, rx in GROUPS:
    rs = [r for r in runs if re.search(rx, r[0])]
    if rs: rows.append(row(label, rs)); used.update(r[0] for r in rs)
rest = [r for r in runs if r[0] not in used]
if rest: rows.append(row("Other", rest))
table = "\n".join([
    "| Test set | Runs | Peak pressure: mean error | max | Muzzle velocity: mean error | max | Curve shape: mean deviation |",
    "|---|---:|---:|---:|---:|---:|---:|",
    *rows,
    row("**All runs**", runs),
])
text = (f"<!-- accuracy:start -->\n{table}\n\n"
        f"_Absolute errors against {len(runs)} runs of the original program; updated {datetime.date.today().isoformat()} "
        f"with `tools/update_accuracy.py`. \"Curve shape\" is the rms relative pressure error along the barrel (where the "
        f"pressure is above 5 % of its maximum). The charge-too-high rule matches the original for all tested loads._\n<!-- accuracy:end -->")
p = os.path.join(ROOT, "README.md")
s = open(p, encoding="utf-8").read()
new, n = re.subn(r"<!-- accuracy:start -->.*?<!-- accuracy:end -->", lambda m: text, s, flags=re.S)
if n != 1: sys.exit("markers not found in README.md")
open(p, "w", encoding="utf-8").write(new)
print(table)
