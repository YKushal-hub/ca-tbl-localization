#!/usr/bin/env bash
# G77 runner: every measured subject runs tier-1 TDoA (--subjectTier1Tdoa; register row 55,
# MS-1 step 2). What each run is for, and what a firing looks like on disk, is in
# scripts/subject_tdoa_gate.py, which reads them.
#
# usage: bash scripts/subject_tdoa_gate.sh [<workdir>]
#
# The preset (five subjects, eight gateways, tier-1 TDoA) at seed 7, 900 s: the subjects' flag
# on and explicitly off, and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g77.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
# N8_ring, the geometry G77 was built on: a TDoA solve needs three gateways stamping one
# broadcast, which the preset's three gateways at 5 km give a subject rarely (G24, seeds 7/8/9:
# 0+1+5). This gate checks the mechanism; G24 checks its population under the preset.
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --specProfile=true --nGateways=8 --gwLayout=legacy"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --subjectTier1Tdoa=true &
run off $B --subjectTier1Tdoa=false &
wait
run r_notdoa --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --nTags=5 --measuredSubjects=5 --subjectTier1Tdoa=true

python3 scripts/subject_tdoa_gate.py "$D"
