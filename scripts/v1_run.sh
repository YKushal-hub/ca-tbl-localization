#!/usr/bin/env bash
# V1-RNGKEYED runner (report/PREREG_V1_RNGKEYED.md). usage: bash scripts/v1_run.sh <outdir> (outside the repo)
set -u
cd "$(dirname "$0")/.." || exit 2
OUT=${1:?usage}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: inside the repository"; exit 2;; esac
[ -e "$OUT" ] && { echo "refusing: $OUT exists"; exit 2; }
mkdir -p "$OUT/bin"; cp build/scratch/ns3.47-ca_tbl_causal_sim-optimized "$OUT/bin/" && cp -r build/lib "$OUT/bin/lib" || exit 2
( cd "$OUT/bin" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so > ../build.sha256 )
echo "$(date -u +%FT%TZ) launch; source $(git rev-parse --short HEAD) dirty $(git status --porcelain | wc -l)" > "$OUT/RUN.md"
BIN="$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
cell() {
    local d=$1; shift
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
P="--specProfile=true --duration=900 --unknownStart=true --fixedVarThreshold=12 --foliageDepthClamp=true --boxHalf=750 --finderMode=1 --arms=static_868,static_2p4,ca_tbl_full,fixed_threshold"
for s in $(seq 1 10); do
    for k in false true; do
        cell "$OUT/PROBE_k$k/s$s" --probe=true --seed=$s --foliageDepthClamp=true --rngKeyed=$k
        # shellcheck disable=SC2086
        cell "$OUT/PAIR_k$k/s$s" $P --rngKeyed=$k --seed=$s
    done
done
wait
echo "$(date -u +%FT%TZ) all cells done" >> "$OUT/RUN.md"
