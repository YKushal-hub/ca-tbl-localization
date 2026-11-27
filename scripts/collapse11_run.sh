#!/usr/bin/env bash
# COLLAPSE-11 runner (report/PREREG_COLLAPSE11.md): COLLAPSE-6 pipe cells, DD 49 off and on (DD 54/56/57/60/62 and the DD 63 world on in both), new seeds. usage: bash scripts/collapse11_run.sh <outdir> pipe
# Committed with the registration, before any cell; not edited after the first cell.
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh
OUT=${1:?usage}
PART=${2:?usage: pipe|field}
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
J=${COLLAPSE11_JOBS:-8}
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
if [ "$PART" = pipe ]; then
    D="--specProfile=true --trace=true --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true"
    D="$D --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500"
    D="$D --interferenceFrameKey=true --collapse=true --gwRidge=false --baseCampFrac=0 --searchLaunchAtS=1 --relaySlots=false"
    D="$D --duration=1800 --collapseFootX=39.74 --collapseFootY=39.74 --collapseDepthMin=0.1 --collapseDepthSolve=true"
    D="$D --uwbFutilityCycles=4 --collapseBracket=central --collapseNominalHeights=true"
    declare -A PILE=([5]=2.286 [8]=3.658)
    declare -A BOX=([5]=71.22 [8]=94.08)
    crew() { echo "--nTags=$1 --measuredSubjects=$1 --identityQueryManifests=$(seq -s, 1 "$1") --finderTasking=need"; }
    declare -A F
    F[off]="--sparseRangeGeometry=true --peerSplitCi=true --sharedDuty868=true --dutyReport868=true --sharedDuty868Finders=true --tdoaRubbleDelay=true --tdoaHeightCorrect=true"
    F[on]="${F[off]} --tofRubbleCorrect=true"
    for seed in $(seq 1701 1710); do
        for st in 5 8; do
            for x in off on; do
                G="$D --collapsePileH=${PILE[$st]} --collapseDepthMax=${PILE[$st]} --boxHalf=${BOX[$st]} --seed=$seed ${F[$x]}"
                # shellcheck disable=SC2046,SC2086
                {
                cell "$OUT/s${st}_g_$x/seed$seed"      $G $(crew 20) --nGateways=4 --arms=ca_tbl_full,static_868,static_2p4,static_uwb,poucet_fusion
                cell "$OUT/s${st}_muller_$x/seed$seed" $G $(crew 20) --nGateways=4 --arms=ca_tbl_full,static_2p4 --tofErrorModel=muller
                cell "$OUT/s${st}_st2_$x/seed$seed"    $G $(crew 20) --nGateways=2 --arms=ca_tbl_full
                cell "$OUT/s${st}_n10_$x/seed$seed"    $G $(crew 10) --nGateways=4 --arms=ca_tbl_full
                cell "$OUT/s${st}_n1_$x/seed$seed"     $G $PIN_ONE_WORKER --nGateways=4 --arms=ca_tbl_full,static_868,static_2p4
                }
            done
        done
    done
    wait
fi
echo "$(date -u +%FT%TZ) done" >> "$OUT/RUN.md"
