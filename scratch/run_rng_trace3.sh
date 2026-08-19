#!/bin/bash
cd ~/ns/ns-3-dev
./ns3 build 2>&1 | tail -20
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/causal" 2>&1 | tail -5

for a in static_868 static_2p4 static_uwb fixed_threshold ca_tbl_no_integrity ca_tbl_full; do
  printf "%-22s " $a
  sed -n '2p' results/causal/trace_${a}_seed7.csv | cut -d, -f34,35
done
