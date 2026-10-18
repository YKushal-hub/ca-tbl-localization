#!/usr/bin/env bash
# B4-SWITCHING runner (report/PREREG_B4_SWITCHING.md).
# usage: bash scripts/b4_run.sh <outdir> <acquireReinit true|false> <onDemandR50M> <onDemandSigmaDb>   (outdir outside the repo)
set -u
cd "$(dirname "$0")/.." || exit 2
OUT=${1:?usage}; REINIT=${2:?usage}; R50=${3:?usage}; SIG=${4:?usage}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: inside the repository"; exit 2;; esac
[ -e "$OUT" ] && { echo "refusing: $OUT exists"; exit 2; }
mkdir -p "$OUT/bin"; cp build/scratch/ns3.47-ca_tbl_causal_sim-optimized "$OUT/bin/" && cp -r build/lib "$OUT/bin/lib" || exit 2
( cd "$OUT/bin" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so > ../build.sha256 )
{ echo "$(date -u +%FT%TZ) launch; source $(git rev-parse --short HEAD) dirty $(git status --porcelain | wc -l)"
  echo "acquireReinit=$REINIT (rule: true iff R1-REINIT R1a or R1c held)"; echo "onDemandR50M=$R50 onDemandSigmaDb=$SIG (fitted to the B3F clamped probe 2.4 GHz PDR curve, scripts/b4_fit_ondemand.py)"; } > "$OUT/RUN.md"
BIN="$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
C="--specProfile=true --duration=1800 --trace=true --traceCovariance=true --traceSubjectConsistency=true --imuQuScale=10 --unknownStart=true --fixedVarThreshold=12 --foliageDepthClamp=true --acquireReinit=$REINIT"
MAIN="--arms=static_868,static_2p4,lora_fusion,ca_tbl_full,fixed_threshold"
OD="--arms=ca_tbl_full --dualBaseOnDemand=true --onDemandR50M=$R50 --onDemandSigmaDb=$SIG"
END="--boxHalf=750 --finderMode=1"
cell() {
    local d=$1; shift
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; rc=$?; echo "$rc" > "$d/rc" ) &
}
for s in $(seq 1 20); do
    for p in engine gottschalk muller wolf; do
        for cfg in A B; do
            X=""; [ "$cfg" = B ] && X="$END"
            # shellcheck disable=SC2086
            cell "$OUT/P_${p}_$cfg/s$s" $C $MAIN $X --tofErrorModel=$p --rssiRangeFromShadowing=true --seed=$s
            # shellcheck disable=SC2086
            cell "$OUT/P_${p}_${cfg}_OD/s$s" $C $OD $X --tofErrorModel=$p --rssiRangeFromShadowing=true --seed=$s
        done
    done
    for cfg in A B; do
        X=""; [ "$cfg" = B ] && X="$END"
        # shellcheck disable=SC2086
        cell "$OUT/S1off_muller_$cfg/s$s" $C $MAIN $X --tofErrorModel=muller --rssiRangeFromShadowing=false --seed=$s
    done
done
wait
rc=$?
echo "$(date -u +%FT%TZ) all cells done, wait exit $rc" >> "$OUT/RUN.md"
