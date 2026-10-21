#!/usr/bin/env bash
# G96 runner: --ekf5Robust=8, CONSIDER -- fixes of fewer than 3 ranges are Schmidt updates of
# the 5-state filter (TECH_DEBT N.33). EKF5-CONSIDER rejected the mode; this gate keeps its wiring
# honest, it does not claim the mode helps. What each run is for, and what a firing looks like on
# disk, is in scripts/ekf5_consider_gate.py.
#
# usage: bash scripts/ekf5_consider_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test; without one, a mktemp dir.
#
# on/base  the preset at 5 km, seed 19, 600 s, DC-1 at 0.15 deg/s, ekf5 -- a cell where Case D fires
#          under the current preset (27 single-range updates); base is the same with mode 0
# def/zero the same at 300 s with --ekf5Robust absent and given as 0: must be byte-identical
# r_*      refusals: the mode without ekf5, and a mode out of range
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_PRE_DD19: the preset this gate was written against (DESIGN_DECISIONS 19-24)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g96.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
P="--specProfile=true $PIN_PRE_DD19 --seed=19 --trace=true --reportAirtime=true --arms=ca_tbl_full --imuGyroBiasSigmaDps=0.15 --imuFilter=ekf5"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $P --duration=600 --ekf5Robust=8 &
run base $P --duration=600 &
run def $P --duration=300 &
run zero $P --duration=300 --ekf5Robust=0 &
wait
run r_cv --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --ekf5Robust=8
run r_range --arms=ca_tbl_full --seed=1 --duration=10 --imuPredict=true --imuFilter=ekf5 --ekf5Robust=16

python3 scripts/ekf5_consider_gate.py "$D"
