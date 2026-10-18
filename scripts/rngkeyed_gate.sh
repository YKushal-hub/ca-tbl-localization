#!/usr/bin/env bash
# G138 runner: V1 --rngKeyed (master plan v3). usage: bash scripts/rngkeyed_gate.sh [<workdir>]; checks in scripts/rngkeyed_gate.py
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g138.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
B="--specProfile=true --arms=static_868,ca_tbl_full --seed=7 --duration=300 --trace=true --traceRanges=true"
run on   $B --rngKeyed=true &
run onp  $B --rngKeyed=true --rngKeyedProbeExtraTx=true &
run off  $B --rngKeyed=false &
run offp $B --rngKeyed=false --rngKeyedProbeExtraTx=true &
wait
python3 scripts/rngkeyed_gate.py "$D"
