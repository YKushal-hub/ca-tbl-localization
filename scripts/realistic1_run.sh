#!/usr/bin/env bash
# REALISTIC-1 runner (report/PREREG_REALISTIC1.md). usage: bash scripts/realistic1_run.sh <outdir> (outside the repo)
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
J=${REALISTIC1_JOBS:-6}
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
# The incident: the lost worker leaves the route at 30 min and stops 10 min later; the alarm (overdue) is at 50 min, the
# same for every design; the run ends at 2 h.
# Every worker's device runs its own filter (owner decision 2026-11-20): no station-side estimation, so each station's
# 868 MHz reply is billed to the device (--tier1ChargeReplies, default true) and no range uplink is billed.
C="--specProfile=true --duration=7200 --trace=true --trailDeviateAtS=1800 --trailStopAfterS=600 --searchLaunchOnLoss=0 --searchLaunchAtS=3000"
C="$C --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false"
# Mast ranges projected onto the ground before the 2-D filter reads them (DD 28, G172).
C="$C --rangeHeightCorrect=true"
declare -A S
S[ROAD1]="--nGateways=1"
S[ROAD3]=""
S[CLUS3]="--gwRidge=false --gwLayout=cluster --nGateways=3 --env=0 --vegWide=0"
S[RING12]="$PIN_PRE_DD27"
declare -A F
F[F2]=""
F[F0]="--nFinders=0 --finderTasking=tag0 --finderSessions=false"
declare -A V
V[ARMS]="--arms=ca_tbl_full,fixed_threshold,static_868,static_2p4,poucet_fusion"
V[TII2]="--arms=fixed_threshold --benefitLambda=1000 --benefitSkip868=true --benefitSkip868Min3=true"
for s in $(seq 501 520); do
    for site in ROAD1 ROAD3 CLUS3 RING12; do
        for f in F2 F0; do
            for v in ARMS TII2; do
                # shellcheck disable=SC2086
                cell "$OUT/${site}_${f}_${v}/s$s" $C ${S[$site]} ${F[$f]} ${V[$v]} --seed=$s
            done
        done
    done
    # The field-measured 2.4 GHz error law (Mueller et al. 2021), realistic sites, 2 searchers, the five arms.
    for site in ROAD1 ROAD3 CLUS3; do
        # shellcheck disable=SC2086
        cell "$OUT/${site}_F2_MUL/s$s" $C ${S[$site]} ${F[F2]} ${V[ARMS]} --tofErrorModel=muller --seed=$s
    done
done
wait
echo "$(date -u +%FT%TZ) done" >> "$OUT/RUN.md"
