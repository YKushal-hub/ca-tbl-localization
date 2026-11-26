#!/usr/bin/env bash
# COLLAPSE-6 runner (report/PREREG_COLLAPSE6.md): COLLAPSE-5 pipe with --collapseNominalHeights=true, plus N = 1. usage: bash scripts/collapse6_run.sh <outdir> pipe
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
J=${COLLAPSE6_JOBS:-8}
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
est() {  # est RUNDIR NAME MOVES METHODS
    local d=$1 name=$2 mv=$3 me=$4
    [ -f "$d/estimates_$name.csv" ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( mkdir -p "$d/.$name" && ln -sf "$d"/obs_*.csv "$d"/run.txt "$d/.$name/" 2>/dev/null
      python3 scripts/collapse/estimate_collapse.py "$d/.$name" --every 1000 --spots 0.5 --moves "$mv" --methods "$me" \
          > "$d/estimate_$name.log" 2>&1 && mv "$d/.$name/estimates_local.csv" "$d/estimates_$name.csv"; rm -rf "$d/.$name" ) &
}
G8="--collapse=true --collapseFootX=39.74 --collapseFootY=39.74 --collapseDepthMin=0.1 --collapsePileH=3.658 --collapseDepthMax=3.658"
if [ "$PART" = pipe ]; then
    D="--specProfile=true --trace=true --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true"
    D="$D --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500"
    D="$D --interferenceFrameKey=true --collapse=true --gwRidge=false --baseCampFrac=0 --searchLaunchAtS=1 --relaySlots=false"
    D="$D --duration=1800 --collapseFootX=39.74 --collapseFootY=39.74 --collapseDepthMin=0.1 --collapseDepthSolve=true"
    D="$D --uwbFutilityCycles=4 --collapseBracket=central --collapseNominalHeights=true"
    declare -A PILE=([5]=2.286 [8]=3.658)
    declare -A BOX=([5]=71.22 [8]=94.08)
    crew() { echo "--nTags=$1 --measuredSubjects=$1 --identityQueryManifests=$(seq -s, 1 "$1") --finderTasking=need"; }
    for seed in $(seq 821 825); do
        for st in 5 8; do
            G="$D --collapsePileH=${PILE[$st]} --collapseDepthMax=${PILE[$st]} --boxHalf=${BOX[$st]} --seed=$seed"
            # shellcheck disable=SC2046,SC2086
            {
            cell "$OUT/s${st}_g/seed$seed"      $G $(crew 20) --nGateways=4 --arms=ca_tbl_full,static_868,static_2p4,static_uwb,poucet_fusion
            cell "$OUT/s${st}_rubble/seed$seed" $G $(crew 20) --nGateways=4 --arms=ca_tbl_full --tofRubbleCorrect=true
            cell "$OUT/s${st}_muller/seed$seed" $G $(crew 20) --nGateways=4 --arms=ca_tbl_full,static_2p4 --tofErrorModel=muller
            cell "$OUT/s${st}_st0/seed$seed"    $G $(crew 20) --nGateways=0 --arms=ca_tbl_full
            cell "$OUT/s${st}_st2/seed$seed"    $G $(crew 20) --nGateways=2 --arms=ca_tbl_full
            cell "$OUT/s${st}_n10/seed$seed"    $G $(crew 10) --nGateways=4 --arms=ca_tbl_full
            cell "$OUT/s${st}_n1/seed$seed"     $G $PIN_ONE_WORKER --nGateways=4 --arms=ca_tbl_full,static_868,static_2p4
            }
        done
    done
    wait
elif [ "$PART" = field ]; then
    SRC=${COLLAPSE5_FIELD_SRC:-$HOME/catbl_work/campaigns/collapse2_field}
    for br in low central high; do
        for seed in $(seq 901 910); do
            s="$SRC/field_$br/seed$seed"; d="$OUT/reuse_$br/seed$seed"
            [ "$(cat "$s/rc" 2>/dev/null)" = 0 ] || continue
            [ -d "$d" ] || { mkdir -p "$d" && cp "$s"/obs_*.csv "$s"/truth_*.csv "$s"/run.txt "$s"/run_id.txt "$s"/rc "$d/"; }
        done
    done
    F="--specProfile=true $G8 --collapseField=true --collapseFieldVictims=20 --collapseGridStep=5 --collapseHoldHeights=1.5"
    F="$F --collapseStations=4 --collapseStationR=56.45 --collapseStationH=6 --collapseSlots=32 --collapseSlotS=60"
    F="$F --collapseGpsSigmaH=3.27 --collapseGpsSigmaV=6.63 --collapseLocalSigma=0.10"
    for seed in $(seq 901 910); do
        for br in low central; do
            # shellcheck disable=SC2086
            cell "$OUT/h15_$br/seed$seed" $F --collapseBracket=$br --seed=$seed
        done
    done
    wait
    for d in "$OUT"/reuse_*/seed* "$OUT"/h15_*/seed*; do
        [ "$(cat "$d/rc" 2>/dev/null)" = 0 ] || continue
        est "$d" serp serpentine tof,rss,joint,coop,rsstri,spawn
        case "$d" in */reuse_*) est "$d" dopt dopt joint,coop;; esac
    done
    wait
fi
echo "$(date -u +%FT%TZ) done $PART" >> "$OUT/RUN.md"
