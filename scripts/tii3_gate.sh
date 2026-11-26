#!/usr/bin/env bash
# G170 runner: TII-3 (--benefitNisInflate, --benefitHeartbeat868N, --benefitSkipNisMax). Checks in scripts/tii3_gate.py.
# base the endgame preset, seed 7, 600 s, fixed_threshold + ca_tbl_full, the TII-1 rule on (lambda 200, skip 868)
# off  base with the three TII-3 flags explicitly off: byte-identical to base
# b01  base at lambda 0.0005, where 868 MHz decisions sit near the threshold; f1 b01 + --benefitNisInflate=true;  f2 base + --benefitHeartbeat868N=3;  f4 base + --benefitSkipNisMax=6.63
set -u
cd "$(dirname "$0")/.." || exit 2
# DD 27 (2026-11-20): this gate was written against DD 26's deployment; pin it back (scripts/preset_pins.sh).
# shellcheck source=/dev/null
. scripts/preset_pins.sh
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g170.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true $PIN_PRE_DD65 $PIN_PRE_DD27 --seed=7 --duration=600 --boxHalf=750 --finderMode=1 --trace=true --arms=fixed_threshold,ca_tbl_full --benefitSkip868=true"
L="--benefitLambda=200"; L1="--benefitLambda=0.0005"
# shellcheck disable=SC2086
run base $E $L &
# shellcheck disable=SC2086
run off $E $L --benefitNisInflate=false --benefitHeartbeat868N=0 --benefitSkipNisMax=0 &
# shellcheck disable=SC2086
run b01 $E $L1 &
# shellcheck disable=SC2086
run f1 $E $L1 --benefitNisInflate=true &
# shellcheck disable=SC2086
run f2 $E $L --benefitHeartbeat868N=3 &
# shellcheck disable=SC2086
run f4 $E $L --benefitSkipNisMax=6.63 &
wait
python3 scripts/tii3_gate.py "$D"
