#!/usr/bin/env bash
# G46 runner: every field node is a measured subject (docs/SPEC_CONFORMANCE_PLAN.md MS-1,
# --measuredSubjects; the plan's "G22 (subjects)"). What each run is for, and what a firing
# looks like on disk, is in scripts/subjects_gate.py, which reads them.
#
# usage: bash scripts/subjects_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# NC9's regime (four finders converging on the walker) with five tags, frames and
# arbitration on, static_868 and ca_tbl_full, 600 s: five subjects over two seeds; the
# explicit default (1); three subjects among five relays under --relayedRanging
# --relaySlots, so two relays are subjects and two are not; and three refusals or warnings.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g46.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=static_868,ca_tbl_full --seed=7 --duration=600 --trace=true --nFinders=4 --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true --escalationRequiresEvidence=false --nTags=5 --sendHandshakeFrames=true --uwbArbitration=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run five $B --seedCount=2 --measuredSubjects=5 &
run one $B --measuredSubjects=1 &
run relay $B --arms=ca_tbl_full --relayedRanging=true --relaySlots=true --measuredSubjects=3 &
wait
run r_big --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --nTags=3 --measuredSubjects=5
run r_zero --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --measuredSubjects=0
run warn --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --nTags=2 --measuredSubjects=2 --tier1Tdoa=true

python3 scripts/subjects_gate.py "$D"
