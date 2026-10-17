#!/usr/bin/env bash
# G87 runner: the Poucet-style fusion arm (--arms=poucet_fusion; the spec's comparison table).
# What each run is for, and what a firing looks like on disk, is in
# scripts/poucet_arm_gate.py, which reads them.
#
# usage: bash scripts/poucet_arm_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# The preset's endgame (seed 7, 900 s): ca_tbl_full and static_uwb with and without the arm
# beside them, and a short --arms=all run.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g87.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
# B12 (scope 2026-10-17): this gate was written against the preset before FIXES-N26 (2026-10-08) and the master-plan-v3
# adoption (2026-10-17). PIN restores that preset, so the gate tests what it was written to test; the flags it
# exercises come after PIN and override it.
PIN="--loraDualBase=false --uwbCrossTierPoll=false --tdoaInnovationGate=false --mahalUseR=false --integrityChi2=9.21 --rngKeyed=false --rangeErrorNoLinkLeak=false --rssiRangeFromShadowing=false --nlosRangeBias=false --rangeReplyPosition=false --acquireParticles=false --foliageDepthClamp=false --radioBasis=lr1121 --halfDuplexChip=true"
B="--seed=7 --duration=900 --trace=true --specProfile=true --boxHalf=750 --finderMode=1 $PIN"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run with $B --arms=ca_tbl_full,poucet_fusion,static_uwb &
run without $B --arms=ca_tbl_full,static_uwb &
run all --seed=1 --duration=30 --trace=false --arms=all &
wait

python3 scripts/poucet_arm_gate.py "$D"
