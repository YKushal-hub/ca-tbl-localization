#!/usr/bin/env bash
# R1-REINIT runner (report/PREREG_R1_REINIT.md). usage: bash scripts/r1_reinit_run.sh <outdir> (outside the repo)
# Runs a FROZEN copy of the binary: <outdir>/bin/ with build_r1.sha256, checked before every cell.
set -u
cd "$(dirname "$0")/.." || exit 2
OUT=${1:?usage: r1_reinit_run.sh <outdir>}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: $OUT is inside the repository"; exit 2;; esac
[ -e "$OUT" ] && { echo "refusing: $OUT exists"; exit 2; }
mkdir -p "$OUT/bin"
cp build/scratch/ns3.47-ca_tbl_causal_sim-optimized "$OUT/bin/" || exit 2
cp -r build/lib "$OUT/bin/lib" || exit 2
( cd "$OUT/bin" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so > ../build_r1.sha256 )
BIN="$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
P="--arms=ca_tbl_full,fixed_threshold,static_2p4 --fixedVarThreshold=12 --unknownStart=true --specProfile=true --duration=1800 --trace=true --traceCovariance=true --traceSubjectConsistency=true --imuQuScale=10"
END="--boxHalf=750 --finderMode=1"
cell() {
    local d=$1
    shift
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build_r1.sha256 ) || { echo "manifest mismatch before $d"; exit 3; }
    mkdir -p "$d"
    # shellcheck disable=SC2068
    ( cd "$(pwd)" && "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; rc=$?; echo "$rc" > "$d/rc" ) &
}
for s in $(seq 1 20); do
    # shellcheck disable=SC2086
    cell "$OUT/U0_A/s$s" $P --seed=$s
    # shellcheck disable=SC2086
    cell "$OUT/U0_B/s$s" $P $END --seed=$s
    # shellcheck disable=SC2086
    cell "$OUT/U1_A/s$s" $P --acquireReinit=true --seed=$s
    # shellcheck disable=SC2086
    cell "$OUT/U1_B/s$s" $P $END --acquireReinit=true --seed=$s
done
wait
rc=$?
echo "$(date -u +%FT%TZ) all cells done, wait exit $rc" >> "$OUT/RUN.log"
