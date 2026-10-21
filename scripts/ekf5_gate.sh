#!/usr/bin/env bash
# G95 runner: --imuFilter=ekf5, the 5-state filter on DC-1's output (TECH_DEBT N.28).
# What each run is for, and what a firing looks like on disk, is in scripts/ekf5_gate.py.
#
# usage: bash scripts/ekf5_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test; without one, a mktemp dir.
#
# on      the preset's endgame, seed 7, 900 s, DC-1 at 0.5 deg/s, ekf5 -- where DC1-BIAS measured
#         DC-1's filter failing; the filter must be wired and must learn the bias and heading
# def/cv  the same at 300 s with --imuFilter absent and given as cv: must be byte-identical
# r_*     refusals: ekf5 without --imuPredict, an unknown value, and --rangeBiasStates (CV-only)
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_PRE_DD19: the preset this gate was written against (DESIGN_DECISIONS 19-24)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g95.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
# PIN (DESIGN_DECISIONS 18, 2026-10-20): this gate's thresholds were set before the preset adopted the receiver
# corrections; the two flags restore the receiver it was written against, as G63/G87/G89 pin theirs.
P="--specProfile=true $PIN_PRE_DD19 --seed=7 --trace=true --boxHalf=750 --finderMode=1 --arms=ca_tbl_full --imuGyroBiasSigmaDps=0.5 --uwbSensitivityDbm=-104 --iqOverloadOnly=false"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $P --duration=900 --imuFilter=ekf5 &
run def $P --duration=300 &
run cv $P --duration=300 --imuFilter=cv &
wait
run r_noimu --arms=ca_tbl_full --seed=1 --duration=10 --imuFilter=ekf5
run r_bogus --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --imuFilter=bogus
run r_rbs --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --imuFilter=ekf5 --rangeBiasStates=true

python3 scripts/ekf5_gate.py "$D"
