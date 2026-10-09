#!/usr/bin/env bash
# ACC-A runner (report/PREREG_ACC_A_FILTER.md): the IMU increment covariance multiplier
# (--imuQuScale) on the 102-setting preset, every arm, with the integrity-trigger ablation and a
# dose-response on ca_tbl_full.
#
# usage: bash scripts/acc_a_run.sh [--resume] <outdir>
#   <outdir> must be outside the repository (a temp directory). Run from a worktree holding the
#   frozen build (PREREG sect 2). Refuses to write into <outdir> if it exists unless --resume;
#   --resume re-runs only the cells without an rc.
set -u
cd "$(dirname "$0")/.." || exit 2
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: acc_a_run.sh [--resume] <outdir>}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: $OUT is inside the repository"; exit 2;; esac
if [ "$RESUME" -eq 0 ] && [ -e "$OUT" ]; then
    echo "refusing: $OUT exists"
    exit 2
fi
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$(pwd)/build/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
P="--specProfile=true --duration=1800 --trace=true --traceCovariance=true"
END="--boxHalf=750 --finderMode=1"
M="--arms=ca_tbl_full,fixed_threshold,ca_tbl_no_integrity,static_2p4,static_868,lora_fusion,poucet_fusion"
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
    for k in 1 10; do
        # shellcheck disable=SC2086
        cell "$OUT/M_A_k$k/s$s" $M $P --imuQuScale=$k --seed=$s
        # shellcheck disable=SC2086
        cell "$OUT/M_B_k$k/s$s" $M $P $END --imuQuScale=$k --seed=$s
        # shellcheck disable=SC2086
        cell "$OUT/E_A_k$k/s$s" --arms=ca_tbl_full $P --suspectForcesEscalation=true --imuQuScale=$k --seed=$s
        # shellcheck disable=SC2086
        cell "$OUT/E_B_k$k/s$s" --arms=ca_tbl_full $P $END --suspectForcesEscalation=true --imuQuScale=$k --seed=$s
    done
    for k in 3 30; do
        # shellcheck disable=SC2086
        cell "$OUT/K_A_k$k/s$s" --arms=ca_tbl_full $P --imuQuScale=$k --seed=$s
        # shellcheck disable=SC2086
        cell "$OUT/K_B_k$k/s$s" --arms=ca_tbl_full $P $END --imuQuScale=$k --seed=$s
    done
done
wait
