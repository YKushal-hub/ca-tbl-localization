#!/usr/bin/env bash
# EKF5-CONSIDER (report/PREREG_EKF5_CONSIDER.md): --ekf5Robust=8 on the preset under ekf5, at DC-1
# bias L0, L2, L3, configurations A, B, C, seeds 1-20, the six comparator arms -- paired by seed
# with EKF5-PRESET's ekf5 cells (mode 0). R1: one mode-0 cell per (config, level) on this binary.
# Committed with the registration, before any cell; not edited after the first cell.
#
# usage: bash scripts/ekf5_consider_run.sh [--resume] <outdir>
set -u
cd "$(dirname "$0")/.." || exit 2
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: ekf5_consider_run.sh [--resume] <outdir>}
if [ "$RESUME" -eq 0 ] && [ -e "$OUT" ]; then
    echo "refusing: $OUT exists"
    exit 2
fi
dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
[ "$dirty" -eq 0 ] || { echo "refusing: $dirty tracked modification(s) under src/ scratch/"; exit 2; }
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$(pwd)/build/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=${CONSIDER_JOBS:-$(nproc 2>/dev/null || echo 4)}
P="--specProfile=true --duration=1800 --trace=true --imuFilter=ekf5"
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
config() {  # config <A|B|C>: that configuration's flags
    case $1 in
    A) echo "--reportAirtime=true" ;;
    B) echo "--boxHalf=750 --finderMode=1 --reportAirtime=true" ;;
    C) echo "--nGateways=8 --gwLayout=legacy" ;;
    esac
}
for s in $(seq 1 20); do
    for lv in L2:0.15 L3:0.5 L0:0; do
        n=${lv%%:*}
        b=""
        [ "${lv#*:}" != 0 ] && b="--imuGyroBiasSigmaDps=${lv#*:}"
        for c in A B C; do
            # shellcheck disable=SC2046
            cell "$OUT/${c}_m8_$n/s$s" $ARMS $P $(config $c) $b --ekf5Robust=8 --seed=$s
            if [ "$s" -eq 1 ]; then
                # shellcheck disable=SC2046
                cell "$OUT/${c}_m0_$n/s$s" $ARMS $P $(config $c) $b --seed=$s
            fi
        done
    done
done
wait
echo "done; score with: python3 scripts/ekf5_consider_analyse.py $OUT <ekf5_preset raw dir>"
