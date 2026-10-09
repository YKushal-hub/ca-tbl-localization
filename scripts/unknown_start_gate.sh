#!/usr/bin/env bash
# G113 runner: --unknownStartSigmaM (TECH_DEBT N.39). What each run is for, and what a firing
# looks like on disk, is in scripts/unknown_start_gate.py, which reads them.
#
# usage: bash scripts/unknown_start_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# The preset at 5 km, arms ca_tbl_full and static_2p4, seed 1, 300 s: the flag absent, at 0,
# and at 100.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g113.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
A="--specProfile=true --arms=ca_tbl_full,static_2p4 --seed=1 --duration=300 --trace=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run base $A &
run off $A --unknownStartSigmaM=0 &
run on $A --unknownStartSigmaM=100 &
wait

python3 scripts/unknown_start_gate.py "$D"
