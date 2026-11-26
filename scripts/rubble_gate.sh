#!/usr/bin/env bash
# G177 runner: collapse-extension rubble model (World side), ca-tbl-rubble-world.{h,cc}.
# Unit values vs docs/SOURCES_COLLAPSE.md rows, A5 validation targets, geometry,
# refusals, and the World/system no-leak check. See scripts/rubble_gate.py.
# Off-byte-identity (no world set): checked against the pre-hook build when the hooks
# were added (DESIGN_DECISIONS 36); scripts/rubble_gate_selftest.sh proves this gate fires.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_rubble_selftest-optimized"
OUT=${1:-$(mktemp "${TMPDIR:-/tmp}/g177.XXXXXX")}
"$BIN" > "$OUT" || { echo "G177 FAIL: selftest binary exit $?"; exit 1; }
python3 scripts/rubble_gate.py "$OUT"
