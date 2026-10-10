#!/usr/bin/env bash
# master_reset.sh
# Complete bottom-to-top regeneration pipeline
#
# WHAT IT DELETES, before regenerating each one:
#   results/geometric_training_dataset_*.csv       (line 22)
#   results/split_index_train.npy, _test.npy       (line 61)
#   results/stage2_result_M[1-4]_T*.json           (line 62)
#   results/stage2_M[1-4]_T*.pkl                   (line 62)
# Nothing else is removed. No `rm -rf`, no git operations.
#
# Runs from any clone: it resolves the repository root from its own location.
#
# Runs 10 parallel ns-3 processes and a PyTorch training stage; expect hours.
set -euo pipefail
cd "$(dirname "$0")/.." || exit 1   # scratch/ -> repository root

log() { echo "[$(date '+%H:%M:%S')] $*"; }

log "=== STEP 1: Topology Observability Simulation (PARALLEL) ==="
rm -f results/geometric_training_dataset_*.csv
pids=()
for i in {0..9}; do
    start=$((i * 100))
    end=$(((i + 1) * 100))
    ./ns3 run "scratch/topology_observability_sim --startRun=$start --endRun=$end" &
    pids+=($!)
done

log "Waiting for 10 parallel processes to complete..."
for pid in "${pids[@]}"; do
    wait $pid
done

log "Merging CSVs..."
head -n 1 results/geometric_training_dataset_0_100.csv > results/geometric_training_dataset.csv
for i in {0..9}; do
    start=$((i * 100))
    end=$(((i + 1) * 100))
    tail -n +2 results/geometric_training_dataset_${start}_${end}.csv >> results/geometric_training_dataset.csv
done

log "Generating topology rates summary JSON..."
python3 scratch/generate_rates.py

log "=== STEP 2: TDOA LM Solver ==="
python3 process_tdoa.py

log "=== STEP 3: Fix Dataset (WAN error sharing) ==="
python3 scratch/fix_dataset.py

log "=== STEP 4: Regenerate M1-M4 multimodal datasets (12 CSVs) ==="
# multimodal_generator.py will now merge ml_training_dataset with tdoa_processed_dataset_fixed
python3 scratch/multimodal_generator.py

log "=== STEP 5: Generate M0 baseline datasets (3 CSVs) ==="
python3 scratch/generate_method0.py

log "=== STEP 6: Clear split index + retrain ==="
rm -f results/split_index_train.npy results/split_index_test.npy
rm -f results/stage2_result_M[1-4]_T*.json results/stage2_M[1-4]_T*.pkl
bash scratch/launch_retrain.sh

log "=== STEP 7: Final validation ==="
python3 scratch/validate_final.py

log "=== PIPELINE COMPLETE ==="
