#!/usr/bin/env bash
# G84 runner: one LR1121 serves 868 MHz and 2.4 GHz (--halfDuplexChip; TECH_DEBT N.6).
# What each run is for, and what a firing looks like on disk, is in
# scripts/chip_duplex_gate.py, which reads them.
#
# usage: bash scripts/chip_duplex_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# G74's regime (seed 7, 900 s, defaults, the event log) on the LR1121 basis: half-duplex
# counted per radio and per chip, enforced per chip, and two refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g84.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --vizEvents=true --radioBasis=lr1121"
run() {
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run count $B --halfDuplex=1 &
run countchip $B --halfDuplex=1 --halfDuplexChip=true &
run enforcechip $B --halfDuplex=2 --halfDuplexChip=true &
wait
run r_sx --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --halfDuplex=1 --halfDuplexChip=true
run r_nohd --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --radioBasis=lr1121 --halfDuplexChip=true

python3 scripts/chip_duplex_gate.py "$D"
