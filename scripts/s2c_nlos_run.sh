#!/usr/bin/env bash
# S2-C-NLOS runner (report/PREREG_S2C_NLOS.md): --nlosRSource operative / none / detector, A and B,
# ca_tbl_full, seeds 1-10.
# usage: bash scripts/s2c_nlos_run.sh [--resume] <outdir>   (outside the repository)
set -u
cd "$(dirname "$0")/.." || exit 2
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: s2c_nlos_run.sh [--resume] <outdir>}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: $OUT is inside the repository"; exit 2;; esac
if [ "$RESUME" -eq 0 ] && [ -e "$OUT" ]; then
    echo "refusing: $OUT exists"
    exit 2
fi
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$(pwd)/build/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
P="--arms=ca_tbl_full --specProfile=true --duration=1800 --trace=true --traceCovariance=true --traceSubjectConsistency=true --imuQuScale=10"
END="--boxHalf=750 --finderMode=1"
cell() {
    local d=$1
    shift
    [ -f "$d/rc" ] && return 0
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    rm -rf "$d"
    mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
for s in $(seq 1 10); do
    for m in oper none det; do
        case $m in oper) f=operative;; none) f=none;; det) f=detector;; esac
        # shellcheck disable=SC2086
        cell "$OUT/N_A_$m/s$s" $P --nlosRSource=$f --seed=$s
        # shellcheck disable=SC2086
        cell "$OUT/N_B_$m/s$s" $P $END --nlosRSource=$f --seed=$s
    done
done
wait
