#!/usr/bin/env bash
# G42 runner: Case B resolves against the IMU-propagated prior
# (docs/SPEC_CONFORMANCE_PLAN.md DC-2). What each run is for, and what a firing looks like on
# disk, is in scripts/case_b_imu_gate.py, which reads them.
#
# usage: bash scripts/case_b_imu_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Null control NC3's regime (3 gateways at boxHalf 2000) with four finders converging on the
# walker, so two-range sets with a finder in them -- Case B -- are common (6-15 per seed
# under --imuPredict), ca_tbl_full, seeds 1-4, 1200 s: with --sparseInStep --imuPredict, and
# with --sparseInStep alone.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g42.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --duration=1200 --trace=true --nGateways=3 --boxHalf=2000 --nFinders=4 --finderMode=1 --uwbCoverageIncludesFinders=true --sparseInStep=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
for s in 1 2 3 4; do
    run "imu$s" $B --seed=$s --imuPredict=true &
    run "cv$s" $B --seed=$s &
done
wait

python3 scripts/case_b_imu_gate.py "$D"
