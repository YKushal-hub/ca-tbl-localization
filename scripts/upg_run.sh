#!/usr/bin/env bash
# UPG-SCOPE runner (report/PREREG_UPG_SCOPE.md). usage: bash scripts/upg_run.sh <outdir> (outside the repo)
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
    mkdir -p "$OUT/bin"; cp build/scratch/ns3.47-ca_tbl_causal_sim-optimized "$OUT/bin/" && cp -r build/lib "$OUT/bin/lib" || exit 2
    ( cd "$OUT/bin" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so > ../build.sha256 )
    echo "$(date -u +%FT%TZ) launch; source $(git rev-parse --short HEAD) dirty $(git status --porcelain | wc -l)" > "$OUT/RUN.md"
fi
BIN="$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
C="--specProfile=true --duration=1800 --trace=true --traceCovariance=true --traceSubjectConsistency=true --imuQuScale=10 --unknownStart=true --fixedVarThreshold=12 --arms=static_868,static_2p4,ca_tbl_full,fixed_threshold"
declare -A V
V[BASE]=""
V[C1]="--acquireNegInfo=true"
V[C2]="--tofRecal=true"
V[C3]="--hold24AfterK=3"
V[B1]="--relayNisInflate=true"
V[B7]="--fadingPerBand=true"
V[WBASE]="--tofErrorModel=wolf"
V[WC5]="--tofErrorModel=wolf --tofCalibrate=true"
declare -A G
G[A]=""
G[B]="--boxHalf=750 --finderMode=1"
for s in $(seq 1 20); do
    for cfg in A B; do
        for v in BASE C1 C2 C3 B1 B7 WBASE WC5; do
            # shellcheck disable=SC2086
            cell "$OUT/${cfg}_${v}/s$s" $C ${G[$cfg]} ${V[$v]} --seed=$s
        done
    done
done
wait
echo "$(date -u +%FT%TZ) all cells done" >> "$OUT/RUN.md"
