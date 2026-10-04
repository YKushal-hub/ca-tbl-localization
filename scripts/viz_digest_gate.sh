#!/usr/bin/env bash
# G73 runner: the sidecar digests the visualiser's outputs (--digestVizOutputs; TECH_DEBT
# M.4). What each run is for, and what a firing looks like on disk, is in
# scripts/viz_digest_gate.py, which reads them.
#
# usage: bash scripts/viz_digest_gate.sh [<workdir>]
#
# One short run with every visualiser output (--vizEvents, --vizTablePeriodS=60), with the
# flag and without it, and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g73.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=300 --trace=true --vizEvents=true --vizTablePeriodS=60"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run on $B --digestVizOutputs=true &
run off $B &
wait
run r_noviz --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --digestVizOutputs=true

python3 scripts/viz_digest_gate.py "$D"
