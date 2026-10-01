#!/usr/bin/env bash
# Runs the reference jobs with several copies of the GRT app at the same time (on the machine that has the app).
#   usage: run_parallel.sh <GRT app folder> [instances=4]
# The app folder must already contain plugins/GRTRef (the whole GRTRef folder, with jobs/).
# Each instance works on its own copy of the app folder and on its own share of the jobs; at the end the
# results are merged into <app folder>/plugins/GRTRef/results, as if one instance had run everything.
set -euo pipefail
APP="$(cd "${1:?usage: $0 <GRT app folder> [instances]}" && pwd)"
N="${2:-4}"
PLUG="$APP/plugins/GRTRef"
[ -d "$PLUG/jobs" ] || { echo "no $PLUG/jobs"; exit 1; }
mapfile -t JOBS < <(ls "$PLUG/jobs"/*.grtload)
echo "${#JOBS[@]} jobs, $N instances"
[ "${#JOBS[@]}" -lt "$N" ] && N="${#JOBS[@]}"

WORK="$(mktemp -d "${TMPDIR:-/tmp}/grtref_par.XXXXXX")"
pids=()
for ((k = 0; k < N; k++)); do
    copy="$WORK/app$k"
    cp -r "$APP" "$copy"                                   # own settings, database and plugin folder per instance
    rm -rf "$copy/plugins/GRTRef/results" "$copy/plugins/GRTRef/grtref.log" "$copy/plugins/GRTRef/jobs"
    mkdir -p "$copy/plugins/GRTRef/jobs"
    for ((i = k; i < ${#JOBS[@]}; i += N)); do cp "${JOBS[$i]}" "$copy/plugins/GRTRef/jobs/"; done
    echo "instance $k: $(ls "$copy/plugins/GRTRef/jobs" | wc -l) jobs"
    (cd "$copy" && ./GordonsReloadingTool >/dev/null 2>&1) &
    pids+=($!)
    sleep 5                                                # let the instances start one after the other
done

# wait until every instance has written its DONE file (at most 2 hours)
for ((t = 0; t < 7200; t += 5)); do
    done_count=0
    for ((k = 0; k < N; k++)); do [ -f "$WORK/app$k/plugins/GRTRef/results/DONE" ] && done_count=$((done_count + 1)); done
    [ "$done_count" -eq "$N" ] && break
    sleep 5
done
[ "$done_count" -eq "$N" ] || echo "WARNING: only $done_count of $N instances finished"

rm -rf "$PLUG/results" "$PLUG/grtref.log"
mkdir -p "$PLUG/results"
ok=0; failed=0
for ((k = 0; k < N; k++)); do
    r="$WORK/app$k/plugins/GRTRef"
    [ -d "$r/results" ] && find "$r/results" -name '*.json' -exec cp {} "$PLUG/results/" \;
    [ -f "$r/grtref.log" ] && { echo "=== instance $k ==="; cat "$r/grtref.log"; } >> "$PLUG/grtref.log"
    [ -f "$r/results/DONE" ] && read -r _ _ j o f < <(sed -E 's/.* jobs=([0-9]+) ok=([0-9]+) failed=([0-9]+).*/x x \1 \2 \3/' "$r/results/DONE") \
        && { ok=$((ok + o)); failed=$((failed + f)); }
done
echo "$(date '+%F %T') jobs=${#JOBS[@]} ok=$ok failed=$failed (merged from $N instances)" > "$PLUG/results/DONE"
for pid in "${pids[@]}"; do kill "$pid" 2>/dev/null || true; done
rm -rf "$WORK"
cat "$PLUG/results/DONE"
