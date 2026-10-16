#!/usr/bin/env bash
# G134 runner: --foliageDepthClamp (B3f, report/FOLIAGE_DEPTH_DEFECT.md). The checks are in
# scripts/foliage_gate.py, which reads these runs.
# usage: bash scripts/foliage_gate.sh [<workdir>]
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g134.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
P="--probe=true --seed=7"
S="--specProfile=true --arms=ca_tbl_full --seed=7 --duration=600 --trace=true --boxHalf=750 --finderMode=1"
run pbase $P &
run poff $P --foliageDepthClamp=false &
run pon $P --foliageDepthClamp=true &
run sbase $S &
run soff $S --foliageDepthClamp=false &
run son $S --foliageDepthClamp=true &
wait
python3 scripts/foliage_gate.py "$D"
