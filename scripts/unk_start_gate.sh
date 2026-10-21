#!/usr/bin/env bash
# G130 runner: --unknownStart, --traceSubjectConsistency and --measVarScale (S2-C). What each run is
# for, and what a firing looks like on disk, is in scripts/unk_start_gate.py, which reads them.
#
# usage: bash scripts/unk_start_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test; without one they go to a mktemp
#   directory removed on exit.
#
# The preset in the endgame configuration (box half-width 750 m), ca_tbl_full, seed 7, 300 s.
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_PRE_DD19: the preset this gate was written against (DESIGN_DECISIONS 19-24)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g130.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--specProfile=true $PIN_PRE_DD19 --arms=ca_tbl_full --seed=7 --duration=300 --trace=true --boxHalf=750 --finderMode=1 --loraDualBase=false --uwbCrossTierPoll=false --tdoaInnovationGate=false --mahalUseR=false --integrityChi2=9.21 --rngKeyed=false --rangeErrorNoLinkLeak=false --rssiRangeFromShadowing=false --nlosRangeBias=false --rangeReplyPosition=false --acquireParticles=false --foliageDepthClamp=false --radioBasis=lr1121 --halfDuplexChip=true --fadingPerBand=false --relayNisInflate=false --rssiSigmaLogNormal=false --imuQuScale=1"  # pinned to the pre-adoption preset (DESIGN_DECISIONS 10, 11, 13, 14): the gate's populations were defined there
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run base $B &
run dflt $B --unknownStart=false --traceSubjectConsistency=false --measVarScale=1 &
run cons $B --traceSubjectConsistency=true &
run unk $B --unknownStart=true &
wait
run mvs $B --traceSubjectConsistency=true --measVarScale=4
python3 scripts/unk_start_gate.py "$D"
