#!/usr/bin/env bash
# REALISTIC-13 runner (report/PREREG_REALISTIC13.md): quiet shift, three vs five random stations. usage: bash scripts/realistic13_run.sh <outdir> (outside the repo)
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
BIN="$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized"; LAY="$OUT/bin/layouts"
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=${REALISTIC13_JOBS:-8}
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
# The DD 67 preset (DD 54/56/57/59/60/62/63 on) with REALISTIC-11's quiet-shift flags.
D="--specProfile=true --trace=true --gatewayEdgeServer=false --edgeUplinkRanges=false --subjectEdgeUplink=false --rangeHeightCorrect=true"
D="$D --finderSweepFromDispatch=true --finderDigestOnArmTier=true --finderLkpFirst=true --finderStaleSearch=true --reacquireSigmaM=500"
Q="$D --duration=28800 --searchLaunchOnLoss=0 --nFinders=0 --finderTasking=tag0 --finderSessions=false --arms=ca_tbl_full"
# Station count as a deployment choice: the same random-site generator, 3 or 5 stations (scripts/layouts/realistic/rand_n{3,5}_k*.csv).
for s in $(seq 2001 2020); do
    k=$(( s % 10 ))
    for n in 3 5; do
        cell "$OUT/Q_RAND${n}/s$s" $Q --gwRidge=false --gwLayout=file --gwLayoutFile=$LAY/rand_n${n}_k$k.csv --nGateways=$n --seed=$s
    done
done
wait
echo "$(date -u +%FT%TZ) done" >> "$OUT/RUN.md"
