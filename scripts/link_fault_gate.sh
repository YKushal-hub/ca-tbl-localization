#!/usr/bin/env bash
# G37 runner: the transmit-power-drop and antenna-loss fault classes
# (docs/SPEC_CONFORMANCE_PLAN.md SY-5). What each run is for, and what a firing looks like
# on disk, is in scripts/link_fault_gate.py, which reads them.
#
# usage: bash scripts/link_fault_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Four 900 s runs of ca_tbl_full at seed 7 with the fault window over 30% of the run (it
# opens at 225 s): no fault, both magnitudes given as 0, a 10 dB transmit-power drop, and
# a 10 dB antenna loss. Plus two refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g37.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --injectBiasFraction=0.3"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run def $B &
run zero $B --injectTxPowerDropDb=0 --injectAntennaLossDb=0 &
run drop $B --injectTxPowerDropDb=10 &
run loss $B --injectAntennaLossDb=10 &
wait
run r_nowin --arms=ca_tbl_full --seed=7 --duration=30 --trace=false --injectTxPowerDropDb=10
run r_neg --arms=ca_tbl_full --seed=7 --duration=30 --trace=false --injectBiasFraction=0.3 --injectAntennaLossDb=-1

python3 scripts/link_fault_gate.py "$D"
