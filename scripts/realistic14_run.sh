#!/usr/bin/env bash
# REALISTIC-14 runner (report/PREREG_REALISTIC14.md). usage: bash scripts/realistic14_run.sh <outdir> (outside the repo)
# REALISTIC-11's emergency cells (copied from realistic11_run.sh) with --finderDigestBothLora=true (DD 78) for every design, seeds 2301-2320. Guarded-skip (TII2) cells dropped: no hypothesis uses them, and C: is nearly full.
# Committed with the registration, before any cell; not edited after the first cell.
set -u
cd "$(dirname "$0")/.." || exit 2
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
    cp -r scripts/layouts/realistic "$OUT/bin/layouts" || exit 2
    ( cd "$OUT/bin" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so layouts/*.csv > ../build.sha256 )
    echo "$(date -u +%FT%TZ) launch; source $(git rev-parse --short HEAD) dirty $(git status --porcelain | wc -l)" > "$OUT/RUN.md"
fi
BIN="$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized"
LAY="$OUT/bin/layouts"
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=${REALISTIC14_JOBS:-8}
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    f=$(df -m /mnt/c | awk 'END{print $4}'); [ "$f" -lt 1000 ] && { echo "$(date -u +%FT%TZ) stop: C: $f MB free" >> "$OUT/RUN.md"; wait; exit 4; }
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    # Kept per run (C: is nearly full): the files realistic14_analyse.py reads, as campaigns/prune_runs.sh keeps them.
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; r=$?
      find "$d" -maxdepth 1 -type f ! -name 'trace_*' ! -name 'nodes_*' ! -name 'subjtrace_*' ! -name subject_summary.csv \
           ! -name arm_summary.csv ! -name run.log ! -name run_id.txt ! -name deployment.json ! -name spec_profile.txt -delete
      touch "$d/.pruned"; echo $r > "$d/rc" ) &
}
# REALISTIC-5's emergency cells on NEW seeds with a different incident on every seed (DD 35, --incidentRandom), plus
# crews of 10 and 15 on ROAD3 (relay slots off: the slot schedule refuses crews above ~7) and the field-measured 2.4 GHz
# error law with every fix on. Every run: REALISTIC-5's flags (device-side estimation, mast-height correction, the
# search party fixed, --finderStaleSearch, --reacquireSigmaM=500, the side of the road known on ROAD sites).
D="--specProfile=true --trace=true --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true"
D="$D --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500"
E="$D --duration=7200 --trailDeviateAtS=1800 --trailStopAfterS=600 --searchLaunchOnLoss=0 --searchLaunchAtS=3000 --incidentRandom=true"
declare -A V
V[ARMS]="--arms=ca_tbl_full,fixed_threshold,static_868,static_2p4,poucet_fusion"
V[TII2]="--arms=fixed_threshold --benefitLambda=1000 --benefitSkip868=true --benefitSkip868Min3=true"
site() {  # site args for seed $2
    local k=$(( $2 % 10 ))
    case $1 in
        ROAD1) echo "--nGateways=1 --acquireSideOfRoad=true";;
        ROAD3) echo "--acquireSideOfRoad=true";;
        CLUS3) echo "--gwRidge=false --gwLayout=cluster --nGateways=3 --env=0 --vegWide=0";;
        RAND3) echo "--gwRidge=false --gwLayout=file --gwLayoutFile=$LAY/rand_n3_k$k.csv --nGateways=3";;
        ROAD3F4) echo "--acquireSideOfRoad=true --nFinders=4";;
        CREW10) echo "--acquireSideOfRoad=true --measuredSubjects=10 --nTags=10 --relaySlots=false";;
        CREW15) echo "--acquireSideOfRoad=true --measuredSubjects=15 --nTags=15 --relaySlots=false";;
        ROAD3MUL) echo "--acquireSideOfRoad=true --tofErrorModel=muller";;
        CLUS3MUL) echo "--gwRidge=false --gwLayout=cluster --nGateways=3 --env=0 --vegWide=0 --tofErrorModel=muller";;
    esac
}
G="--sparseRangeGeometry=true --peerSplitCi=true --sharedDuty868=true --dutyReport868=true --ekf5WindowThetaWalk=true --sharedDuty868Finders=true --finderDigestBothLora=true"
# no quiet shift: it has no searchers, so DD 78 cannot act there
for i in $(seq 0 19); do
    s=$((2301 + i))
    for st in ROAD1 ROAD3 CLUS3 RAND3 ROAD3F4; do
        # shellcheck disable=SC2086
        cell "$OUT/E_${st}_ARMS/s$s" $E $(site $st $s) ${V[ARMS]} $G --seed=$s
    done
    for st in ROAD3MUL CLUS3MUL CREW10 CREW15; do
        # shellcheck disable=SC2086
        cell "$OUT/E_${st}_ARMS/s$s" $E $(site $st $s) ${V[ARMS]} $G --seed=$s
    done
done
wait
echo "$(date -u +%FT%TZ) done" >> "$OUT/RUN.md"
