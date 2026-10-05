#!/usr/bin/env bash
# FIXES-N26 runner (report/PREREG_FIXES_N26.md): the four N.26 fixes measured on the preset,
# singly and together, paired by seed with CAMP-FINAL's, LORA-CMP's and LORA-CMP-2's cells.
#
# usage: bash scripts/fixes_n26_run.sh cal|eval [--resume] <outdir>
#   cal:  fault-free calibration cells (seeds 21-40, never evaluated) for the integrity threshold.
#   eval: needs <outdir>/fit.json (scripts/fixes_n26_analyse.py fit); seeds 1-20 and the
#         fault matrix's 1-10, plus reproduction cells for the cross-binary pairing (rule 8).
#   Run from a worktree holding the frozen build (PREREG header). Refuses to write into a phase
#   directory that exists unless --resume; --resume re-runs only the cells without an rc.
set -u
cd "$(dirname "$0")/.." || exit 2
MODE=${1:?usage: fixes_n26_run.sh cal|eval [--resume] <outdir>}
shift
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: fixes_n26_run.sh cal|eval [--resume] <outdir>}
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$(pwd)/build/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
P="--specProfile=true --duration=1800 --trace=true"
END="--boxHalf=750 --finderMode=1"
N8="--nGateways=8 --gwLayout=legacy"
AIR="--reportAirtime=true"
ALL="--loraDualBase=true --uwbCrossTierPoll=true --tdoaInnovationGate=true --mahalUseR=true"
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
case "$MODE" in
cal)
    if [ "$RESUME" -eq 0 ] && [ -e "$OUT/cal" ]; then
        echo "refusing: $OUT/cal exists"
        exit 2
    fi
    for s in $(seq 21 40); do
        # shellcheck disable=SC2086
        cell "$OUT/cal/CAL_main/s$s" --arms=ca_tbl_full $P $ALL --seed=$s
        # shellcheck disable=SC2086
        cell "$OUT/cal/CAL_end/s$s" --arms=ca_tbl_full $P $ALL $END --seed=$s
    done
    ;;
eval)
    if [ "$RESUME" -eq 0 ] && [ -e "$OUT/eval" ]; then
        echo "refusing: $OUT/eval exists"
        exit 2
    fi
    [ -f "$OUT/fit.json" ] || { echo "no $OUT/fit.json: run fixes_n26_analyse.py fit first"; exit 2; }
    CHI=$(python3 -c "import json;print(json.load(open('$OUT/fit.json'))['integrityChi2'])")
    E="$OUT/eval"
    X="--arms=ca_tbl_full,static_868,poucet_fusion"
    # shellcheck disable=SC2086
    cell "$E/X_main/s1" $X $P $AIR --seed=1
    # shellcheck disable=SC2086
    cell "$E/X_end/s1" $X $P $END $AIR --seed=1
    # shellcheck disable=SC2086
    cell "$E/X_lf/s1" --arms=lora_fusion $P $END $AIR --seed=1
    for s in $(seq 1 20); do
        # shellcheck disable=SC2086
        cell "$E/ALL_main/s$s" --arms=ca_tbl_full $P $AIR $ALL --integrityChi2=$CHI --seed=$s
        # shellcheck disable=SC2086
        cell "$E/ALL_end/s$s" --arms=ca_tbl_full $P $END $AIR $ALL --integrityChi2=$CHI --seed=$s
        # shellcheck disable=SC2086
        cell "$E/ALL_n8/s$s" --arms=ca_tbl_full $P $N8 $ALL --integrityChi2=$CHI --seed=$s
        for c in "D --loraDualBase=true" "U --uwbCrossTierPoll=true" "T --tdoaInnovationGate=true" \
                 "M --mahalUseR=true --integrityChi2=$CHI"; do
            set -- $c
            n=$1
            shift
            # shellcheck disable=SC2068
            cell "$E/${n}_main/s$s" --arms=ca_tbl_full $P $AIR $@ --seed=$s
            # shellcheck disable=SC2068,SC2086
            cell "$E/${n}_end/s$s" --arms=ca_tbl_full $P $END $AIR $@ --seed=$s
        done
    done
    for f in "rb_common --injectRangeBias=30 --injectFaultAnchors=0" "rb_single --injectRangeBias=30 --injectFaultAnchors=1" \
             "txdrop --injectTxPowerDropDb=6" "antenna --injectAntennaLossDb=10"; do
        set -- $f
        name=$1
        shift
        for s in $(seq 1 10); do
            # shellcheck disable=SC2068,SC2086
            cell "$E/E_${name}_downweight/s$s" --arms=ca_tbl_full $P $ALL --integrityChi2=$CHI --seed=$s \
                --injectBiasFraction=0.25 --suspectRecovery=downweight $@
        done
    done
    ;;
*)
    echo "usage: fixes_n26_run.sh cal|eval [--resume] <outdir>"
    exit 2
    ;;
esac
wait
