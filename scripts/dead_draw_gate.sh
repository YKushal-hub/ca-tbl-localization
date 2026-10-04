#!/usr/bin/env bash
# G71 runner: a node whose battery has run out draws nothing more (--deadBatteryNoDraw;
# TECH_DEBT N.20). What each run is for, and what a firing looks like on disk, is in
# scripts/dead_draw_gate.py, which reads them.
#
# usage: bash scripts/dead_draw_gate.sh [<workdir>]
#
# G67's regime (seed 7, 900 s, five subjects, a 3 J budget for tag 0 and every subject,
# triggered updates so the last gasps go), with --silenceStopsRelay (which N.20 needs): the
# flag on and off, and two refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g71.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --nTags=5 --measuredSubjects=5 --triggeredUpdates=true --subjectTriggeredUpdates=true --batteryDepleteJ=3 --subjectBatteryDeplete=true --silenceStopsRelay=true"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --deadBatteryNoDraw=true &
run off $B &
wait
run r_norelay --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --batteryDepleteJ=3 --deadBatteryNoDraw=true
run r_nobat --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --deadBatteryNoDraw=true

python3 scripts/dead_draw_gate.py "$D"
