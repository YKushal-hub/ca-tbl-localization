#!/usr/bin/env bash
# G61 runner: the LR1121 radio basis (docs/SPEC_CONFORMANCE_PLAN.md SY-2, --radioBasis).
# What each run is for, and what a firing looks like on disk, is in
# scripts/radio_basis_gate.py, which reads them.
#
# usage: bash scripts/radio_basis_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Pairs that differ only in --radioBasis: the default configuration (all six arms, NC1's
# flags), five measured subjects (full arm), and G16's grant-gossip regime (full arm); then
# the preset, and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g61.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
DEF="--seed=7 --duration=900 --trace=true"
SUB="--arms=ca_tbl_full --seed=7 --duration=600 --trace=true --nTags=5 --measuredSubjects=5"
GOS="--arms=ca_tbl_full --seed=7 --duration=900 --trace=false --nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 --sendHandshakeFrames=true --finderDigestPeriodS=20 --boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180 --anchorGrantGossipS=5"
run sx $DEF --radioBasis=sx &
run lr $DEF --radioBasis=lr1121 &
run sub_sx $SUB &
run sub_lr $SUB --radioBasis=lr1121 &
wait
run gos_sx $GOS &
run gos_lr $GOS --radioBasis=lr1121 &
run pre --arms=ca_tbl_full --seed=7 --duration=300 --trace=false --specProfile=true &
wait
run r_bad --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --radioBasis=lr1110

python3 scripts/radio_basis_gate.py "$D"
