#!/usr/bin/env bash
# EKF5-PRESET, as report/PREREG_EKF5_PRESET.md registers it: the preset at A, B and C, six arms,
# --imuFilter=ekf5 at DC-1 bias L0 (the preset), L2 0.15 and L3 0.5 deg/s, seeds 1-20 (180 cells),
# plus R1: one cv cell per (config, level) at seed 1 (9 cells). <out>/<A|B|C>_<ekf5|cv>_<L0|L2|L3>/s<seed>/
#
# usage: bash scripts/ekf5_preset_run.sh [--resume] <outdir>
set -u
cd "$(dirname "$0")/.." || exit 2
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: ekf5_preset_run.sh [--resume] <outdir>}
if [ "$RESUME" -eq 0 ] && [ -e "$OUT" ]; then
    echo "refusing: $OUT exists"
    exit 2
fi
dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
[ "$dirty" -eq 0 ] || { echo "refusing: $dirty tracked modification(s) under src/ scratch/"; exit 2; }
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$(pwd)/build/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=${EKF5_JOBS:-$(nproc 2>/dev/null || echo 4)}
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
config() {  # config <A|B|C>: that configuration's flags
    case $1 in
    A) echo "--reportAirtime=true" ;;
    B) echo "--boxHalf=750 --finderMode=1 --reportAirtime=true" ;;
    C) echo "--nGateways=8 --gwLayout=legacy" ;;
    esac
}
for s in $(seq 1 20); do
    for lv in L0:0 L2:0.15 L3:0.5; do
        n=${lv%%:*}
        b=""
        [ "${lv#*:}" != 0 ] && b="--imuGyroBiasSigmaDps=${lv#*:}"
        for c in A B C; do
            # shellcheck disable=SC2046
            cell "$OUT/${c}_ekf5_$n/s$s" $ARMS $P $(config $c) $b --imuFilter=ekf5 --seed=$s
            if [ "$s" -eq 1 ]; then
                # shellcheck disable=SC2046
                cell "$OUT/${c}_cv_$n/s$s" $ARMS $P $(config $c) $b --seed=$s
            fi
        done
    done
done
wait
echo "done; score with: python3 scripts/ekf5_preset_analyse.py $OUT <dc1_bias raw dir>"
