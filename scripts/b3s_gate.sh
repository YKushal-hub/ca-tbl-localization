#!/usr/bin/env bash
# G135 runner: S1 --rssiRangeFromShadowing and S2 --tofErrorModel (master plan v2), via the probe.
# usage: bash scripts/b3s_gate.sh [<workdir>]; checks in scripts/b3s_gate.py
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g135.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
P="--probe=true --seed=7"
run base $P &
run off $P --rssiRangeFromShadowing=false --tofErrorModel=engine &
run s1 $P --rssiRangeFromShadowing=true &
run gottschalk $P --tofErrorModel=gottschalk &
run muller $P --tofErrorModel=muller &
run wolf $P --tofErrorModel=wolf &
wait
python3 scripts/b3s_gate.py "$D"
