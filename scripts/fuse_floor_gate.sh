#!/usr/bin/env bash
# G63 runner: the active tier fused with the floor (TECH_DEBT N.14, --fuseFloorActive).
# What each run is for, and what a firing looks like on disk, is in
# scripts/fuse_floor_gate.py, which reads them.
#
# usage: bash scripts/fuse_floor_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# The preset's endgame (750 m box, finders converging: where a node reaches UWB with one or
# two ranges and the floor solves), full arm, seed 7, 900 s: the flag on and off; the
# preset at 5 km with the flag on (where the floor itself is short); and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g63.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --specProfile=true"
END="--boxHalf=750 --finderMode=1"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run end $B $END --fuseFloorActive=true &
run off $B $END &
run main $B --fuseFloorActive=true &
wait
run r_nostep --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --fuseFloorActive=true

python3 scripts/fuse_floor_gate.py "$D"
