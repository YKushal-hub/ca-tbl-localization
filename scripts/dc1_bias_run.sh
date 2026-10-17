#!/usr/bin/env bash
# DC1-BIAS, as report/PREREG_DC1_BIAS.md registers it: the preset at A (5 km), B (endgame) and
# C (N8 ring), every comparator arm, DC-1's gyro bias sigma at L0 (the preset's 8.816 deg/h),
# L2 0.15 and L3 0.5 deg/s, seeds 1-20. 180 cells, <out>/<A|B|C>_<L0..L3>/s<seed>/.
#
# usage: bash scripts/dc1_bias_run.sh [--resume] <outdir>
#   --resume re-runs only cells without an rc file (a machine stop); completed cells are kept.
set -u
cd "$(dirname "$0")/.." || exit 2
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: dc1_bias_run.sh [--resume] <outdir>}
if [ "$RESUME" -eq 0 ] && [ -e "$OUT" ]; then
    echo "refusing: $OUT exists"
    exit 2
fi
dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
[ "$dirty" -eq 0 ] || { echo "refusing: $dirty tracked modification(s) under src/ scratch/"; exit 2; }
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$(pwd)/build/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=${DC1_JOBS:-$(nproc 2>/dev/null || echo 4)}
P="--specProfile=true --duration=1800 --trace=true"
ARMS="--arms=ca_tbl_full,static_868,static_2p4,fixed_threshold,poucet_fusion,lora_fusion"
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
for s in $(seq 1 20); do
    for lv in L0:0 L2:0.15 L3:0.5; do
        n=${lv%%:*}
        f=""
        [ "${lv#*:}" != 0 ] && f="--imuGyroBiasSigmaDps=${lv#*:}"
        cell "$OUT/A_$n/s$s" $ARMS $P --reportAirtime=true $f --seed=$s
        cell "$OUT/B_$n/s$s" $ARMS $P --boxHalf=750 --finderMode=1 --reportAirtime=true $f --seed=$s
        cell "$OUT/C_$n/s$s" $ARMS $P --nGateways=8 --gwLayout=legacy $f --seed=$s
    done
done
wait
echo "done; score with: python3 scripts/dc1_bias_analyse.py $OUT"
