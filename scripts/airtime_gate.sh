#!/usr/bin/env bash
# G86 runner: airtime per tier and message type (--reportAirtime; MAC sect 2, Transport sect 7).
# What each run is for, and what a firing looks like on disk, is in scripts/airtime_gate.py,
# which reads them.
#
# usage: bash scripts/airtime_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# The preset, full arm, seed 7, 900 s: with the report, without it (the control), and with the
# report and --reversePath=false (responses flooded, the comparison the report serves).
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g86.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --specProfile=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --reportAirtime=true &
run off $B &
run flood $B --reportAirtime=true --reversePath=false &
wait

python3 scripts/airtime_gate.py "$D"
