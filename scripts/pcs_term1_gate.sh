#!/usr/bin/env bash
# G90 runner: register row 17, PCS term 1 as the spec's literal 1/trace(Sigma)
# (--pcsTerm1Inverse). What each run is for is in scripts/pcs_term1_gate.py, which reads them.
#
# usage: bash scripts/pcs_term1_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test; without one they go to a
#   mktemp directory removed on exit.
#
# ca_tbl_full at the default deployment (legacy PCS, so pcs = t1 + t2 + t3 exactly, and no
# SUSPECT downweight), seed 7, 600 s: the literal term on, and off (the control); and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g90.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=600 --trace=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --pcsTerm1Inverse=true &
run off $B &
wait
run r_v2 --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --pcsForm=v2 --pcsTerm1Inverse=true
python3 scripts/pcs_term1_gate.py "$D"
