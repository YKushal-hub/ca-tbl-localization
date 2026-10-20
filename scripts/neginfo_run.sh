#!/usr/bin/env bash
# NEGINFO runner (report/PREREG_NEGINFO.md). usage: bash scripts/neginfo_run.sh <outdir> W
# The endgame preset as it stands plus the window and --traceNegInfo; the argument records the base in RUN.md.
# Committed with the registration, before any cell; not edited after the first cell.
set -u
cd "$(dirname "$0")/.." || exit 2
OUT=${1:?usage}
BASE=${2:?usage: base N or W}
case "$BASE" in W) ;; *) echo "base must be W"; exit 2;; esac
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: inside the repository"; exit 2;; esac
if [ -e "$OUT/build.sha256" ]; then
    # RESUME (after an interruption): the same frozen binary, verified; finished cells are skipped, partial
    # ones are deleted and re-run by cell(). No cell is chosen or dropped.
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "refusing resume: manifest mismatch"; exit 2; }
    head -1 "$OUT/RUN.md" | grep -q "; base $BASE\$" || { echo "refusing resume: launched with another base"; exit 2; }
    echo "$(date -u +%FT%TZ) resume; source $(git rev-parse --short HEAD) dirty $(git status --porcelain | wc -l)" >> "$OUT/RUN.md"
else
    [ -e "$OUT" ] && { echo "refusing: $OUT exists without a manifest"; exit 2; }
    dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
    [ "$dirty" -eq 0 ] || { echo "refusing: $dirty tracked modification(s) under src/ scratch/"; exit 2; }
    mkdir -p "$OUT/bin"; cp build/scratch/ns3.47-ca_tbl_causal_sim-optimized "$OUT/bin/" && cp -r build/lib "$OUT/bin/lib" || exit 2
    ( cd "$OUT/bin" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so > ../build.sha256 )
    echo "$(date -u +%FT%TZ) launch; source $(git rev-parse --short HEAD) dirty $(git status --porcelain | wc -l); base $BASE" > "$OUT/RUN.md"
fi
BIN="$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=${NEGINFO_JOBS:-$(nproc 2>/dev/null || echo 4)}
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
C="--specProfile=true --duration=1800 --trace=true --traceCovariance=true --unknownStart=true --fixedVarThreshold=12 --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2 --imuFilter=ekf5 --imuMagSigmaDeg=3 --ekf5FinderConsider=true --ekf5RelayGate=true --reportAirtime=true --traceNegInfo=true --arms=ca_tbl_full"
declare -A BS
BS[N]="--ekf5Window=60 --t2SensitivityRowKHz=812.5 --publishAfterStep=true --nlosRSource=detector --foliageScaleBounds=true --interferenceDistance=true --rssiRangeFromRxPower=true --pcsByteScale=4"
BS[W]="--ekf5Window=60"
declare -A G
G[A]=""
G[B]="--boxHalf=750 --finderMode=1"
G[C]="--nGateways=8 --gwLayout=legacy"
for s in $(seq 221 240); do
    for cfg in B; do
        for v in P; do
            # shellcheck disable=SC2086
            cell "$OUT/${cfg}_${v}/s$s" $C ${BS[$BASE]} ${G[$cfg]} --seed=$s
        done
    done
done
wait
echo "$(date -u +%FT%TZ) done" >> "$OUT/RUN.md"
