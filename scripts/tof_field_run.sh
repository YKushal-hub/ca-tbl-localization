#!/usr/bin/env bash
# TOF-FIELD runner (report/PREREG_TOF_FIELD.md): the 2.4 GHz ToF ranging error at measured
# outdoor levels (Mueller et al., ICL-GNSS 2021) instead of the 3.0 m hardware floor, on the
# preset's 5 km and endgame configurations, for every arm that consumes tier-2 ranges. The preset
# (102 settings, e308c14f) carries FIXES-N26's fixes for ca_tbl_full and fixed_threshold alike, so
# the two compare the decision rules on equal measurements.
#
# usage: bash scripts/tof_field_run.sh [--resume] <outdir>
#   <outdir> must be outside the repository's tracked results (a temp directory). Run from a
#   worktree holding the frozen build (PREREG sect 2). Refuses to write into <outdir> if it
#   exists unless --resume; --resume re-runs only the cells without an rc.
set -u
cd "$(dirname "$0")/.." || exit 2
RESUME=0
if [ "${1:-}" = --resume ]; then
    RESUME=1
    shift
fi
OUT=${1:?usage: tof_field_run.sh [--resume] <outdir>}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: $OUT is inside the repository"; exit 2;; esac
if [ "$RESUME" -eq 0 ] && [ -e "$OUT" ]; then
    echo "refusing: $OUT exists"
    exit 2
fi
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$(pwd)/build/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
P="--specProfile=true --duration=1800 --trace=true"
END="--boxHalf=750 --finderMode=1"
X="--arms=ca_tbl_full,fixed_threshold,static_2p4,lora_fusion,poucet_fusion"
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
    for e in "D0" "R1 --tofFieldSigmaM=83.6 --tofFieldBiasM=46.4" "R0 --tofFieldSigmaM=83.6 --tofFieldBiasM=0" \
             "S --tofFieldSigmaM=45 --tofFieldBiasM=39"; do
        set -- $e
        n=$1
        shift
        # shellcheck disable=SC2068,SC2086
        cell "$OUT/X_main_$n/s$s" $X $P $@ --seed=$s
        # shellcheck disable=SC2068,SC2086
        cell "$OUT/X_end_$n/s$s" $X $P $END $@ --seed=$s
    done
done
wait
