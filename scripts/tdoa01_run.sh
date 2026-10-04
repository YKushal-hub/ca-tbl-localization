#!/usr/bin/env bash
# TDOA-01 runner (report/PREREG_TDOA01.md): the preset (which sets --tier1Tdoa) in three
# configurations -- p8 (as it is), p3 (--nGateways=3), end (--boxHalf=750 --finderMode=1) --
# seeds 1-20, arms ca_tbl_full and static_868, 1 800 s, traces on.
#
# usage: bash scripts/tdoa01_run.sh [--resume] <outdir>
#   Refuses if <outdir> exists (unless --resume, which re-runs only cells without an rc file,
#   with this same command line; the simulation is deterministic).
set -u
cd "$(dirname "$0")/.." || exit 2
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: tdoa01_run.sh [--resume] <outdir>}
if [ "$RESUME" -eq 0 ] && [ -e "$OUT" ]; then
    echo "refusing: $OUT exists"
    exit 2
fi
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
J=$(nproc 2>/dev/null || echo 4)
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
B="--arms=ca_tbl_full,static_868 --duration=1800 --trace=true --specProfile=true"
for s in $(seq 1 20); do
    cell "$OUT/p8/s$s" $B --seed=$s
    cell "$OUT/p3/s$s" $B --seed=$s --nGateways=3
    cell "$OUT/end/s$s" $B --seed=$s --boxHalf=750 --finderMode=1
done
wait
