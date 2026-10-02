#!/usr/bin/env bash
# G47 runner: finders search for specific people (docs/SPEC_CONFORMANCE_PLAN.md MS-1,
# --finderTasking=roundrobin). What each run is for, and what a firing looks like on disk,
# is in scripts/finder_tasking_gate.py, which reads them.
#
# usage: bash scripts/finder_tasking_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# NC9's regime with five tags and four finders converging (--finderMode=1, dispersal 1
# sigma), ca_tbl_full, seed 7, 600 s: two subjects (two finders each, so the spread is
# within a pair) and five (one finder each, one subject untasked); the sweep-then-converge
# mode from a base camp; the explicit default against no flag; and four refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g47.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=600 --trace=true --nFinders=4 --boxHalf=750 --uwbCoverageIncludesFinders=true --escalationRequiresEvidence=false --nTags=5 --finderDispersal=1.0 --finderPseudoAnchor=true --sendHandshakeFrames=true --finderDigestPeriodS=20"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run rr2 $B --finderMode=1 --measuredSubjects=2 --finderTasking=roundrobin &
run rr5 $B --finderMode=1 --measuredSubjects=5 --finderTasking=roundrobin &
run sweep $B --finderMode=2 --baseCampFrac=0.75 --measuredSubjects=3 --finderTasking=roundrobin &
run def $B --finderMode=1 --measuredSubjects=5 &
run tag0 $B --finderMode=1 --measuredSubjects=5 --finderTasking=tag0 &
wait
S="--arms=ca_tbl_full --seed=1 --duration=30 --trace=false --nTags=3"
run r_bad $S --nFinders=2 --measuredSubjects=2 --finderTasking=nearest
run r_one $S --nFinders=2 --finderTasking=roundrobin
run r_nofind $S --measuredSubjects=2 --finderTasking=roundrobin
run r_mode0 $S --nFinders=2 --finderMode=0 --measuredSubjects=2 --finderTasking=roundrobin

python3 scripts/finder_tasking_gate.py "$D"
