#!/usr/bin/env bash
# G98 runner: --acquireHandoverCurvature, the particle cloud hands over only once every range is linearisable
# (TECH_DEBT N.58). What each run is for, and what a firing looks like on disk, is in scripts/acq_curvature_gate.py.
#
# usage: bash scripts/acq_curvature_gate.sh [<workdir>]
#
# on/base  the preset at 5 km, seed 5, unknown start, 600 s, ca_tbl_full, with and without the guard
# def/zero the same at 300 s with the flag absent and given false: must be byte-identical
# r_nopf   refusal: the guard without --acquireParticles
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g98.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
P="--specProfile=true --seed=5 --trace=true --unknownStart=true --arms=ca_tbl_full"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $P --duration=600 --acquireHandoverCurvature=true &
run base $P --duration=600 &
run def $P --duration=300 &
run zero $P --duration=300 --acquireHandoverCurvature=false &
wait
run r_nopf --arms=ca_tbl_full --seed=1 --duration=10 --acquireHandoverCurvature=true

python3 scripts/acq_curvature_gate.py "$D"
