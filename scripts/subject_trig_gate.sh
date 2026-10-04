#!/usr/bin/env bash
# G66 runner: subjects send MC-2's large correction (--subjectTriggeredUpdates; TECH_DEBT
# N.3, register row 55). What each run is for, and what a firing looks like on disk, is in
# scripts/subject_trig_gate.py, which reads them.
#
# usage: bash scripts/subject_trig_gate.sh [<workdir>]
#
# G34's regime (full arm, seed 7, 900 s, events on) with five measured subjects and tag 0's
# triggered updates on, at --triggeredUpdateK=0.5: at the default k = 1 no subject's
# correction exceeds one prior sigma at seeds 1, 2, 3 or 7 (the largest is 0.88 sigma), so
# the path is exercised at half the threshold. The subjects' flag on and off, and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g66.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --vizEvents=true --nTags=5 --measuredSubjects=5 --triggeredUpdates=true --triggeredUpdateK=0.5"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --subjectTriggeredUpdates=true &
run off $B &
wait
run r_notu --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --nTags=5 --measuredSubjects=5 --subjectTriggeredUpdates=true

python3 scripts/subject_trig_gate.py "$D"
