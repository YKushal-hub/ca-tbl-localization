#!/bin/bash
cd ~/ns/ns-3-dev

echo "=== fdgw9 seeds ==="
ls results/fdgw9_all_b3000/
echo "---"
ls results/fdgw9_proposed_b3000/

echo "=== fd_all seeds ==="
ls results/fd_all_b3000/ | head -25

echo "=== fd_proposed seeds ==="
ls results/fd_proposed_b3000/ | head -25

echo "=== box sweep ==="
for d in results/box_*/; do
  printf "%s: " "$d"
  grep ca_tbl_full "$d/arm_summary.csv" | cut -d, -f1,5,12
done

echo "=== sd sweep ==="
for d in results/sd_*/; do
  printf "%s: " "$d"
  grep static_uwb "$d/arm_summary.csv" | cut -d, -f1,12,51
done

echo "=== trace cols ==="
head -1 results/causal/trace_ca_tbl_full_seed7.csv | tr ',' '\n' | cat -n
