#!/usr/bin/env bash
# G131 runner: --nlosRSource (S2-C). What each run is for, and what a firing looks like on disk, is
# in scripts/nlos_r_gate.py, which reads them.
#
# usage: bash scripts/nlos_r_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test; without one they go to a mktemp
#   directory removed on exit.
#
# The preset (classifier on, oracle NLOS tag) at 5 km, ca_tbl_full, seed 7, 600 s.
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_DD19_22: the preset this gate was written against (DESIGN_DECISIONS 19-22)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g131.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--specProfile=true $PIN_DD19_22 --arms=ca_tbl_full --seed=7 --duration=600 --trace=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run base $B &
run oper $B --nlosRSource=operative &
run none $B --nlosRSource=none &
run det $B --nlosRSource=detector &
wait
python3 scripts/nlos_r_gate.py "$D"
