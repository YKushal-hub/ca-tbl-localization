#!/usr/bin/env bash
# ENV-SWEEP runner (report/PREREG_ENV_SWEEP.md). usage: bash scripts/envsweep_run.sh <outdir> (outside the repo)
# Committed with the registration, before any cell; not edited after the first cell.
set -u
cd "$(dirname "$0")/.." || exit 2
OUT=${1:?usage}
FROZEN=${ENVSWEEP_FROZEN:-$HOME/ns/catbl-work/preset5_2026-10-22}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: inside the repository"; exit 2;; esac
if [ -e "$OUT/build.sha256" ]; then
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "refusing resume: manifest mismatch"; exit 2; }
    echo "$(date -u +%FT%TZ) resume; source $(git rev-parse --short HEAD) dirty $(git status --porcelain | wc -l)" >> "$OUT/RUN.md"
else
    [ -e "$OUT" ] && { echo "refusing: $OUT exists without a manifest"; exit 2; }
    ( cd "$FROZEN/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "refusing: frozen PRESET-5 binary fails its manifest"; exit 2; }
    mkdir -p "$OUT/bin"; cp "$FROZEN/bin/ns3.47-ca_tbl_causal_sim-optimized" "$OUT/bin/" && cp -r "$FROZEN/bin/lib" "$OUT/bin/lib" || exit 2
    ( cd "$OUT/bin" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so > ../build.sha256 )
    cmp "$OUT/build.sha256" "$FROZEN/build.sha256" || { echo "refusing: copy differs from the frozen manifest"; exit 2; }
    echo "$(date -u +%FT%TZ) launch; source $(git rev-parse --short HEAD) dirty $(git status --porcelain --untracked-files=no | wc -l); binary = PRESET-5 frozen (8e92fe3a)" > "$OUT/RUN.md"
fi
BIN="$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=${ENVSWEEP_JOBS:-$(nproc 2>/dev/null || echo 4)}
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
declare -A G
G[A]=""
G[B]="--boxHalf=750 --finderMode=1"
for s in $(seq 361 380); do
    for cfg in A B; do
        for e in 0 1 2 3; do
            # shellcheck disable=SC2086
            cell "$OUT/${cfg}_E${e}/s$s" $C ${G[$cfg]} --env=$e --seed=$s
        done
    done
done
wait
echo "$(date -u +%FT%TZ) done" >> "$OUT/RUN.md"
