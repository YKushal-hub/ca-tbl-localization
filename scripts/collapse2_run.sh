#!/usr/bin/env bash
# COLLAPSE-2 runner (report/PREREG_COLLAPSE2.md). usage: bash scripts/collapse2_run.sh <outdir> [stations|field]
# Committed with the registration, before any cell; not edited after the first cell.
set -u
cd "$(dirname "$0")/.." || exit 2
OUT=${1:?usage}
PART=${2:-stations}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: inside the repository"; exit 2;; esac
if [ -e "$OUT/build.sha256" ]; then
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "refusing resume: manifest mismatch"; exit 2; }
    echo "$(date -u +%FT%TZ) resume $PART; source $(git rev-parse --short HEAD)" >> "$OUT/RUN.md"
else
    [ -e "$OUT" ] && { echo "refusing: $OUT exists without a manifest"; exit 2; }
    dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
    [ "$dirty" -eq 0 ] || { echo "refusing: $dirty tracked modification(s) under src/ scratch/"; exit 2; }
    mkdir -p "$OUT/bin"; cp build/scratch/ns3.47-ca_tbl_causal_sim-optimized "$OUT/bin/" && cp -r build/lib "$OUT/bin/lib" || exit 2
    ( cd "$OUT/bin" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so > ../build.sha256 )
    echo "$(date -u +%FT%TZ) launch $PART; source $(git rev-parse --short HEAD) dirty $(git status --porcelain | wc -l)" > "$OUT/RUN.md"
fi
BIN="$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=${COLLAPSE2_JOBS:-6}
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
G8="--collapse=true --collapseFootX=39.74 --collapseFootY=39.74 --collapseDepthMin=0.1 --collapsePileH=3.658 --collapseDepthMax=3.658"
if [ "$PART" = stations ]; then
    D="--specProfile=true --trace=true --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true"
    D="$D --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500"
    D="$D --interferenceFrameKey=true --gwRidge=false --baseCampFrac=0 --searchLaunchAtS=1 --relaySlots=false"
    D="$D --arms=ca_tbl_full,static_868,static_2p4 --duration=1800 $G8 --boxHalf=94.08 --collapseBracket=central"
    N="--nTags=20 --measuredSubjects=20 --identityQueryManifests=$(seq -s, 1 20) --finderTasking=need"
    for seed in $(seq 801 810); do
        for gw in 3 6; do
            # shellcheck disable=SC2086
            cell "$OUT/s8_central_n20_gw${gw}/seed${seed}" $D $N --nGateways=$gw --seed=$seed
        done
    done
    wait
elif [ "$PART" = field ]; then
    F="--specProfile=true $G8 --collapseField=true --collapseFieldVictims=20 --collapseGridStep=5 --collapseHoldHeights=0.3,1.5"
    F="$F --collapseStations=4 --collapseStationR=56.45 --collapseStationH=6 --collapseSlots=32 --collapseSlotS=60"
    F="$F --collapseGpsSigmaH=3.27 --collapseGpsSigmaV=6.63 --collapseLocalSigma=0.10"
    for seed in $(seq 901 910); do
        for br in low central high; do
            # shellcheck disable=SC2086
            cell "$OUT/field_${br}/seed${seed}" $F --collapseBracket=$br --seed=$seed
        done
    done
    wait
    for d in "$OUT"/field_*/seed*; do
        [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] || continue
        for sp in 1.0 0.5; do
            [ -f "$d/estimates_sp${sp}.csv" ] && continue
            python3 scripts/collapse/estimate_collapse.py "$d" --every 1000 --spots "$sp" > "$d/estimate_sp${sp}.log" 2>&1 \
                && mv "$d/estimates_local.csv" "$d/estimates_sp${sp}.csv"
        done
    done
fi
echo "$(date -u +%FT%TZ) done $PART" >> "$OUT/RUN.md"
