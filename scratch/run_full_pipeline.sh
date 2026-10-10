#!/usr/bin/env bash
# run_full_pipeline.sh
# Full regeneration pipeline after topology_observability_sim produces rates.
# Run from anywhere; it changes to the repository root itself.
#
# Steps:
#   1. Verify topology_rates_summary.json exists
#   2. Regenerate M1-M4 multimodal datasets (12 CSVs)
#   3. Generate M0 baseline datasets (3 CSVs)
#   4. Clear old split index + retrain all 12 Stage-2 models
#   5. Run final validation

set -euo pipefail
cd "$(dirname "$0")/.." || exit 1   # scratch/ -> repository root (the old ../.. went one level too far)

log() { echo "[$(date '+%H:%M:%S')] $*"; }

# ── Step 1: verify topology rates ─────────────────────────────────────────
log "Step 1: Checking topology_rates_summary.json..."
if [ ! -f results/topology_rates_summary.json ]; then
    log "ERROR: results/topology_rates_summary.json not found."
    log "  Run the simulator first:"
    log "  ./ns3 run 'topology_observability_sim --numNodes=500 --numGateways=10 --areaSideKm=50 --runs=100'"
    exit 1
fi
python3 -c "
import json
d = json.load(open('results/topology_rates_summary.json'))
print(f'  WAN={d[\"WAN\"]:.4f}  MESH={d[\"MESH\"]:.4f}  HYBRID={d[\"HYBRID\"]:.4f}  runs={d[\"runs\"]}')
assert d['runs'] >= 10, 'Too few MC runs — rerun simulator with --runs=100'
print('  OK')
"

# ── Step 2: regenerate M1-M4 (12 CSVs) ────────────────────────────────────
log "Step 2: Regenerating M1-M4 multimodal datasets (12 CSVs)..."
log "  This will overwrite existing multimodal_M[1-4]_T*.csv files."
python3 scratch/multimodal_generator.py
log "  M1-M4 generation complete."

# ── Step 3: generate M0 baseline (3 CSVs) ─────────────────────────────────
log "Step 3: Generating M0 baseline datasets (3 CSVs)..."
python3 scratch/generate_method0.py
log "  M0 generation complete."

# ── Step 4: clear split index + retrain ───────────────────────────────────
log "Step 4: Clearing old split index and retraining..."
rm -f results/split_index_train.npy results/split_index_test.npy
log "  Split index cleared (will be recreated by first variant)."
log "  Starting parallel retraining of 12 Stage-2 models..."
# Remove old model results to force re-run
rm -f results/stage2_result_M[1-4]_T*.json results/stage2_M[1-4]_T*.pkl
bash scratch/launch_retrain.sh
log "  Retraining complete."

# ── Step 5: final validation ───────────────────────────────────────────────
log "Step 5: Running final validation..."
python3 scratch/validate_final.py
log "  Validation complete."

log "Pipeline finished. Check results/final_comparison_table.csv"
