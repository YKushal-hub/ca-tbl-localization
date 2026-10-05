#!/usr/bin/env bash
# G68 runner: a subject's range to a finder uses the finder's reported position
# (--subjectFinderPseudoAnchor; TECH_DEBT N.21). What each run is for, and what a firing looks
# like on disk, is in scripts/subject_pseudo_gate.py, which reads them.
#
# usage: bash scripts/subject_pseudo_gate.sh [<workdir>]
#
# G57's converging regime (five subjects, four finders converging in a 750 m box, mesh links,
# finder digests, finders as pseudo-anchors for tag 0) with entry velocity and prediction:
# the subjects' flag on and off, and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g68.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --nFinders=4 --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true --escalationRequiresEvidence=false --nTags=5 --measuredSubjects=5 --finderDispersal=1.0 --finderPseudoAnchor=true --sendHandshakeFrames=true --finderDigestPeriodS=20 --bidirectionalDigest=true --finderTasking=roundrobin --meshLinks=all --entryVelocity=true --entryPrediction=true"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --subjectFinderPseudoAnchor=true &
run off $B &
wait
run r_nopa --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --nTags=5 --measuredSubjects=5 --subjectFinderPseudoAnchor=true

python3 scripts/subject_pseudo_gate.py "$D"
