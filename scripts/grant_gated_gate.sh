#!/usr/bin/env bash
# G49 runner: a UWB escalation commits only on a confirmed grant (docs/SPEC_CONFORMANCE_PLAN.md
# NW-7, --grantGatedUwb). What each run is for, and what a firing looks like on disk, is in
# scripts/grant_gated_gate.py, which reads them.
#
# usage: bash scripts/grant_gated_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G33's handshake regime (four finders converging, three tags, frames, arbitration and
# --handshakeFixes, seed 1, 600 s) with the event log: the flag on with both static arms
# beside ca_tbl_full; the flag on with five measured subjects in G46's regime (seed 7, five
# tags), where subjects reach UWB; the flag off (the control); and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g49.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--seed=1 --duration=600 --trace=true --nFinders=4 --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true --escalationRequiresEvidence=false --sendHandshakeFrames=true --uwbArbitration=true --handshakeFixes=true --nTags=3 --vizEvents=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run on $B --arms=static_868,static_uwb,ca_tbl_full --grantGatedUwb=true &
run subj $B --arms=ca_tbl_full --seed=7 --nTags=5 --measuredSubjects=5 --grantGatedUwb=true &
run off $B --arms=ca_tbl_full &
wait
run r_nofix --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --sendHandshakeFrames=true --grantGatedUwb=true

python3 scripts/grant_gated_gate.py "$D"
