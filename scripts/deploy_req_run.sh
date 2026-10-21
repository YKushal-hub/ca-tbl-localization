#!/usr/bin/env bash
# DEPLOY-REQ runner (report/PREREG_DEPLOY_REQ.md). usage: bash scripts/deploy_req_run.sh <outdir> (outside the repo)
# Committed with the registration, before any cell; not edited after the first cell.
set -u
cd "$(dirname "$0")/.." || exit 2
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
J=${DEPLOYREQ_JOBS:-$(nproc 2>/dev/null || echo 4)}
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
C="--specProfile=true --duration=1800 --trace=true --traceCovariance=true --reportAirtime=true --arms=ca_tbl_full,fixed_threshold,static_868,static_2p4,poucet_fusion"
# The preset at 5 km, unchanged. Only the deployment varies: gateways (the brief's 3; an 8- or 12-gateway ring) and
# finders (8, the preset's; 12; 16; 24).
declare -A G
G[g3]=""
G[g8]="--nGateways=8 --gwLayout=legacy"
G[g12]="--nGateways=12 --gwLayout=legacy"
for s in $(seq 301 320); do
    for g in g3 g8 g12; do
        for f in 8 12 16 24; do
            # shellcheck disable=SC2086
            cell "$OUT/${g}_f${f}/s$s" $C ${G[$g]} --nFinders=$f --seed=$s
        done
    done
done
wait
echo "$(date -u +%FT%TZ) done" >> "$OUT/RUN.md"
