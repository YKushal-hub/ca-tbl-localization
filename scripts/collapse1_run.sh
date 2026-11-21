#!/usr/bin/env bash
# COLLAPSE-1 runner (report/PREREG_COLLAPSE1.md). usage: bash scripts/collapse1_run.sh <outdir> (outside the repo)
# Committed with the registration, before any cell; not edited after the first cell.
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh
OUT=${1:?usage}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: inside the repository"; exit 2;; esac
if [ -e "$OUT/build.sha256" ]; then
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "refusing resume: manifest mismatch"; exit 2; }
    echo "$(date -u +%FT%TZ) resume; source $(git rev-parse --short HEAD) dirty $(git status --porcelain | wc -l)" >> "$OUT/RUN.md"
else
    [ -e "$OUT" ] && { echo "refusing: $OUT exists without a manifest"; exit 2; }
    dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
    [ "$dirty" -eq 0 ] || { echo "refusing: $dirty tracked modification(s) under src/ scratch/"; exit 2; }
    mkdir -p "$OUT/bin"; cp build/scratch/ns3.47-ca_tbl_causal_sim-optimized "$OUT/bin/" && cp -r build/lib "$OUT/bin/lib" || exit 2
    ( cd "$OUT/bin" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so > ../build.sha256 )
    echo "$(date -u +%FT%TZ) launch; source $(git rev-parse --short HEAD) dirty $(git status --porcelain | wc -l)" > "$OUT/RUN.md"
fi
BIN="$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=${COLLAPSE1_JOBS:-6}
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
D="$D --interferenceFrameKey=true --collapse=true --gwRidge=false --baseCampFrac=0 --nGateways=4 --searchLaunchAtS=1 --relaySlots=false"
D="$D --arms=ca_tbl_full,static_868,static_2p4 --duration=1800 --collapseFootX=39.74 --collapseFootY=39.74 --collapseDepthMin=0.1"
# storeys -> pile height (PILE-ALPHA), station ring R / 0.6 (NIOSH-CZ)
declare -A PILE=([2]=0.914 [5]=2.286 [8]=3.658)
declare -A BOX=([2]=48.36 [5]=71.22 [8]=94.08)
crew() { # N
    if [ "$1" = 1 ]; then echo "$PIN_ONE_WORKER"; else
        echo "--nTags=$1 --measuredSubjects=$1 --identityQueryManifests=$(seq -s, 1 "$1") --finderTasking=need"; fi
}
for seed in $(seq 801 810); do
    for st in 2 5 8; do
        G="--collapsePileH=${PILE[$st]} --collapseDepthMax=${PILE[$st]} --boxHalf=${BOX[$st]}"
        for br in low central high; do
            for n in 1 20; do
                # shellcheck disable=SC2046
                cell "$OUT/s${st}_${br}_n${n}/seed${seed}" $D $G --collapseBracket=$br --seed=$seed $(crew $n)
            done
        done
        # shellcheck disable=SC2046
        cell "$OUT/s${st}_central_n30/seed${seed}" $D $G --collapseBracket=central --seed=$seed $(crew 30)
    done
done
wait
echo "$(date -u +%FT%TZ) done" >> "$OUT/RUN.md"
