#!/usr/bin/env bash
# G168 runner: --tdoaToaScale (TDOA-TIMING, report/PREREG_TDOA_TIMING.md). Checks in scripts/tdoa_timing_gate.py.
# abs  the preset at 5 km, seed 7, 600 s, static_868, flag absent
# one  the same with --tdoaToaScale=1: byte-identical to abs
# t0   the same with --tdoaPpsSigmaNs=0 --tdoaToaScale=0.01: tier-1 TDoA error collapses
# r0   --tdoaToaScale=0: refused
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g168.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true --seed=7 --duration=600 --trace=true --arms=static_868"
# shellcheck disable=SC2086
run abs $E &
# shellcheck disable=SC2086
run one $E --tdoaToaScale=1 &
# shellcheck disable=SC2086
run t0 $E --tdoaPpsSigmaNs=0 --tdoaToaScale=0.01 &
wait
run r0 --seed=1 --duration=10 --arms=static_868 --tdoaToaScale=0
python3 scripts/tdoa_timing_gate.py "$D"
