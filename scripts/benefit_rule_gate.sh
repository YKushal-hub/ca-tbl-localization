#!/usr/bin/env bash
# G169 runner: --benefitLambda / --benefitSkip868 (TII-1, report/PREREG_PARETO1.md). Checks in scripts/benefit_rule_gate.py.
# abs  the preset in the endgame, seed 7, 600 s, fixed_threshold and ca_tbl_full, flags absent
# off  the same with --benefitLambda=0 --benefitSkip868=false: byte-identical to abs
# on   the same with --benefitLambda=200 --benefitSkip868=true: only fixed_threshold changes
# min3 as on, plus --benefitSkip868Min3=true (TII-2): announced, and skips fewer 868 MHz slots than on, but some
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_PRE_DD65: the preset this gate was written against (DESIGN_DECISIONS 65)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g169.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true $PIN_PRE_DD65 --seed=7 --duration=600 --boxHalf=750 --finderMode=1 --trace=true --arms=fixed_threshold,ca_tbl_full"
# shellcheck disable=SC2086
run abs $E &
# shellcheck disable=SC2086
run off $E --benefitLambda=0 --benefitSkip868=false &
# shellcheck disable=SC2086
run on $E --benefitLambda=200 --benefitSkip868=true &
# shellcheck disable=SC2086
run min3 $E --benefitLambda=200 --benefitSkip868=true --benefitSkip868Min3=true &
wait
python3 scripts/benefit_rule_gate.py "$D"
