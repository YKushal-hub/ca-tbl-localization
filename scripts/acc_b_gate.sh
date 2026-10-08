#!/usr/bin/env bash
# G110 runner: --dualBaseRelayTier (S2 Part B, F1). What each run is for is in
# scripts/acc_b_gate.py, which reads them.
#
# usage: bash scripts/acc_b_gate.sh [<workdir>]
#
# The preset's endgame (where relay legs matter), seed 7, 600 s, with a hybrid arm and two
# non-hybrid arms: the flag absent, given at its default, and on.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g110.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--specProfile=true --arms=ca_tbl_full,static_2p4,lora_fusion --seed=7 --duration=600 --trace=true --boxHalf=750 --finderMode=1"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run off $B &
run zero $B --dualBaseRelayTier=false &
run on $B --dualBaseRelayTier=true &
wait
python3 scripts/acc_b_gate.py "$D"
