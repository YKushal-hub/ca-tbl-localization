#!/usr/bin/env bash
# G159 runner: --pcsByteScale (TECH_DEBT N.7), the scale of the PCS byte a table entry carries.
# What each run is for, and what a firing looks like on disk, is in scripts/pcs_byte_gate.py.
#
# on/base  the preset's endgame, seed 7, 600 s, ca_tbl_full, with --pcsByteScale=4 and without
# def/zero the same at 300 s with the flag absent and given 1: must be byte-identical
# r_*      refusals: a scale below 1, and above 64
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g159.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
P="--specProfile=true --seed=7 --boxHalf=750 --finderMode=1 --arms=ca_tbl_full"
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
# shellcheck disable=SC2086
run on $P --duration=600 --pcsByteScale=4 &
# shellcheck disable=SC2086
run base $P --duration=600 &
# shellcheck disable=SC2086
run def $P --duration=300 &
# shellcheck disable=SC2086
run zero $P --duration=300 --pcsByteScale=1 &
wait
run r_low --arms=ca_tbl_full --seed=1 --duration=10 --pcsByteScale=0.5
run r_high --arms=ca_tbl_full --seed=1 --duration=10 --pcsByteScale=100
python3 scripts/pcs_byte_gate.py "$D"
