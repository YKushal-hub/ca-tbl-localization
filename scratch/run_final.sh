#!/bin/bash
cd ~/ns/ns-3-dev

echo "=== 1. FULL ARM TABLE (searchDuty=1.0) ==="
column -s, -t < results/causal/arm_summary.csv | cut -c1-200

echo "=== 2. TRACE DIVERGENCE DIFF ==="
diff <(cut -d, -f1,4-15,17-33 results/causal/trace_ca_tbl_full_seed7.csv) \
     <(cut -d, -f1,4-15,17-33 results/causal/trace_ca_tbl_no_integrity_seed7.csv) | head -12 || true

echo "=== 3. MAC_TX AND MAC_COLLISIONS ==="
awk -F, 'NR==1{for(i=1;i<=NF;i++)h[$i]=i} NR>1{print $1, "mac_tx="$h["mac_tx"], "mac_collisions="$h["mac_collisions"]}' results/causal/arm_summary.csv | column -t

echo "=== 4. VERIFIER CHECKS ==="
python3 scratch/python_handoff/verify_causal_sim.py results/causal > /tmp/ver.txt 2>&1
head -n 5 /tmp/ver.txt

echo "=== 5. REBUILD AND RE-PROBE ==="
./ns3 build 2>&1 | tail -40
./ns3 run "ca_tbl_causal_sim --probe=true" 2>&1 | tail -120
