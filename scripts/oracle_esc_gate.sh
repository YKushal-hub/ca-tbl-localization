#!/usr/bin/env bash
# G167 runner: --oracleTrigger (ORACLE-ESC, report/PREREG_ORACLE_ESC.md). Checks in scripts/oracle_esc_gate.py.
# abs  the preset at 5 km, seed 7, 600 s, fixed_threshold and ca_tbl_full, flag absent, --fixedVarThreshold=12
# off  the same with --oracleTrigger=false: byte-identical to abs
# on   the same with --oracleTrigger=true: only fixed_threshold changes, and it de-escalates on the true error
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g167.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true --seed=7 --duration=600 --trace=true --arms=fixed_threshold,ca_tbl_full --fixedVarThreshold=12"
# shellcheck disable=SC2086
run abs $E &
# shellcheck disable=SC2086
run off $E --oracleTrigger=false &
# shellcheck disable=SC2086
run on $E --oracleTrigger=true &
wait
python3 scripts/oracle_esc_gate.py "$D"
