#!/usr/bin/env bash
# REALISTIC-5 runner (report/PREREG_REALISTIC5.md). usage: bash scripts/realistic5_run.sh <outdir> (outside the repo)
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
J=${REALISTIC5_JOBS:-6}
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
# REALISTIC-4's emergency cells and REALISTIC-2's quiet shift on NEW seeds, every design, with lost filters re-entering
# particle acquisition (DD 34, --reacquireSigmaM=500). Everything else as REALISTIC-4: device-side estimation, mast-height
# correction, the search party fixed (DD 30-32 + --finderStaleSearch), the side of the road known on ROAD sites (DD 29).
D="--specProfile=true --trace=true --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true"
D="$D --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500"
E="$D --duration=7200 --trailDeviateAtS=1800 --trailStopAfterS=600 --searchLaunchOnLoss=0 --searchLaunchAtS=3000"
Q="$D --duration=28800 --searchLaunchOnLoss=0 --nFinders=0 --finderTasking=tag0 --finderSessions=false"
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
    esac
}
for i in $(seq 0 19); do
    s=$((561 + i))
    for st in ROAD1 ROAD3 CLUS3 RAND3 ROAD3F4; do
        for v in ARMS TII2; do
            # shellcheck disable=SC2086
            cell "$OUT/E_${st}_${v}/s$s" $E $(site $st $s) ${V[$v]} --seed=$s
        done
    done
done
for i in $(seq 0 9); do
    s=$((581 + i))
    for st in ROAD1 ROAD3 CLUS3 RAND3; do
        for v in ARMS TII2; do
            # shellcheck disable=SC2086
            cell "$OUT/Q_${st}_${v}/s$s" $Q $(site $st $s) ${V[$v]} --seed=$s
        done
    done
done
wait
echo "$(date -u +%FT%TZ) done" >> "$OUT/RUN.md"
