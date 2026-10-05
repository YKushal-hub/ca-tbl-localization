#!/usr/bin/env bash
# LORA-CMP-2 runner (report/PREREG_LORA_CMP2.md): the LoRa comparators completed on both LoRa
# bands -- dual-band LoRa fusion (--arms=lora_fusion) with and without gateway TDoA, and
# 2.4 GHz LoRa ToF on N8_ring -- on CAMP-FINAL's three configurations, paired by seed with
# CAMP-FINAL's and LORA-CMP's cells. 1 800 s, traces on.
#
# usage: bash scripts/lora_cmp2_run.sh [--resume] <outdir>
#   Run from a worktree holding the frozen build of 3d637f4 (PREREG_LORA_CMP2 header). Refuses
#   if <outdir> exists (unless --resume). X_main/s1, X_end/s1 and X_rssi/s1 re-run cells of
#   CAMP-FINAL and LORA-CMP with this binary; the analysis aborts unless their summary rows
#   are byte-identical to those campaigns' (rule 8: the pairing crosses a binary change).
set -u
cd "$(dirname "$0")/.." || exit 2
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: lora_cmp2_run.sh [--resume] <outdir>}
if [ "$RESUME" -eq 0 ] && [ -e "$OUT" ]; then
    echo "refusing: $OUT exists (never write over a results directory)"
    exit 2
fi
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$(pwd)/build/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
P="--specProfile=true --duration=1800 --trace=true"
END="--boxHalf=750 --finderMode=1"
N8="--nGateways=8 --gwLayout=legacy"
X4="--arms=ca_tbl_full,static_868,static_2p4,poucet_fusion"
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
cell "$OUT/X_main/s1" $X4 $P --seed=1 --reportAirtime=true
# shellcheck disable=SC2086
cell "$OUT/X_end/s1" $X4 $P $END --seed=1 --reportAirtime=true
cell "$OUT/X_rssi/s1" --arms=static_868 --tier1Tdoa=false $P --seed=1 --reportAirtime=true
for s in $(seq 1 20); do
    cell "$OUT/F_main/s$s" --arms=lora_fusion $P --seed=$s --reportAirtime=true
    # shellcheck disable=SC2086
    cell "$OUT/F_end/s$s" --arms=lora_fusion $P $END --seed=$s --reportAirtime=true
    # shellcheck disable=SC2086
    cell "$OUT/F_n8/s$s" --arms=lora_fusion $P $N8 --seed=$s
    cell "$OUT/FR_main/s$s" --arms=lora_fusion --tier1Tdoa=false $P --seed=$s --reportAirtime=true
    # shellcheck disable=SC2086
    cell "$OUT/FR_end/s$s" --arms=lora_fusion --tier1Tdoa=false $P $END --seed=$s --reportAirtime=true
    # shellcheck disable=SC2086
    cell "$OUT/FR_n8/s$s" --arms=lora_fusion --tier1Tdoa=false $P $N8 --seed=$s
    # shellcheck disable=SC2086
    cell "$OUT/S2_n8/s$s" --arms=static_2p4 $P $N8 --seed=$s
done
wait
