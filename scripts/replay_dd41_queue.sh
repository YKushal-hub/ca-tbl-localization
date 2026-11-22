#!/usr/bin/env bash
# REPLAY-DD41 queue (report/PREREG_REPLAY_DD41.md), after COLLAPSE-8. usage: bash scripts/replay_dd41_queue.sh
set -u
cd "$(dirname "$0")/.." || exit 2
R=$HOME/catbl_work/campaigns
COLLAPSE8_JOBS=8 bash scripts/collapse8_run.sh "$R/collapse8" > "$R/collapse8_runner.log" 2>&1
mkdir -p "$R/replay41"
for c in realistic5 realistic6 pareto1 pareto2 realistic4 realistic3 realistic2 realistic1; do
    U=$(echo "$c" | tr a-z A-Z)
    env "${U}_JOBS=8" bash "scripts/${c}_run.sh" "$R/replay41/$c" > "$R/replay41/${c}_runner.log" 2>&1
    echo "$(date -u +%FT%TZ) $c rc=$?" >> "$R/replay41/QUEUE.md"
done
