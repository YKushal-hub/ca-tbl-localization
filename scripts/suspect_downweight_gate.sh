#!/usr/bin/env bash
# G29 runner: a SUSPECT fix is downweighted, not rejected (docs/SPEC_CONFORMANCE_PLAN.md
# DC-7). What each run is for, and what a firing looks like on disk, is in
# scripts/suspect_downweight_gate.py, which reads them.
#
# usage: bash scripts/suspect_downweight_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Six 900 s runs at seed 7 under propagation NLOS -- the configuration of null control
# NC8, where the integrity check flags 131 fixes at defaults -- and four refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g29.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
BASE="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --nlosSource=propagation"
SHORT="--arms=ca_tbl_full --seed=7 --duration=30 --trace=false"
DW="--suspectRecovery=downweight"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run none $BASE --suspectRecovery=none
run dw $BASE $DW
run dw11 $BASE $DW --suspectInflateFactor=1 --suspectPcsFactor=1
run b1 $BASE --suspectRecovery=bounded --suspectRecoveryN=1
run dw1h $BASE $DW --suspectInflateFactor=1 --suspectPcsFactor=0.5
run dw4_1 $BASE $DW --suspectInflateFactor=4 --suspectPcsFactor=1
run r_nodw $SHORT --suspectPcsFactor=0.5
run r_zero $SHORT $DW --suspectPcsFactor=0
run r_big $SHORT $DW --suspectPcsFactor=1.5
run r_mode $SHORT --suspectRecovery=downweighted

python3 scripts/suspect_downweight_gate.py "$D"
