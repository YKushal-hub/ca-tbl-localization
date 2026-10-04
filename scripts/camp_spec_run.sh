#!/usr/bin/env bash
# CAMP-SPEC runner (report/PREREG_CAMP_SPEC.md): the spec configuration measured.
# usage: bash scripts/camp_spec_run.sh <outdir>     (the outdir must not exist)
set -u
cd "$(dirname "$0")/.." || exit 2
OUT=${1:?usage: camp_spec_run.sh <outdir>}
[ -e "$OUT" ] && { echo "refusing: $OUT exists (never write over a results directory)"; exit 2; }
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
J=$(nproc 2>/dev/null || echo 4)
ARMS6="--arms=ca_tbl_full,ca_tbl_no_integrity,fixed_threshold,static_868,static_2p4,static_uwb"
P="--specProfile=true --duration=1800 --trace=true"
END="--boxHalf=750 --finderMode=1"
# The LoRaWAPS-style single-hop baseline: no relayed measurement in any fix set, and so none
# of what depends on relays (FATAL otherwise): the chain, routing, the per-hop terms.
SINGLE="--relayedRanging=false --relayChain=false --hopVarianceTerms=false --hopProvenance=false --fixRouting=false --fixRoutingConfidence=false --fixRoutingSubjects=false"
RUNNER_UP=$(cat "$(dirname "$0")/../report/geo01_runner_up_flags.txt" 2>/dev/null)
[ -n "$RUNNER_UP" ] || { echo "refusing: report/geo01_runner_up_flags.txt is missing or empty"; exit 2; }
cell() {  # cell <dir> <args...>
    local d=$1
    shift
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
mkdir -p "$OUT"
for s in $(seq 1 20); do
    cell "$OUT/A_main/s$s" $ARMS6 $P --seed=$s                                    # A: preset x six arms
    cell "$OUT/B_end/s$s" $ARMS6 $P $END --seed=$s                                # B: the endgame
    # shellcheck disable=SC2086
    cell "$OUT/C_runnerup/s$s" --arms=ca_tbl_full,static_868 $P $RUNNER_UP --seed=$s  # C: GEO-01 stage 3
    cell "$OUT/G_single/s$s" --arms=ca_tbl_full $P $SINGLE --seed=$s              # G: single-hop baseline
    cell "$OUT/G_single_end/s$s" --arms=ca_tbl_full $P $SINGLE $END --seed=$s
done
# D: the NW-2 routing-weight sweep (endgame, where subjects route through each other)
for w in "1 1 1 1" "1 0 0 1" "0 1 0 1" "0 0 1 1" "1 0.01 1 1" "1 1 1 2" "1 0 0 2" "1 0.01 1 2"; do
    set -- $w
    for s in $(seq 1 10); do
        cell "$OUT/D_route_a$1_b$2_g$3_s$4/s$s" --arms=ca_tbl_full $P $END --seed=$s \
            --routeAlpha=$1 --routeBeta=$2 --routeGamma=$3 --routeSlack=$4
    done
done
# E: the SY-5 fault matrix, downweight (the preset) against none (reject and coast)
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
python3 scripts/camp_spec_analyse.py "$OUT"
