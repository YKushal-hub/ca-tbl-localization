#!/usr/bin/env bash
# G82 runner: run_id.txt records the commit the binary was BUILT from (--stampBuildCommit;
# TECH_DEBT M.3). What each run is for, and what a firing looks like on disk, is in
# scripts/build_stamp_gate.py, which reads them.
#
# usage: bash scripts/build_stamp_gate.sh [<workdir>]
#
# A short run with the flag and without it, launched from OUTSIDE the repository (so the
# source line reads "unknown" and cannot be the binary's), and the repository's state now.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
BIN="$ROOT/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g82.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
D=$(cd "$D" && pwd)
git rev-parse --short HEAD > "$D/head.txt"
git status --porcelain --untracked-files=no -- src scratch | wc -l > "$D/dirty.txt"
cp cmake-cache/scratch/catbl_build_stamp.cc "$D/stamp.cc" 2>/dev/null
B="--arms=ca_tbl_full --seed=7 --duration=120 --trace=true"
run() {  # run <name> <args...>, from $D (outside the repository)
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    (cd "$D" && "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc")
}
run on $B --stampBuildCommit=true &
run off $B &
wait

python3 scripts/build_stamp_gate.py "$D"
