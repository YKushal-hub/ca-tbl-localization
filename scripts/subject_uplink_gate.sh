#!/usr/bin/env bash
# G65 runner: subjects bill their tier-2/3 range uplink (SY-1 for subjects;
# --subjectEdgeUplink; TECH_DEBT N.3, register row 55). What each run is for, and what a
# firing looks like on disk, is in scripts/subject_uplink_gate.py, which reads them.
#
# usage: bash scripts/subject_uplink_gate.sh [<workdir>]
#
# Five measured subjects at defaults otherwise (static_2p4 and ca_tbl_full, seed 7, 600 s,
# traces on), tag 0's uplink billed in both: the flag on and off, and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g65.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=static_2p4,ca_tbl_full --seed=7 --duration=600 --trace=true --nTags=5 --measuredSubjects=5 --edgeUplinkRanges=true"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --subjectEdgeUplink=true &
run off $B &
wait
run r_noup --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --nTags=5 --measuredSubjects=5 --subjectEdgeUplink=true

python3 scripts/subject_uplink_gate.py "$D"
