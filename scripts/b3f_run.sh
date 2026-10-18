#!/usr/bin/env bash
# B3F-FOLIAGE runner (report/PREREG_B3F_FOLIAGE.md). usage: bash scripts/b3f_run.sh <outdir> (outside the repo)
set -u
cd "$(dirname "$0")/.." || exit 2
OUT=${1:?usage}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: inside the repository"; exit 2;; esac
[ -e "$OUT" ] && { echo "refusing: $OUT exists"; exit 2; }
mkdir -p "$OUT/bin"; cp build/scratch/ns3.47-ca_tbl_causal_sim-optimized "$OUT/bin/" && cp -r build/lib "$OUT/bin/lib" || exit 2
( cd "$OUT/bin" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so > ../build.sha256 )
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
for s in 1 2 3 4 5; do for c in false true; do
    d="$OUT/clamp_$c/s$s"; mkdir -p "$d"
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch"; exit 3; }
    ( "$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized" --probe=true --seed=$s --foliageDepthClamp=$c --outdir="$d" > "$d/run.log" 2>&1; rc=$?; echo "$rc" > "$d/rc" ) &
done; done
wait
