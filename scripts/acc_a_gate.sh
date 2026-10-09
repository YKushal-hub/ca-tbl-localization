#!/usr/bin/env bash
# G100 runner: --imuQuScale, the multiplier on the IMU increment covariance handed to every filter
# (Part A, A1). What each run is for, and what a firing looks like on disk, is in
# scripts/acc_a_gate.py, which reads them.
#
# usage: bash scripts/acc_a_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test; without one they go to a mktemp
#   directory removed on exit.
#
# The preset (IMU prediction on) at 5 km, ca_tbl_full, seed 7, 600 s: no flag (the control), the
# flag at its default given explicitly, and the flag at 10.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g100.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--specProfile=true --arms=ca_tbl_full --seed=7 --duration=600 --trace=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run base $B &
run one $B --imuQuScale=1 &
run ten $B --imuQuScale=10 &
wait
python3 scripts/acc_a_gate.py "$D"
