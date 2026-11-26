#!/usr/bin/env bash
# COLLAPSE-13 runner (report/PREREG_COLLAPSE13.md): DD 75 --sigmaShadow off and on, the DD 67 preset, new seeds. usage: bash scripts/collapse13_run.sh <outdir> all
# Committed with the registration, before any cell; not edited after the first cell.
set -u
cd "$(dirname "$0")/.." || exit 2
OUT=${1:?usage}
PART=${2:?usage: all}
OUT=$(realpath -m "$OUT")
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
J=${COLLAPSE13_JOBS:-8}
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
D="--specProfile=true --trace=true --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true"
D="$D --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500"
D="$D --interferenceFrameKey=true --collapse=true --gwRidge=false --baseCampFrac=0 --searchLaunchAtS=1 --relaySlots=false"
D="$D --duration=1800 --collapseFootX=39.74 --collapseFootY=39.74 --collapseDepthMin=0.1 --collapseDepthSolve=true"
D="$D --uwbFutilityCycles=4 --collapseNominalHeights=true --arms=ca_tbl_full"
D="$D --nTags=20 --measuredSubjects=20 --identityQueryManifests=$(seq -s, 1 20) --finderTasking=need"
S5="--collapsePileH=2.286 --collapseDepthMax=2.286 --boxHalf=71.22"
S8="--collapsePileH=3.658 --collapseDepthMax=3.658 --boxHalf=94.08"
declare -A X=([off]="" [on]="--sigmaShadow=true")
for seed in $(seq 2101 2110); do
    for x in off on; do
        # shellcheck disable=SC2086
        {
        cell "$OUT/K5_$x/seed$seed" $D $S5 --collapseBracket=central --nGateways=4 --seed=$seed ${X[$x]}
        cell "$OUT/K8_$x/seed$seed" $D $S8 --collapseBracket=central --nGateways=4 --seed=$seed ${X[$x]}
        cell "$OUT/H5_$x/seed$seed" $D $S5 --collapseBracket=high --nGateways=4 --seed=$seed ${X[$x]}
        cell "$OUT/H8_$x/seed$seed" $D $S8 --collapseBracket=high --nGateways=4 --seed=$seed ${X[$x]}
        cell "$OUT/K2_$x/seed$seed" $D $S5 --collapseBracket=central --nGateways=2 --seed=$seed ${X[$x]}
        }
    done
done
wait
echo "$(date -u +%FT%TZ) done" >> "$OUT/RUN.md"
