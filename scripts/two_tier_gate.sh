#!/usr/bin/env bash
# G167 runner: the ca_tbl_two_tier arm and the --tier1ExtraLossDb flag. What each run is for is in
# PIN_PRE_DD41 (2026-11-22): written against --interferenceFrameKey off (the default until DD 41 was adopted); pinned so it tests what it was written to test.
# scripts/two_tier_gate.py, which reads them.
#
# usage: bash scripts/two_tier_gate.sh [<workdir>]
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_PRE_DD65: the preset this gate was written against (DESIGN_DECISIONS 65)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g159.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
S=${TWOTIER_GATE_SEED:-361}
B="--specProfile=true $PIN_PRE_DD65 --seed=$S --duration=${TWOTIER_GATE_DUR:-600} --trace=true --env=1 --boxHalf=750 --finderMode=1"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --interferenceFrameKey=false --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
# TWOTIER_GATE_JOBS=1 runs the six sims one after another (a loaded or small machine); default: all at once.
bg() { if [ "${TWOTIER_GATE_JOBS:-6}" -le 1 ]; then "$@"; else "$@" & fi; }
bg run all $B
bg run pair $B --arms=ca_tbl_full,ca_tbl_two_tier
bg run off $B --arms=ca_tbl_two_tier
bg run zero $B --arms=ca_tbl_two_tier --tier1ExtraLossDb=0
P="--specProfile=true $PIN_PRE_DD65 --probe=true --seed=$S --env=1"
bg run probe0 $P
bg run probe30 $P --tier1ExtraLossDb=30
wait
python3 scripts/two_tier_gate.py "$D"
