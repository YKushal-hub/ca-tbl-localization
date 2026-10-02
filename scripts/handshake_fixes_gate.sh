#!/usr/bin/env bash
# G33 runner: the escalation handshake as MAC sect 8 specifies it
# (docs/SPEC_CONFORMANCE_PLAN.md MC-1, --handshakeFixes). What each run is for, and what a
# firing looks like on disk, is in scripts/handshake_fixes_gate.py, which reads them.
#
# usage: bash scripts/handshake_fixes_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Three 600 s runs in null control NC9's regime -- the one where the tag reaches UWB --
# with handshake frames, arbitration and two peer tags (whose synthetic requests give the
# arbiter something to deny): the default, --handshakeFixes=false and =true. Plus one
# refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g33.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=1 --duration=600 --trace=true --nFinders=4 --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true --escalationRequiresEvidence=false --nlosClassifier=true --sendHandshakeFrames=true --uwbArbitration=true --nTags=3 --vizEvents=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run def $B &
run off $B --handshakeFixes=false &
run fix $B --handshakeFixes=true &
wait
run r_noframes --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --handshakeFixes=true

python3 scripts/handshake_fixes_gate.py "$D"
