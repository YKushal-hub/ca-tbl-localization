#!/usr/bin/env bash
# PARETO-3: copy of pareto1_dd54_run.sh; adds --peerSplitCi --sharedDuty868 --dutyReport868 (DD 56, 57), new seeds (report/PREREG_PARETO34.md).
# usage: bash scripts/pareto3_run.sh <outdir> (outside the repo)
# Committed with the registration, before any cell; not edited after the first cell.
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh
OUT=${1:?usage}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: inside the repository"; exit 2;; esac
if [ -e "$OUT/build.sha256" ]; then
    # RESUME (after an interruption): the same frozen binary, verified; finished cells are skipped, partial
    # ones are deleted and re-run by cell(). No cell is chosen or dropped.
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
J=${PARETO3_JOBS:-6}  # 8 GB laptop: ~0.5 GB per run
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
C="--specProfile=true $PIN_PRE_DD27 --sparseRangeGeometry=true --peerSplitCi=true --sharedDuty868=true --dutyReport868=true --duration=1800 --trace=true --reportAirtime=true"
declare -A G
G[A]=""
G[B]="--boxHalf=750 --finderMode=1"
declare -A V
V[BASE]="--arms=ca_tbl_full,fixed_threshold,static_868,static_2p4,lora_fusion,poucet_fusion"
for t in 3 30 100 300; do V[FT$t]="--arms=fixed_threshold --fixedVarThreshold=$t"; done
V[OPP]="--arms=ca_tbl_full --c2bEscalation=true"
V[D868]="--arms=fixed_threshold --dualBase868OnNeed=true"
for l in 0.01 0.1 1 10 100 1000; do V[BEN$l]="--arms=fixed_threshold --benefitLambda=$l --benefitSkip868=true"; done
V[BNS100]="--arms=fixed_threshold --benefitLambda=100"
for s in $(seq 1301 1320); do
    for cfg in A B; do
        for v in BASE FT3 FT30 FT100 FT300 OPP D868 BEN0.01 BEN0.1 BEN1 BEN10 BEN100 BEN1000 BNS100; do
            # shellcheck disable=SC2086
            cell "$OUT/${cfg}_${v}/s$s" $C ${G[$cfg]} ${V[$v]} --seed=$s
        done
    done
done
wait
echo "$(date -u +%FT%TZ) done" >> "$OUT/RUN.md"
