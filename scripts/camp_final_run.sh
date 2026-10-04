#!/usr/bin/env bash
# CAMP-FINAL runner (report/PREREG_CAMP_FINAL.md): the finished preset measured for the spec's
# comparison table -- six arms plus poucet_fusion at 5 km and in the endgame, the N8_ring
# sensitivity geometry, the single-hop baseline, the fault matrix, the flooding pair for the
# airtime numbers, and the time-on-air table. 1 800 s, traces on.
#
# usage: bash scripts/camp_final_run.sh [--resume] <outdir>
#   Refuses if <outdir> exists (unless --resume). A cell is <outdir>/<cell>/s<seed>/, with rc
#   and run.log beside the outputs. --resume re-runs only the cells without an rc file, with
#   this same command line; the simulation is deterministic.
set -u
cd "$(dirname "$0")/.." || exit 2
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: camp_final_run.sh [--resume] <outdir>}
if [ "$RESUME" -eq 0 ] && [ -e "$OUT" ]; then
    echo "refusing: $OUT exists (never write over a results directory)"
    exit 2
fi
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
# Pin the libraries beside the binary (a frozen worktree copy; PREREG_CAMP_FINAL header).
export LD_LIBRARY_PATH="$(pwd)/build/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
ARMS7="--arms=ca_tbl_full,ca_tbl_no_integrity,fixed_threshold,static_868,static_2p4,static_uwb,poucet_fusion"
P="--specProfile=true --duration=1800 --trace=true"
END="--boxHalf=750 --finderMode=1"
N8="--nGateways=8 --gwLayout=legacy"
SINGLE="--relayedRanging=false --relayChain=false --hopVarianceTerms=false --hopProvenance=false --fixRouting=false --fixRoutingConfidence=false --fixRoutingSubjects=false"
cell() {  # cell <dir> <args...>
    local d=$1
    shift
    [ -f "$d/rc" ] && return 0
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    rm -rf "$d"
    mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
mkdir -p "$OUT"
cell "$OUT/T_toa/s1" --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --toaTable=true
for s in $(seq 1 20); do
    cell "$OUT/A_main/s$s" $ARMS7 $P --seed=$s --reportAirtime=true                 # A: 5 km
    # shellcheck disable=SC2086
    cell "$OUT/B_end/s$s" $ARMS7 $P $END --seed=$s --reportAirtime=true             # B: endgame
    # shellcheck disable=SC2086
    cell "$OUT/C_n8/s$s" --arms=ca_tbl_full,static_868 $P $N8 --seed=$s             # C: N8_ring
    # shellcheck disable=SC2086
    cell "$OUT/G_single/s$s" --arms=ca_tbl_full $P $SINGLE --seed=$s                # G: single hop
    # shellcheck disable=SC2086
    cell "$OUT/G_single_end/s$s" --arms=ca_tbl_full $P $SINGLE $END --seed=$s
    cell "$OUT/H_flood/s$s" --arms=ca_tbl_full $P --seed=$s --reportAirtime=true --reversePath=false
done
for f in "rb_common --injectRangeBias=30 --injectFaultAnchors=0" "rb_single --injectRangeBias=30 --injectFaultAnchors=1" \
         "txdrop --injectTxPowerDropDb=6" "antenna --injectAntennaLossDb=10"; do
    set -- $f
    name=$1
    shift
    for rec in downweight none; do
        for s in $(seq 1 10); do
            cell "$OUT/E_${name}_${rec}/s$s" --arms=ca_tbl_full,ca_tbl_no_integrity $P --seed=$s \
                --injectBiasFraction=0.25 --suspectRecovery=$rec $@
        done
    done
done
wait
