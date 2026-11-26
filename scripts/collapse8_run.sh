#!/usr/bin/env bash
# COLLAPSE-8 runner (report/PREREG_COLLAPSE8.md): lone-worker rule DD 53b vs the no-leak gate configuration. usage: bash scripts/collapse4_run.sh <outdir> (outside the repo)
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh
OUT=${1:?usage}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: inside the repository"; exit 2;; esac
if [ -e "$OUT/build.sha256" ]; then
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "refusing resume: manifest mismatch"; exit 2; }
    echo "$(date -u +%FT%TZ) resume; source $(git rev-parse --short HEAD)" >> "$OUT/RUN.md"
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
J=${COLLAPSE8_JOBS:-8}
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
D="$D --arms=ca_tbl_full --duration=1800 --collapseFootX=39.74 --collapseFootY=39.74 --collapseDepthMin=0.1 --collapseDepthSolve=true --collapseNominalHeights=true --uwbFutilityCycles=4"
declare -A PILE=([5]=2.286 [8]=3.658)
declare -A BOX=([5]=71.22 [8]=94.08)
declare -A CFG=([base]="" [lone]="--loneStay868=true")
crew() { if [ "$1" = 1 ]; then echo "$PIN_ONE_WORKER --arms=ca_tbl_full,static_868"; else echo "--nTags=$1 --measuredSubjects=$1 --identityQueryManifests=$(seq -s, 1 "$1") --finderTasking=need"; fi; }
for seed in $(seq 841 850); do
    for st in 5 8; do
        for br in low central; do
            for n in 1 20; do
                for c in base lone; do
                    # shellcheck disable=SC2046,SC2086
                    cell "$OUT/s${st}_${br}_n${n}_${c}/seed${seed}" $D --collapsePileH=${PILE[$st]} --collapseDepthMax=${PILE[$st]} \
                        --boxHalf=${BOX[$st]} --collapseBracket=$br --seed=$seed $(crew $n) ${CFG[$c]}
                done
            done
        done
    done
done
wait
echo "$(date -u +%FT%TZ) done" >> "$OUT/RUN.md"
