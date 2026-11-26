#!/usr/bin/env bash
# G180 runner: --interferenceFrameKey (DD 41). 5 buried workers, 4 stations 30 m from them, 120 s, ca_tbl_full.
#   off  flag false (PIN_PRE_DD41): subjects 2..5 starve (the defect, kept as the reference)
#   on   flag absent (the default since 2026-11-22): subjects 2..5 hear the stations
# Off-byte-identity against the pre-flag build was checked when the flag was added (DESIGN_DECISIONS 41).
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g180.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
A="--specProfile=true --collapse=true --collapseFootX=30 --collapseFootY=30 --collapsePileH=3.66 --collapseDepthMin=0.5 --collapseDepthMax=3.5 --boxHalf=50 --gwRidge=false --baseCampFrac=0 --nGateways=4 --searchLaunchAtS=1 --relaySlots=false --arms=ca_tbl_full --seed=5 --duration=120 --nTags=5 --measuredSubjects=5 --identityQueryManifests=1,2,3,4,5"
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
# shellcheck disable=SC2086
run off $A --interferenceFrameKey=false &
# shellcheck disable=SC2086
run on $A &
wait
python3 scripts/frame_key_gate.py "$D"
