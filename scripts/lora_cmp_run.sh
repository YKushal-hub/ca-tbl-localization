#!/usr/bin/env bash
# LORA-CMP runner (report/PREREG_LORA_CMP.md): plain RSSI LoRa localisation (static_868 with
# --tier1Tdoa=false) on CAMP-FINAL's three configurations, so the outdoor-LoRa comparison table
# can pair it with CAMP-FINAL's cells by seed. 1 800 s, traces on.
#
# usage: bash scripts/lora_cmp_run.sh [--resume] <outdir>
#   Run from a worktree holding CAMP-FINAL's frozen build (PREREG_LORA_CMP header). Refuses if
#   <outdir> exists (unless --resume). X_repro/s1 re-runs CAMP-FINAL's A_main seed 1
#   static_868 alone; the analysis aborts unless its row is byte-identical to CAMP-FINAL's.
set -u
cd "$(dirname "$0")/.." || exit 2
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: lora_cmp_run.sh [--resume] <outdir>}
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
RSSI="--arms=static_868 --tier1Tdoa=false"
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
cell "$OUT/X_repro/s1" --arms=static_868 $P --seed=1 --reportAirtime=true
for s in $(seq 1 20); do
    # shellcheck disable=SC2086
    cell "$OUT/R_main/s$s" $RSSI $P --seed=$s --reportAirtime=true         # A_main's flags
    # shellcheck disable=SC2086
    cell "$OUT/R_end/s$s" $RSSI $P $END --seed=$s --reportAirtime=true     # B_end's flags
    # shellcheck disable=SC2086
    cell "$OUT/R_n8/s$s" $RSSI $P $N8 --seed=$s                            # C_n8's flags
done
wait
