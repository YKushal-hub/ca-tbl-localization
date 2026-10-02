#!/usr/bin/env bash
# G34 runner: the spec's triggered update and last gasp (docs/SPEC_CONFORMANCE_PLAN.md
# MC-2, --triggeredUpdates). What each run is for, and what a firing looks like on disk, is
# in scripts/triggered_updates_gate.py, which reads them.
#
# usage: bash scripts/triggered_updates_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Three 900 s runs at seed 7 with a battery budget the node spends mid-run, so both
# triggers can fire: the default, --triggeredUpdates=false and =true. Plus one refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g34.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --vizEvents=true --batteryDepleteJ=@J@"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
B=${B/@J@/${G34_BATTERY_J:-3}}
run def $B &
run off $B --triggeredUpdates=false &
run on $B --triggeredUpdates=true &
wait
run r_k --arms=ca_tbl_full --seed=7 --duration=30 --trace=false --triggeredUpdateK=2

python3 scripts/triggered_updates_gate.py "$D"
