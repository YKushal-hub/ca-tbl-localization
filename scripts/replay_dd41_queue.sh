#!/usr/bin/env bash
# REPLAY-DD41 queue (report/PREREG_REPLAY_DD41.md), after COLLAPSE-8. usage: bash scripts/replay_dd41_queue.sh
set -u
cd "$(dirname "$0")/.." || exit 2
R=$HOME/catbl_work/campaigns
# COLLAPSE-8 is complete; its runner skips finished cells.
[ -e "$R/replay41/realistic5" ] || COLLAPSE8_JOBS=8 bash scripts/collapse8_run.sh "$R/collapse8" > "$R/collapse8_runner.log" 2>&1
mkdir -p "$R/replay41"
for c in realistic5 realistic6 pareto1_dd41 pareto2_dd41 realistic4 realistic3 realistic2 realistic1; do
    grep -q " done" "$R/replay41/$c/RUN.md" 2>/dev/null && continue
    free=$(df --output=avail -BG "$R" | tail -1 | tr -dc 0-9)
    winfree=$(df --output=avail -BG /mnt/c | tail -1 | tr -dc 0-9)
    [ "$free" -ge 30 ] && [ "$winfree" -ge 6 ] || { echo "$(date -u +%FT%TZ) STOP before $c: $free GB free, C: $winfree GB" >> "$R/replay41/QUEUE.md"; exit 1; }
    ( while [ ! -e "$R/replay41/$c/build.sha256" ]; do sleep 5; done; bash "$R/prune_runs.sh" "$R/replay41/$c" --loop ) & P=$!
    U=$(echo "${c%_dd41}" | tr a-z A-Z)
    env "${U}_JOBS=8" bash "scripts/${c}_run.sh" "$R/replay41/$c" > "$R/replay41/${c}_runner.log" 2>&1
    rc=$?
    kill "$P" 2>/dev/null; wait "$P" 2>/dev/null
    bash "$R/prune_runs.sh" "$R/replay41/$c"
    echo "$(date -u +%FT%TZ) $c rc=$rc" >> "$R/replay41/QUEUE.md"
done
