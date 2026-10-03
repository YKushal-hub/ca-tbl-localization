#!/usr/bin/env bash
# G59 runner: every measured subject routes its own position-fix traffic, on its own tier
# (docs/SPEC_CONFORMANCE_PLAN.md NW-2, --fixRoutingSubjects, --routeAlpha/Beta/Gamma/Slack).
# What each run is for, and what a firing looks like on disk, is in
# scripts/subject_routing_gate.py, which reads them.
#
# usage: bash scripts/subject_routing_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G18b's routed regime (relays across the default box, the confidence selector, 600 s) on
# the full arm with five measured subjects and one plain relay, tag 0's trace on: the
# subjects routing on their own tiers; the same with the RSSI-variance weight off and a
# slack of 2; the flag off (the control); and two refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g59.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=600 --trace=true --relayedRanging=true --nTags=6 --measuredSubjects=5 --fixRouting=true --fixRoutingConfidence=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run on $B --fixRoutingSubjects=true &
run w $B --fixRoutingSubjects=true --routeBeta=0 --routeSlack=2 &
run off $B &
wait
run r_nosubj --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --relayedRanging=true --nTags=6 --fixRouting=true --fixRoutingSubjects=true
run r_w --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --routeBeta=0

python3 scripts/subject_routing_gate.py "$D"
