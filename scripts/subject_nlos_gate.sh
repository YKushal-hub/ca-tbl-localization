#!/usr/bin/env bash
# G78 runner: every measured subject scores its own links (--subjectNlosClassifier; register row 55,
# MS-1 step 2). What each run is for, and what a firing looks like on disk, is in
# scripts/subject_nlos_gate.py, which reads them.
#
# usage: bash scripts/subject_nlos_gate.sh [<workdir>]
#
# The preset (five subjects, eight gateways, the NLOS classifier feeding PCS term 3) at seed 7, 900 s: the subjects' flag
# on and explicitly off, and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g78.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --specProfile=true"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --subjectNlosClassifier=true &
run off $B --subjectNlosClassifier=false &
wait
run r_nonlos --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --nTags=5 --measuredSubjects=5 --subjectNlosClassifier=true

python3 scripts/subject_nlos_gate.py "$D"
