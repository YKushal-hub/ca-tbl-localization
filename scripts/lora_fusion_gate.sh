#!/usr/bin/env bash
# G88 runner: the dual-band LoRa fusion arm (--arms=lora_fusion; the LoRa comparators on both bands).
# What each run is for, and what a firing looks like on disk, is in
# scripts/lora_fusion_gate.py, which reads them.
#
# usage: bash scripts/lora_fusion_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# The preset's endgame (seed 7, 900 s): ca_tbl_full and static_868 with and without the arm
# beside them, and a short --arms=all run.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g88.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--seed=7 --duration=900 --trace=true --specProfile=true --boxHalf=750 --finderMode=1"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run with $B --arms=ca_tbl_full,lora_fusion,static_868 &
run without $B --arms=ca_tbl_full,static_868 &
run all --seed=1 --duration=30 --trace=false --arms=all &
wait

python3 scripts/lora_fusion_gate.py "$D"
