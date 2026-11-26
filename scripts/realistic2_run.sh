#!/usr/bin/env bash
# REALISTIC-2 runner (report/PREREG_REALISTIC2.md). usage: bash scripts/realistic2_run.sh <outdir> (outside the repo)
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
J=${REALISTIC2_JOBS:-6}
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
# As REALISTIC-1: device-side estimation and mast-height correction on every run (DESIGN_DECISIONS 28).
D="--specProfile=true --trace=true --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true"
E="$D --duration=7200 --trailDeviateAtS=1800 --trailStopAfterS=600 --searchLaunchOnLoss=0 --searchLaunchAtS=3000"
Q="$D --duration=28800 --searchLaunchOnLoss=0 --nFinders=0 --finderTasking=tag0 --finderSessions=false"
declare -A V
V[ARMS]="--arms=ca_tbl_full,fixed_threshold,static_868,static_2p4,poucet_fusion"
V[TII2]="--arms=fixed_threshold --benefitLambda=1000 --benefitSkip868=true --benefitSkip868Min3=true"
site() {  # site args for seed $2
    local k=$(( $2 % 10 ))
    case $1 in
        RAND3) echo "--gwRidge=false --gwLayout=file --gwLayoutFile=$LAY/rand_n3_k$k.csv --nGateways=3";;
        RAND5) echo "--gwRidge=false --gwLayout=file --gwLayoutFile=$LAY/rand_n5_k$k.csv --nGateways=5";;
        ROAD5) echo "--nGateways=5";;
        ROAD3F4) echo "--nFinders=4";;
        ROAD1) echo "--nGateways=1";;
        ROAD3) echo "";;
        CLUS3) echo "--gwRidge=false --gwLayout=cluster --nGateways=3 --env=0 --vegWide=0";;
    esac
}
for s in $(seq 521 540); do
    for st in RAND3 RAND5 ROAD5 ROAD3F4; do
        for v in ARMS TII2; do
            # shellcheck disable=SC2086
            cell "$OUT/E_${st}_${v}/s$s" $E $(site $st $s) ${V[$v]} --seed=$s
        done
    done
done
for s in $(seq 541 550); do
    for st in ROAD1 ROAD3 CLUS3 RAND3; do
        for v in ARMS TII2; do
            # shellcheck disable=SC2086
            cell "$OUT/Q_${st}_${v}/s$s" $Q $(site $st $s) ${V[$v]} --seed=$s
        done
    done
done
wait
echo "$(date -u +%FT%TZ) done" >> "$OUT/RUN.md"
