#!/usr/bin/env bash
# G89 runner: the four N.26 fixes (--loraDualBase, --uwbCrossTierPoll, --integrityChi2 with
# --mahalUseR, --tdoaInnovationGate). What each run is for, and what a firing looks like on
# disk, is in scripts/n26_fixes_gate.py, which reads them.
#
# usage: bash scripts/n26_fixes_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# The preset's endgame (seed 7, 900 s; ca_tbl_full and static_868) with each fix off and on,
# and the 5 km seed whose TDoA went wrong (seed 1, 1 800 s; static_868) with the gate off/on.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g89.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
# B12 (scope 2026-10-17): this gate was written against the preset before FIXES-N26 (2026-10-08) and the master-plan-v3
# adoption (2026-10-17). PIN restores that preset, so the gate tests what it was written to test; the flags it
# exercises come after PIN and override it.
PIN="--loraDualBase=false --uwbCrossTierPoll=false --tdoaInnovationGate=false --mahalUseR=false --integrityChi2=9.21 --rngKeyed=false --rangeErrorNoLinkLeak=false --rssiRangeFromShadowing=false --nlosRangeBias=false --rangeReplyPosition=false --acquireParticles=false --foliageDepthClamp=false --radioBasis=lr1121 --halfDuplexChip=true --fadingPerBand=false --relayNisInflate=false --rssiSigmaLogNormal=false --imuQuScale=1"
E="--specProfile=true --seed=7 --duration=900 --trace=true --boxHalf=750 --finderMode=1 --arms=ca_tbl_full,static_868 $PIN"
K="--specProfile=true --seed=1 --duration=1800 --trace=true --arms=static_868 $PIN"
# Every run states all four fixes explicitly (TECH_DEBT N.31). Since e308c14f the preset turns
# them ON, so a run that left them to the preset was no longer an "off" control, and a single-fix
# run had the other three on too. Each run below starts from all four off at the pre-preset
# values (integrityChi2 = 9.21, chi-square 0.99) and turns on only the one it tests; a later
# flag overrides an earlier one, so "$OFF4 --loraDualBase=true" is the dual base alone.
OFF4="--loraDualBase=false --uwbCrossTierPoll=false --tdoaInnovationGate=false --mahalUseR=false --integrityChi2=9.21"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run e_off $E $OFF4 &
run e_dual $E $OFF4 --loraDualBase=true &
run e_cross $E $OFF4 --uwbCrossTierPoll=true &
wait
run e_chi $E $OFF4 --mahalUseR=true --integrityChi2=20 &
run k_off $K $OFF4 &
run k_gate $K $OFF4 --tdoaInnovationGate=true &
wait

python3 scripts/n26_fixes_gate.py "$D"
