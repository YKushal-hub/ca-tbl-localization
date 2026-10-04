#!/usr/bin/env bash
# G44 runner: the IMU witness uses the synthetic IMU against the raw radio fix
# (docs/SPEC_CONFORMANCE_PLAN.md DC-9, --imuWitnessSource=model). What each run is for, and
# what a firing looks like on disk, is in scripts/imu_witness_gate.py, which reads them.
#
# usage: bash scripts/imu_witness_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# NC9's regime (four finders converging on the walker, so the policy wants to escalate),
# ca_tbl_full, seed 1, 600 s, with --imuPredict --imuCrossCheck: the default witness,
# --imuWitnessSource=shs, and model -- once with the evidence gate off (NC9's setting) and
# once on (from neighbours), where refusals can be absorbed. Plus two refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g44.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=1 --duration=600 --trace=true --nFinders=4 --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true --nlosClassifier=true --imuPredict=true --imuCrossCheck=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run def $B --escalationRequiresEvidence=false &
run shs $B --escalationRequiresEvidence=false --imuWitnessSource=shs &
run model $B --escalationRequiresEvidence=false --imuWitnessSource=model &
run model_ev $B --escalationRequiresEvidence=true --uwbEvidenceFromNeighbours=true --imuWitnessSource=model &
wait
run r_noimu --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --imuCrossCheck=true --imuWitnessSource=model
run r_bad --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --imuWitnessSource=gps

python3 scripts/imu_witness_gate.py "$D"
