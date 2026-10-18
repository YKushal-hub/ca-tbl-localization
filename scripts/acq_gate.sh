#!/usr/bin/env bash
# G132 runner: --acquireMinAnchors (S2-C), under --unknownStart. What each run is for, and what a
# firing looks like on disk, is in scripts/acq_gate.py, which reads them.
#
# usage: bash scripts/acq_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test; without one they go to a mktemp
#   directory removed on exit.
#
# The preset at 5 km under --unknownStart, ca_tbl_full, seed 7, 600 s.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g132.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--specProfile=true --arms=ca_tbl_full --seed=7 --duration=600 --trace=true --unknownStart=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run base $B &
run zero $B --acquireMinAnchors=0 &
run acq3 $B --acquireMinAnchors=3 &
wait
python3 scripts/acq_gate.py "$D"
