#!/usr/bin/env bash
# Regenerates the reference jobs of all batches (the originals, tools/GRTRef/jobs_all, are not in the repository) and packs
# the plugin folder for the machine that runs the original app.
#   usage: make_all_jobs.sh [output folder, default /home/patrick/Vorlagen/GRTRef/GRTRef]
# Needs a Release grt_compare (GRT_COMPARE, default build/release/grt_compare: the generators set charges by the load ratio),
# the base load ~/Downloads/GordonsReloadingTool-2021.2040-NIGHTLY-linux/loads/8x57is.grtload and tools/GordonsReloadingTool.plain.db.
# Result: <out>/{grtref.py,com.grt.plugin.xml,run_parallel.sh,jobs/exp_*.grtload}. Copy the folder to <GRT app>/plugins/GRTRef.
# The published-data batches of make_jobs_vv.py are not reference runs of the original and are left out.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT="${1:-/home/patrick/Vorlagen/GRTRef/GRTRef}"
WORK="$(mktemp -d "${TMPDIR:-/tmp}/grtjobs.XXXXXX")"
cd "$HERE"
cp fast_jobs.json "$WORK/fast_jobs.json"  # make_jobs_fast.py rewrites it; the tracked copy stays as it is
trap 'cp "$WORK/fast_jobs.json" "$HERE/fast_jobs.json"; rm -rf "$WORK"' EXIT

collect() {  # collect <generator> [prefix to put after "exp_"]: run it into jobs/, move the result to $WORK/<generator>
    rm -rf jobs
    python3 "$1" > /dev/null
    mkdir -p "$WORK/$1"
    for f in jobs/exp_*.grtload; do
        b="$(basename "$f")"
        [ -n "${2:-}" ] && [[ "$b" != exp_aa_warmup.grtload && "$b" != exp_fp_* ]] && b="exp_$2${b#exp_}"
        mv "$f" "$WORK/$1/$b"
    done
    rm -rf jobs
}

for g in make_jobs.py make_jobs_cv.py make_jobs_charge.py make_jobs_ba.py make_jobs_sweeps.py make_jobs_tempcoef.py \
         make_jobs_powders.py make_jobs_calibers.py make_jobs_slow.py make_jobs_fast.py make_jobs_ladder.py; do
    collect "$g"
done
collect make_jobs_fast_8x57.py x7_  # batch 7a (8x57 base) was archived with the prefix x7_; the 9 mm batch 7 (make_jobs_fast.py) has none
mkdir -p "$WORK/jump" "$WORK/shot" "$WORK/shotgun"
python3 make_jobs_shotgun.py ../../reference/vv2026_shotgun.csv "$WORK/shotgun" > /dev/null
python3 make_jobs_jump.py "$WORK/jump" > /dev/null
python3 make_jobs_shot.py "$WORK/shot" > /dev/null

rm -rf "$OUT/jobs"
mkdir -p "$OUT/jobs"
cp --update=none "$WORK"/*/exp_*.grtload "$OUT/jobs/"  # identical names are identical files (checked: warm-up and fp_ loads)
cp grtref.py com.grt.plugin.xml run_parallel.sh "$OUT/"
echo "$(ls "$OUT/jobs" | wc -l) jobs in $OUT/jobs"
