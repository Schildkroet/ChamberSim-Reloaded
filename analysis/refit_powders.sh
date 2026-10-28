#!/bin/bash
# Refits the physical burn values of the powders with the published loads of scripts/data/loaddata/sources and judges the result. This is how the refit of
# 2026-10-23 (Barnes and H&N loads added) was done; see scripts/data/loaddata/README.md "Refit with the new sources" for the results.
#
#   [BY_MAKER=<dir>] [CLEAN_DIR=<dir>] analysis/refit_powders.sh <csr_eval> <work dir> [steps]   steps: groups clean ba multistart (default: all), run from the repository root
#
# <csr_eval> is a Release build of tools/csr_eval.cpp, <work dir> an empty scratch directory (outside the tree). Needs a venv with numpy + scipy
# (PYTHON=... overrides python3). Takes about 20 minutes (Ba refit 8 min, multi-start 40 min on 16 cores). The data/ tree is NOT changed:
# the candidates are in <work dir>/phys/*.json; adopt powders by hand (see the end of the output).
#
# 1 groups     by_maker.py (all samples sorted by powder maker, loads once); P = powders with >= 10 measured pressures, Q = powders without (>= 20 loads).
# 2 clean      filter_published_outliers.py: copies without the loads whose simulated pressure is > 45 % off (sources/samples_clean).
# 3 ba         fit_physical_powders.py --fit ba --keep-form: Ba of P from the measured pressures, Ba of Q with the C.I.P. limit - 50 bar as the pressure
#              of the maximum loads (the form functions stay as shipped). Free refits of a0, z1, z2 are clearly worse: do not use --fit form for P.
# 4 multistart fit_physical_multistart.py on Q without the grain-geometry powders (8 start shapes, cost must fall by 1 %).
# 5 compare    compare_physical_published.py of every candidate against the shipped values (rifle / pistol, with the bullets' start pressures) and the
#              measured loads of data/measurements. The work dir's phys/ has the copies of calibers.json and primers.json that csr_eval reads next to
#              --physical (without them every comparison is biased: rifle +0.3, pistol +0.9 points); the check "shipped file as --physical" must give 0.
set -euo pipefail
EVAL=$(realpath "$1"); WORK=$(realpath -m "$2"); STEPS=${3:-groups clean ba multistart}
PY=${PYTHON:-python3}
ROOT=$(pwd); SAMPLES=scripts/data/loaddata/sources/samples; CLEAN=${CLEAN_DIR:-scripts/data/loaddata/sources/samples_clean}
BYM=${BY_MAKER:-$SAMPLES/by_maker}   # BY_MAKER=<dir>: per-maker files to use instead (e.g. sources/samples_checked from check_published_loads.py --apply-hard); by_maker.py is then not run
KEEP="rws-rottweil r907,vectan tubal 3000,vectan tubal 5000,vihtavuori n570,vihtavuori n110"   # grain geometry (fit_grain_geometry.py): never refitted here
PHYS=$WORK/phys; mkdir -p "$PHYS"
cp data/physical/calibers.json data/physical/primers.json "$PHYS/"; cp data/physical/propellants.json "$PHYS/base.json"; cp "$PHYS/base.json" "$PHYS/ba.json"
has() { [[ " $STEPS " == *" $1 "* ]]; }
if has groups; then
  [ -z "${BY_MAKER:-}" ] && $PY scripts/data/loaddata/by_maker.py
  $PY - "$WORK" "$BYM" <<'PYEOF'
import csv, glob, json, sys, collections
sys.path.insert(0, "scripts/data")
import recordfiles
rec = {(r["mname"] + " " + r["pname"]).lower(): r for _, r in recordfiles.read("data/propellants")}
byname = collections.defaultdict(list)
for k, r in rec.items(): byname[r["pname"].lower()].append(k)
n = collections.Counter(); p = collections.Counter()
for f in glob.glob(sys.argv[2] + "/*.csv"):
    if "shot" in f: continue
    for r in csv.DictReader(open(f)):
        k = r["powder"].lower()
        if k not in rec and len(byname.get(k, [])) == 1: k = byname[k][0]
        if k not in rec: continue
        n[k] += 1
        if r.get("pressure_bar") and 500 <= float(r["pressure_bar"]) <= 9000: p[k] += 1
P = sorted(k for k in n if p[k] >= 10); Q = sorted(k for k in n if p[k] < 10 and n[k] >= 20)
open(sys.argv[1] + "/P.txt", "w").write(",".join(P)); open(sys.argv[1] + "/Q.txt", "w").write(",".join(Q))
print(f"{len(rec)} records: {len(P)} powders with pressures (P), {len(Q)} without (Q, >= 20 loads)")
PYEOF
fi
FILES=$(ls $BYM/*.csv | grep -v "shot\|other\|norma" | tr '\n' ' ')
if has clean; then
  rm -rf $CLEAN; $PY analysis/filter_published_outliers.py "$EVAL" $FILES --out $CLEAN
fi
CLEANFILES=$(ls $CLEAN/*.csv | grep -v "removed\|norma" | tr '\n' ' ')
if has ba; then
  $PY analysis/fit_physical_powders.py "$EVAL" $CLEANFILES --fit ba --keep-form --start "$PHYS/base.json" --out "$PHYS/ba.json" --merge --only "$(cat $WORK/P.txt)" > "$WORK/fitP.log"
  $PY analysis/fit_physical_powders.py "$EVAL" $CLEANFILES --fit ba --keep-form --start "$PHYS/ba.json" --out "$PHYS/ba.json" --merge --limit-pressure --limit-margin 50 \
      --min-samples 20 --only "$(cat $WORK/Q.txt)" > "$WORK/fitQ.log"
fi
if has multistart; then
  $PY analysis/fit_physical_multistart.py "$EVAL" $CLEANFILES --powders "$(cat $WORK/Q.txt)" --keep "$KEEP" --start "$PHYS/base.json" --out "$PHYS/ms.json" --work "$WORK/ms" | tee "$WORK/ms.log"
fi
echo "== shipped file as --physical (must be zero change)"
$PY analysis/compare_physical_published.py "$EVAL" $CLEANFILES --projectiles data/projectiles --eval-arg=--physical="$PHYS/base.json" --baseline "$EVAL" | grep -E "^(Rifle|Pistol)" | tail -2
for c in ba ms; do
  [ -f "$PHYS/$c.json" ] || continue
  echo "== $c.json against the shipped values: velocity / pressure rms, bias"
  $PY analysis/compare_physical_published.py "$EVAL" $CLEANFILES --projectiles data/projectiles --eval-arg=--physical="$PHYS/$c.json" --baseline "$EVAL" | grep -E "^(Rifle|Pistol)" | tail -2
  for m in beschussamt deva; do echo "measured $m ($c): $("$EVAL" data/measurements/${m}_loads.csv --profile physical --physical "$PHYS/$c.json" | tr '\n' ' ')"; done
done
for m in beschussamt deva; do echo "measured $m (shipped): $("$EVAL" data/measurements/${m}_loads.csv --profile physical --physical "$PHYS/base.json" | tr '\n' ' ')"; done
cat <<'TXT'

Adopt by hand: per powder compare its own loads (analysis/compare_physical_published.py on a one-powder CSV, --min-samples 1 --drop-inconsistent 0, shipped vs candidate;
the adoption rule of 2026-10-23: its velocity rms falls by >= 0.1 point, pressures not worse, and the measured loads (458 SOCOM N120, DEVA N340) do not get worse).
Write the chosen entries of <work dir>/phys/ms.json into data/physical/propellants.json (json.dumps(indent=1) reproduces the file's format exactly, no trailing newline),
then build, run the tests and tools/update_accuracy.py.
TXT
