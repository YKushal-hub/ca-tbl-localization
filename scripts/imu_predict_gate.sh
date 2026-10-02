#!/usr/bin/env bash
# G40 runner: the filter's prediction takes a synthetic-IMU control input
# (docs/SPEC_CONFORMANCE_PLAN.md DC-1, --imuPredict). What each run is for, and what a
# firing looks like on disk, is in scripts/imu_predict_gate.py, which reads them.
#
# usage: bash scripts/imu_predict_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Null control NC3's regime (3 gateways at boxHalf 2000, where the filter coasts on about two
# cycles in three), ca_tbl_full: the default, --imuPredict=false and =true at seed 7, 1200 s;
# and --imuPredict at seeds 1-20, 600 s, for the consistency test across runs.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g40.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --trace=true --nGateways=3 --boxHalf=2000"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run def $B --seed=7 --duration=1200 &
run off $B --seed=7 --duration=1200 --imuPredict=false &
run on $B --seed=7 --duration=1200 --imuPredict=true &
wait
for s in $(seq 1 20); do
    run "s$s" $B --seed=$s --duration=600 --imuPredict=true &
    [ $((s % 10)) -eq 0 ] && wait
done
wait

python3 scripts/imu_predict_gate.py "$D"
