#!/usr/bin/env bash
# S2-C runner (report/PREREG_S2C.md): stage T (training, seeds 21-30), the registered selection of
# s*, then stage C (reuse check), U (unknown start) and H (held-out at s*).
#
# usage: bash scripts/s2c_run.sh [--resume] <outdir>
#   <outdir> must be outside the repository. Run from a worktree holding the frozen S2-C build.
set -u
cd "$(dirname "$0")/.." || exit 2
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: s2c_run.sh [--resume] <outdir>}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: $OUT is inside the repository"; exit 2;; esac
if [ "$RESUME" -eq 0 ] && [ -e "$OUT" ]; then
    echo "refusing: $OUT exists"
    exit 2
fi
mkdir -p "$OUT"
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$(pwd)/build/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
P="--specProfile=true --duration=1800 --trace=true --traceCovariance=true --traceSubjectConsistency=true --imuQuScale=10"
END="--boxHalf=750 --finderMode=1"
A3="--arms=ca_tbl_full,fixed_threshold,static_2p4 --fixedVarThreshold=12"
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
for s in $(seq 21 30); do
    for k in 1 3 10; do
        # shellcheck disable=SC2086
        cell "$OUT/T_A_s$k/s$s" --arms=ca_tbl_full $P --measVarScale=$k --seed=$s
    done
done
wait
SSTAR=$(python3 scripts/s2c_analyse.py --select "$OUT") || { echo "selection failed"; exit 3; }
echo "$SSTAR" > "$OUT/SSTAR"
for s in 1 2 3; do
    # shellcheck disable=SC2086
    cell "$OUT/C_A/s$s" $A3 $P --seed=$s
    # shellcheck disable=SC2086
    cell "$OUT/C_B/s$s" $A3 $P $END --seed=$s
done
for s in $(seq 1 20); do
    # shellcheck disable=SC2086
    cell "$OUT/U_A/s$s" $A3 $P --unknownStart=true --seed=$s
    # shellcheck disable=SC2086
    cell "$OUT/U_B/s$s" $A3 $P $END --unknownStart=true --seed=$s
done
if [ "$SSTAR" != 1 ]; then
    for s in $(seq 1 20); do
        # shellcheck disable=SC2086
        cell "$OUT/H_A/s$s" --arms=ca_tbl_full,static_2p4 $P --measVarScale=$SSTAR --seed=$s
        # shellcheck disable=SC2086
        cell "$OUT/H_B/s$s" --arms=ca_tbl_full,static_2p4 $P $END --measVarScale=$SSTAR --seed=$s
    done
fi
wait
