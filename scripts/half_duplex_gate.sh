#!/usr/bin/env bash
# G74 runner: every radio half-duplex (--halfDuplex; TECH_DEBT N.6). What each run is for,
# and what a firing looks like on disk, is in scripts/half_duplex_gate.py, which reads them.
#
# usage: bash scripts/half_duplex_gate.sh [<workdir>]
#
# Seed 7, 900 s, defaults with the event log: --halfDuplex 0 (off), 1 (count only) and
# 2 (enforce), and a refusal.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g74.XXXXXX") || exit 2
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
run off $B &
run count $B --halfDuplex=1 &
run enforce $B --halfDuplex=2 &
wait
run r_bad --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --halfDuplex=3

python3 scripts/half_duplex_gate.py "$D"
