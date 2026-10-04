#!/usr/bin/env bash
# G85 runner: the fuse narrowed to a short floor (--fuseFloorShort; TECH_DEBT N.14, FUSE-02).
# What each run is for, and what a firing looks like on disk, is in
# scripts/fuse_short_gate.py, which reads them.
#
# usage: bash scripts/fuse_short_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G63's regime (the preset, full arm, seed 7, 900 s): the endgame and 5 km with the narrowed
# fuse, the endgame with the full fuse (the control), and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g85.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --specProfile=true --fuseFloorActive=true"
END="--boxHalf=750 --finderMode=1"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run end $B $END --fuseFloorShort=true &
run main $B --fuseFloorShort=true &
run full $B $END &
wait
run r_nofuse --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --fuseFloorShort=true

python3 scripts/fuse_short_gate.py "$D"
