#!/usr/bin/env bash
# Resume GEO-01 stage 2 after the session ended mid-grid: cells without an rc file are
# cleared and re-run with exactly the registered command line (scripts/geo_stage2.sh at
# d05992b). Finished cells are not touched. The simulation is deterministic (a finished
# cell re-run reproduced all 71 files byte for byte), so this equals an uninterrupted run.
set -u
cd /home/nethi/projects/ca-tbl-localization || exit 2
OUT=results/geo01_2026-10-03
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
J=$(nproc 2>/dev/null || echo 4)
while read -r cand flags; do
    [ -z "$cand" ] && continue
    for s in 1 2 3 4 5 6 7 8 9 10; do
        d="$OUT/raw/$cand/s$s"
        [ -f "$d/rc" ] && continue
        rm -rf "$d"
        while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
        mkdir -p "$d"
        echo "$cand s$s" >> "$OUT/resumed_cells.txt"
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
