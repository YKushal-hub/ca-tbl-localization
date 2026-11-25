#!/usr/bin/env bash
# G179 runner: --collapse in the main simulator (DD 39), the full pipeline for buried workers.
#   n20      20 buried measured workers, ring stations around the pile, 300 s, three arms
#   n1       the same with one worker ($PIN_ONE_WORKER): worker 1 at the same spot as in n20
#   r_*      refusals: missing geometry, depthMax > pileH, bad bracket
# Off-byte-identity is checked when the flag is added (DESIGN_DECISIONS 39), not here.
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g179.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
G="--collapse=true --collapseFootX=30 --collapseFootY=30 --collapsePileH=3.66 --collapseDepthMin=0.5 --collapseDepthMax=3.5 --collapseBracket=central"
S="--specProfile=true $PIN_PRE_DD65 --boxHalf=50 --gwRidge=false --baseCampFrac=0 --nGateways=4 --searchLaunchAtS=1 --relaySlots=false --arms=ca_tbl_full,static_868,static_2p4 --seed=5 --duration=300"
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
M=$(seq -s, 1 20)
# shellcheck disable=SC2086
run n20 $S $G --nTags=20 --measuredSubjects=20 --identityQueryManifests=$M &
# shellcheck disable=SC2086
run n1 $S $G $PIN_ONE_WORKER &
wait
# shellcheck disable=SC2086
run r_missing $S --collapse=true --nTags=20 --measuredSubjects=20
# shellcheck disable=SC2086
run r_depth $S ${G/--collapseDepthMax=3.5/--collapseDepthMax=4.0} --nTags=20 --measuredSubjects=20
# shellcheck disable=SC2086
run r_bracket $S ${G/central/medium} --nTags=20 --measuredSubjects=20
python3 scripts/collapse_main_gate.py "$D"
