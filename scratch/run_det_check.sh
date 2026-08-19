#!/bin/bash
cd ~/ns/ns-3-dev

# Determinism must be re-established: RNG initialisation changed.
mkdir -p results/det_a results/det_b
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/det_a --trace=false" 2>&1 | tail -3
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/det_b --trace=false" 2>&1 | tail -3
diff results/det_a/arm_summary.csv results/det_b/arm_summary.csv && echo DETERMINISTIC

column -s, -t < results/causal/arm_summary.csv | cut -c1-160
python3 scratch/python_handoff/verify_causal_sim.py results/causal
