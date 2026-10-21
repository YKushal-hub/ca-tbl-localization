#!/usr/bin/env bash
# G70 runner: a silenced radio relays no other node's frame (--silenceStopsRelay; TECH_DEBT
# N.1). What each run is for, and what a firing looks like on disk, is in
# scripts/silence_relay_gate.py, which reads them.
#
# usage: bash scripts/silence_relay_gate.sh [<workdir>]
#
# G34's regime (seed 7, 900 s, a 3 J battery, MC-2's triggered updates so the last gasp
# goes, the per-frame event log): the flag on and off; the --injectSilentId fault (node 1
# mute from 300 to 600 s) with the flag on; and neither fault (unused, with a NOTE).
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g70.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --vizEvents=true"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --batteryDepleteJ=3 --triggeredUpdates=true --silenceStopsRelay=true &
run off $B --batteryDepleteJ=3 --triggeredUpdates=true &
run sil $B --injectSilentId=1 --injectSilentStartS=300 --injectSilentEndS=600 --silenceStopsRelay=true &
wait
run r_none --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --silenceStopsRelay=true

python3 scripts/silence_relay_gate.py "$D"
