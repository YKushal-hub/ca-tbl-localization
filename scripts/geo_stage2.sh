#!/usr/bin/env bash
# GEO-01 stage 2 runner (report/PREREG_GEO01.md). Every cell of the registered grid:
# the ten pruned candidates x seeds 1-10 x 1 800 s, arms static_868 and ca_tbl_full, under
# --specProfile with the candidate's geometry given explicitly (the preset minus geometry).
#
# usage: bash scripts/geo_stage2.sh <outdir>      (the outdir must not exist)
set -u
cd "$(dirname "$0")/.." || exit 2
OUT=${1:?usage: geo_stage2.sh <outdir>}
[ -e "$OUT" ] && { echo "refusing: $OUT exists (never write over a results directory)"; exit 2; }
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
mkdir -p "$OUT"
J=$(nproc 2>/dev/null || echo 4)
while read -r cand flags; do
    [ -z "$cand" ] && continue
    for s in 1 2 3 4 5 6 7 8 9 10; do
        while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
        d="$OUT/raw/$cand/s$s"
        mkdir -p "$d"
        # shellcheck disable=SC2086
        ( "$BIN" --arms=static_868,ca_tbl_full --seed=$s --duration=1800 --trace=true \
              --specProfile=true $flags --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
    done
done <<'LIST'
N3_star --nGateways=3 --gwLayout=star
N3_cluster --nGateways=3 --gwLayout=cluster
N4_arc90 --nGateways=4 --gwArcDeg=90 --gwArcCentreDeg=180
N4_star --nGateways=4 --gwLayout=star
N5_arc90 --nGateways=5 --gwArcDeg=90 --gwArcCentreDeg=180
N5_arc140 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180
N6_arc180 --nGateways=6 --gwArcDeg=180 --gwArcCentreDeg=180
N6_arc240 --nGateways=6 --gwArcDeg=240 --gwArcCentreDeg=180
N8_ring --nGateways=8
N8_arc240 --nGateways=8 --gwArcDeg=240 --gwArcCentreDeg=180
LIST
wait
python3 scripts/geo_stage2_analyse.py "$OUT"
