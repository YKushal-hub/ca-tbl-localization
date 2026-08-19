#!/bin/bash
cd ~/ns/ns-3-dev
./ns3 build 2>&1 | tail -20
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/causal" 2>&1 | tail -5

for a in ca_tbl_full ca_tbl_no_integrity; do
  echo "== $a"
  cut -d, -f1,17,34-37 results/causal/trace_${a}_seed7.csv | sed -n '1,5p'
done
