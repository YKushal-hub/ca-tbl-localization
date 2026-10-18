#!/usr/bin/env bash
# S2-C-ACQ runner (report/PREREG_S2C_ACQ.md). usage: bash scripts/s2c_acq_run.sh <outdir> (outside the repo)
set -u
cd "$(dirname "$0")/.." || exit 2
OUT=${1:?usage: s2c_acq_run.sh <outdir>}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: $OUT is inside the repository"; exit 2;; esac
[ -e "$OUT" ] && { echo "refusing: $OUT exists"; exit 2; }
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$(pwd)/build/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
P="--arms=ca_tbl_full,static_2p4 --unknownStart=true --specProfile=true --duration=1800 --trace=true --traceCovariance=true --traceSubjectConsistency=true --imuQuScale=10"
cell() {
    local d=$1
    shift
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
for s in 1 2; do
    # shellcheck disable=SC2086
    cell "$OUT/R/s$s" $P --seed=$s
done
for s in $(seq 1 10); do
    # shellcheck disable=SC2086
    cell "$OUT/Q3/s$s" $P --acquireMinAnchors=3 --seed=$s
done
wait
