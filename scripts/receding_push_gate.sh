#!/usr/bin/env bash
# G75 runner: L.1.5's RECEDING eager push is counted (--reportRecedingPushes; TECH_DEBT
# M.9). What each run is for, and what a firing looks like on disk, is in
# scripts/receding_push_gate.py, which reads them.
#
# usage: bash scripts/receding_push_gate.sh [<workdir>]
#
# Seed 7, 900 s, --mobilityFromRange (the only way the RECEDING branch runs), triggered
# updates off (so every TRIGGERED_UPDATE tag 0 originates is a RECEDING push), the event log:
# the report on and off, and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g75.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --vizEvents=true --mobilityFromRange=true --triggeredUpdates=false"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --reportRecedingPushes=true &
run off $B &
wait
run r_nomob --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --reportRecedingPushes=true

python3 scripts/receding_push_gate.py "$D"
