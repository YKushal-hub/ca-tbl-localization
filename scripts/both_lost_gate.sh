#!/usr/bin/env bash
# G83 runner: both colliding frames are judged (--collisionBothLost; TECH_DEBT N.22).
# What each run is for, and what a firing looks like on disk, is in
# scripts/both_lost_gate.py, which reads them.
#
# usage: bash scripts/both_lost_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G74's regime (seed 7, 900 s, defaults, the event log): the flag on, and off (the control).
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g83.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --vizEvents=true"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --collisionBothLost=true &
run off $B &
wait

python3 scripts/both_lost_gate.py "$D"
