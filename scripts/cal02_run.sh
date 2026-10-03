#!/usr/bin/env bash
# CAL-02 runner (report/PREREG_CAL02.md).
#   gen  <outdir>: the data. The preset with PCS left legacy, and its endgame variant, seeds
#                  1-40, arms ca_tbl_full and fixed_threshold, 1 800 s.
#   eval <outdir>: the calibrated preset on the held-out seeds 21-40, both configurations,
#                  the weights and thresholds read from <outdir>/fit.json (written by
#                  scripts/cal02_analyse.py fit).
set -u
cd "$(dirname "$0")/.." || exit 2
MODE=${1:?usage: cal02_run.sh gen|eval <outdir>}
OUT=${2:?usage: cal02_run.sh gen|eval <outdir>}
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
J=$(nproc 2>/dev/null || echo 4)
END="--boxHalf=750 --finderMode=1"
cell() {  # cell <dir> <args...>
    local d=$1
    shift
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
case "$MODE" in
gen)
    [ -e "$OUT/gen" ] && { echo "refusing: $OUT/gen exists"; exit 2; }
    for s in $(seq 1 40); do
        cell "$OUT/gen/main/s$s" --arms=ca_tbl_full,fixed_threshold --seed=$s --duration=1800 --trace=true --specProfile=true
        cell "$OUT/gen/end/s$s" --arms=ca_tbl_full,fixed_threshold --seed=$s --duration=1800 --trace=true --specProfile=true $END
    done
    ;;
eval)
    [ -e "$OUT/eval" ] && { echo "refusing: $OUT/eval exists"; exit 2; }
    [ -f "$OUT/fit.json" ] || { echo "no $OUT/fit.json: run cal02_analyse.py fit first"; exit 2; }
    CAL=$(python3 -c "import json;f=json.load(open('$OUT/fit.json'));print(' '.join('--%s=%r'%(k,v) for k,v in f['flags'].items()))")
    for s in $(seq 21 40); do
        # shellcheck disable=SC2086
        cell "$OUT/eval/main/s$s" --arms=ca_tbl_full --seed=$s --duration=1800 --trace=true --specProfile=true --pcsForm=calibrated $CAL
        # shellcheck disable=SC2086
        cell "$OUT/eval/end/s$s" --arms=ca_tbl_full --seed=$s --duration=1800 --trace=true --specProfile=true --pcsForm=calibrated $CAL $END
    done
    ;;
*) echo "mode must be gen or eval"; exit 2 ;;
esac
wait
