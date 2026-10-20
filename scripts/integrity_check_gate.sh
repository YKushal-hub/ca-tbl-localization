#!/usr/bin/env bash
# G163 runner: --integrityCheck (DESIGN_DECISIONS 19). Checks in scripts/integrity_check_gate.py.
# off   endgame preset, seed 7, 600 s, arms ca_tbl_full + ca_tbl_no_integrity, --integrityCheck=false
# on    the same with --integrityCheck=true (the check running: suspects flagged)
# def   CODE defaults (no preset; the preset sets the flag), 300 s, ca_tbl_full, flag absent;
# tru   the same with --integrityCheck=true: byte-identical
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_DD19_22: the preset this gate was written against (DESIGN_DECISIONS 19-22)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g163.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
P="--specProfile=true $PIN_DD19_22 --seed=7 --trace=true --boxHalf=750 --finderMode=1"
# shellcheck disable=SC2086
run off $P --duration=600 --arms=ca_tbl_full,ca_tbl_no_integrity --integrityCheck=false &
# shellcheck disable=SC2086
run on $P --duration=600 --arms=ca_tbl_full --integrityCheck=true &
# shellcheck disable=SC2086
run def --seed=7 --trace=true --duration=300 --arms=ca_tbl_full &
# shellcheck disable=SC2086
run tru --seed=7 --trace=true --duration=300 --arms=ca_tbl_full --integrityCheck=true &
wait
python3 scripts/integrity_check_gate.py "$D"
