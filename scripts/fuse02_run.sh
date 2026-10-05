#!/usr/bin/env bash
# FUSE-02 runner (report/PREREG_FUSE02.md): the preset as it stands (no fuse) against the preset with --fuseFloorActive --fuseFloorShort
# (on), at 5 km and in the endgame, seeds 1-20, arms ca_tbl_full and static_868,
# 1 800 s, traces on.
#
# usage: bash scripts/fuse02_run.sh [--resume] <outdir>
#   Refuses if <outdir> exists. A cell is <outdir>/<config>_<off|on>/s<seed>/, with rc and
#   run.log beside the outputs. The simulation is deterministic, so an interrupted grid is
#   resumed by re-running only the cells without an rc file, with this same command line
#   (bash scripts/fuse02_run.sh --resume <outdir>), and that is recorded in RUN.md.
set -u
cd "$(dirname "$0")/.." || exit 2
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: fuse02_run.sh [--resume] <outdir>}
if [ "$RESUME" -eq 0 ] && [ -e "$OUT" ]; then
    echo "refusing: $OUT exists"
    exit 2
fi
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
J=$(nproc 2>/dev/null || echo 4)
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
ON="--fuseFloorActive=true --fuseFloorShort=true"
B="--arms=ca_tbl_full,static_868 --duration=1800 --trace=true --specProfile=true"
for s in $(seq 1 20); do
    cell "$OUT/main_off/s$s" $B --seed=$s
    cell "$OUT/main_on/s$s" $B --seed=$s $ON
    # shellcheck disable=SC2086
    cell "$OUT/end_off/s$s" $B --seed=$s $END
    # shellcheck disable=SC2086
    cell "$OUT/end_on/s$s" $B --seed=$s $END $ON
done
wait
