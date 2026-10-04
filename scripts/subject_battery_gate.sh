#!/usr/bin/env bash
# G67 runner: subjects run the battery fault and its last gasp (--subjectBatteryDeplete;
# TECH_DEBT N.3, register row 55). What each run is for, and what a firing looks like on
# disk, is in scripts/subject_battery_gate.py, which reads them.
#
# usage: bash scripts/subject_battery_gate.sh [<workdir>]
#
# G34's regime (full arm, seed 7, 900 s, events on, a 3 J budget) with five measured subjects
# and triggered updates on for tag 0 and the subjects: the flag on and off, and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g67.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --vizEvents=true --nTags=5 --measuredSubjects=5 --triggeredUpdates=true --subjectTriggeredUpdates=true --batteryDepleteJ=3"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --subjectBatteryDeplete=true &
run off $B &
wait
run r_nobat --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --nTags=5 --measuredSubjects=5 --subjectBatteryDeplete=true

python3 scripts/subject_battery_gate.py "$D"
