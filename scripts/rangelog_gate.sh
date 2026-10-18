#!/usr/bin/env bash
# G137 runner: B1 --traceRanges. usage: bash scripts/rangelog_gate.sh [<workdir>]; checks in scripts/rangelog_gate.py
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g137.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
B="--specProfile=true --arms=ca_tbl_full --seed=7 --duration=600 --trace=true --boxHalf=750 --finderMode=1"
run base $B &
run off $B --traceRanges=false &
run on $B --traceRanges=true &
wait
python3 scripts/rangelog_gate.py "$D"
