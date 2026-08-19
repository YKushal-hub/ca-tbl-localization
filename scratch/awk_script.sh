#!/bin/bash
cd ~/ns/ns-3-dev
for d in results/causal results/causal_nosearch; do
  echo "== $d"
  awk -F, 'NR==1{for(i=1;i<=NF;i++)h[$i]=i} $1=="static_uwb"{print "totalJ="$h["totalJ"], "rx_uwb="$h["rx_uwb"], "blind="$h["blindCycles"]}' $d/arm_summary.csv
done
