#!/usr/bin/env bash
# G166 runner: --subjectEnergyBuckets (N.17), --caseAPreFuse (plan item 27), --anchorPairCad (3.5),
# --subjectMobilityFromRange (plan item 9, N.3), --relayRoundInSlice (M.1(b)).
# Checks in scripts/cut_items_gate.py.
# buck       the 5 km preset, seed 6, 300 s, ca_tbl_full, --subjectEnergyBuckets
# ca_off/on  the endgame preset, seed 6, 900 s, ca_tbl_full, without and with --caseAPreFuse
# apc        the 8-gateway ring preset, seed 6, 300 s, ca_tbl_full, --anchorPairCad
# mob        the 5 km preset, seed 6, 900 s, ca_tbl_full, --subjectMobilityFromRange
# rs_on      the endgame preset, seed 6, 600 s, ca_tbl_full, --relayRoundInSlice (against ca_off's first 600 s: rs_off)
# def/zero   code defaults, 300 s, the three flags absent and given their defaults: byte-identical
# r_ca, r_rs refusals: --caseAPreFuse without --sparseInStep; --relayRoundInSlice without the relays' round
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g166.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
P="--specProfile=true --seed=6 --arms=ca_tbl_full"
# shellcheck disable=SC2086
run buck $P --duration=300 --subjectEnergyBuckets=true &
# shellcheck disable=SC2086
run ca_off $P --duration=900 --boxHalf=750 --finderMode=1 &
# shellcheck disable=SC2086
run ca_on $P --duration=900 --boxHalf=750 --finderMode=1 --caseAPreFuse=true &
# shellcheck disable=SC2086
run apc $P --duration=300 --nGateways=8 --gwLayout=legacy --anchorPairCad=true &
# shellcheck disable=SC2086
run mob $P --duration=900 --subjectMobilityFromRange=true &
# shellcheck disable=SC2086
run rs_off $P --duration=600 --boxHalf=750 --finderMode=1 &
# shellcheck disable=SC2086
run rs_on $P --duration=600 --boxHalf=750 --finderMode=1 --relayRoundInSlice=true &
run def --seed=7 --trace=true --duration=300 --arms=ca_tbl_full &
run zero --seed=7 --trace=true --duration=300 --arms=ca_tbl_full --subjectEnergyBuckets=false --caseAPreFuse=false --anchorPairCad=false --subjectMobilityFromRange=false --relayRoundInSlice=false &
wait
run r_ca --arms=ca_tbl_full --seed=1 --duration=10 --caseAPreFuse=true
run r_rs --arms=ca_tbl_full --seed=1 --duration=10 --relayRoundInSlice=true
python3 scripts/cut_items_gate.py "$D"
