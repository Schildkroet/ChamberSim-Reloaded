#!/bin/bash
# The refit of the physical burn values in one run: all published load files -> one load file per class -> checked and cleaned -> refit -> judged.
#
#   analysis/refit.sh <csr_eval> <work dir> [steps]       steps (one quoted argument): build groups ba multistart compare   (default: build groups ba compare)
#
# <csr_eval> is a Release build of tools/csr_eval.cpp, <work dir> a scratch directory outside the tree (a venv with numpy + scipy: PYTHON=...). Run from the repository root.
# The data/ tree is not changed: candidates are in <work dir>/phys, the loads in <work dir>/loads.
#
# build      scripts/data/loaddata/build_loads.py --simulate: every sample file of sources/samples -> loads_rifle.csv, loads_pistol.csv, loads_shot.csv (loads kept once,
#            the class from the cartridge), the invalid loads dropped (ranges, energy, pressure, conflicting duplicates, simulated pressure > 45 % off), removed.csv, report.txt.
# groups     P = project powders with >= 10 measured pressures in the clean loads, Q = powders with >= 20 loads and fewer pressures.
# ba         fit_physical_powders.py --fit ba --keep-form: Ba of P from the pressures, Ba of Q with the C.I.P. limit - 50 bar as the pressure of the maximum loads.
# multistart fit_physical_multistart.py on Q (8 start shapes) without the grain-geometry powders: slow (about 40 minutes).
# compare    the candidates against the shipped values (rifle / pistol), per powder (adopt_report.py: velocity rms of its own loads -0.1 point or more, pressure not worse),
#            and on the measured loads of data/measurements. <work dir>/phys has copies of calibers.json and primers.json (csr_eval reads them next to --physical; without
#            them every comparison is biased) and the check "shipped file as --physical" must give zero change.
# A step only writes its own file (ba.json, ms.json): the others stay, so a step can be rerun alone. Adopted powders: <work dir>/phys/adopted_<candidate>.json.
set -euo pipefail
EVAL=$(realpath "$1"); WORK=$(realpath -m "$2"); STEPS=${3:-build groups ba compare}
PY=${PYTHON:-python3}
KEEP="rws-rottweil r907,vectan tubal 3000,vectan tubal 5000,vihtavuori n570,vihtavuori n110"   # grain geometry (fit_grain_geometry.py): never refitted here
LOADS=$WORK/loads; PHYS=$WORK/phys; mkdir -p "$LOADS" "$PHYS"
cp data/physical/calibers.json data/physical/primers.json "$PHYS/"; cp data/physical/propellants.json "$PHYS/base.json"
has() { [[ " $STEPS " == *" $1 "* ]]; }
FILES="$LOADS/loads_rifle.csv $LOADS/loads_pistol.csv"
if has build; then
  $PY scripts/data/loaddata/build_loads.py --python "$PY" --simulate "$EVAL" --out "$LOADS"
fi
if has groups; then
  $PY - "$WORK" "$LOADS" <<'PYEOF'
import csv, collections, sys
sys.path.insert(0, "scripts/data")
import recordfiles
rec = {(r["mname"] + " " + r["pname"]).lower(): r for _, r in recordfiles.read("data/propellants")}
byname = collections.defaultdict(list)
for k, r in rec.items(): byname[r["pname"].lower()].append(k)
n = collections.Counter(); p = collections.Counter()
for cls in ("rifle", "pistol"):
    for r in csv.DictReader(open(f"{sys.argv[2]}/loads_{cls}.csv")):
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
if has ba; then
  cp "$PHYS/base.json" "$PHYS/ba.json"
  $PY analysis/fit_physical_powders.py "$EVAL" $FILES --fit ba --keep-form --start "$PHYS/base.json" --out "$PHYS/ba.json" --merge --only "$(cat $WORK/P.txt)" > "$WORK/fitP.log"
  $PY analysis/fit_physical_powders.py "$EVAL" $FILES --fit ba --keep-form --start "$PHYS/ba.json" --out "$PHYS/ba.json" --merge --limit-pressure --limit-margin 50 \
      --min-samples 20 --only "$(cat $WORK/Q.txt)" > "$WORK/fitQ.log"
fi
if has multistart; then
  $PY analysis/fit_physical_multistart.py "$EVAL" $FILES --powders "$(cat $WORK/Q.txt)" --keep "$KEEP" --start "$PHYS/base.json" --out "$PHYS/ms.json" --work "$WORK/ms" | tee "$WORK/ms.log"
fi
if has compare; then
  echo "== shipped file as --physical (must be zero change)"
  $PY analysis/compare_physical_published.py "$EVAL" $FILES --projectiles data/projectiles --eval-arg=--physical="$PHYS/base.json" --baseline "$EVAL" | grep -E "^(Rifle|Pistol)" | tail -2
  for c in ba ms; do
    [ -f "$PHYS/$c.json" ] || continue
    echo "== $c.json against the shipped values: velocity / pressure rms, bias"
    $PY analysis/compare_physical_published.py "$EVAL" $FILES --projectiles data/projectiles --eval-arg=--physical="$PHYS/$c.json" --baseline "$EVAL" \
        --dump "$WORK/dump_$c.csv" | grep -E "^(Rifle|Pistol)" | tail -2
    $PY analysis/adopt_report.py "$WORK/dump_$c.csv" "$PHYS/base.json" "$PHYS/$c.json" --out "$PHYS/adopted_$c.json"
    for m in beschussamt deva; do echo "measured $m ($c): $("$EVAL" data/measurements/${m}_loads.csv --profile physical --physical "$PHYS/adopted_$c.json" | tr '\n' ' ')"; done
  done
  for m in beschussamt deva; do echo "measured $m (shipped): $("$EVAL" data/measurements/${m}_loads.csv --profile physical --physical "$PHYS/base.json" | tr '\n' ' ')"; done
  cat <<'TXT'

Adopt by hand after the measured loads (458 SOCOM N120, DEVA N340) have not got worse: write the entries of <work dir>/phys/adopted_<candidate>.json into data/physical/propellants.json
(json.dumps(indent=1) reproduces the file's format exactly, no trailing newline), then build, run the tests and tools/update_accuracy.py.
TXT
fi
