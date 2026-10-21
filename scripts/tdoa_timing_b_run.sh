#!/usr/bin/env bash
# TDOA-TIMING-B runner (report/PREREG_TDOA_TIMING_B.md). usage: bash scripts/tdoa_timing_b_run.sh <outdir> (outside the repo)
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
    ( cd "$OUT/bin" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so > ../build.sha256 )
    echo "$(date -u +%FT%TZ) launch; source $(git rev-parse --short HEAD) dirty $(git status --porcelain | wc -l)" > "$OUT/RUN.md"
fi
BIN="$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=${TDOA_TIMING_JOBS:-$(nproc 2>/dev/null || echo 4)}
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
C="--specProfile=true --duration=1800 --trace=true"
declare -A G
G[A]="--arms=fixed_threshold,poucet_fusion,lora_fusion,static_868,static_2p4"
G[B]="--arms=fixed_threshold,poucet_fusion,lora_fusion,static_868,static_2p4 --boxHalf=750 --finderMode=1"
G[C]="--arms=ca_tbl_full,static_868,lora_fusion --nGateways=8 --gwLayout=legacy"
declare -A V
# L: the preset. S0: perfect sync. T0: perfect sync and arrival sigma at 1 % of the bound. N: tier-1 TDoA off.
V[L]=""
V[S0]="--tdoaPpsSigmaNs=0"
V[T0]="--tdoaPpsSigmaNs=0 --tdoaToaScale=0.01"
V[N]="--tier1Tdoa=false"
for s in $(seq 361 380); do
    for g in A B C; do
        for v in L S0 T0 N; do
            # shellcheck disable=SC2086
            cell "$OUT/${g}_${v}/s$s" $C ${G[$g]} ${V[$v]} --seed=$s
        done
    done
done
wait
echo "$(date -u +%FT%TZ) done" >> "$OUT/RUN.md"
