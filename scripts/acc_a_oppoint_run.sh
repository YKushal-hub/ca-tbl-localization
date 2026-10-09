#!/usr/bin/env bash
# ACC-A-OPPOINT runner (report/PREREG_ACC_A_OPPOINT.md): fixed_threshold alone at
# --fixedVarThreshold 100 (control), 8, 12, 25 in the endgame and 12 at 5 km, k = 10, on the frozen
# ACC-A build.
#
# usage: bash scripts/acc_a_oppoint_run.sh [--resume] <outdir>
#   <outdir> must be outside the repository (a temp directory). Run from a worktree holding the
#   frozen ACC-A build (PREREG sect 1). Refuses to write into <outdir> if it exists unless --resume;
#   --resume re-runs only the cells without an rc.
set -u
cd "$(dirname "$0")/.." || exit 2
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: acc_a_oppoint_run.sh [--resume] <outdir>}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: $OUT is inside the repository"; exit 2;; esac
if [ "$RESUME" -eq 0 ] && [ -e "$OUT" ]; then
    echo "refusing: $OUT exists"
    exit 2
fi
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$(pwd)/build/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
P="--arms=fixed_threshold --specProfile=true --duration=1800 --trace=true --traceCovariance=true --imuQuScale=10"
END="--boxHalf=750 --finderMode=1"
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
    # shellcheck disable=SC2086
    cell "$OUT/C_B_T100/s$s" $P $END --fixedVarThreshold=100 --seed=$s
    for t in 8 12 25; do
        # shellcheck disable=SC2086
        cell "$OUT/O_B_T$t/s$s" $P $END --fixedVarThreshold=$t --seed=$s
    done
    # shellcheck disable=SC2086
    cell "$OUT/O_A_T12/s$s" $P --fixedVarThreshold=12 --seed=$s
done
wait
