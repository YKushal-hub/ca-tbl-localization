#!/bin/bash
# usage: runcell.sh <cell> <seed> <extra flags...>
cell=$1; seed=$2; shift 2
export LD_LIBRARY_PATH=${LIBS:-$PWD/../../build/lib}
BIN=${BIN:-$PWD/../../build/scratch/ns3.47-ca_tbl_causal_sim-optimized}
d=${RUNS:-runs}/$cell/s$seed
[ -f "$d/rc" ] && exit 0
rm -rf "$d"; mkdir -p "$d"
CATBL_RANGELOG=1 $BIN --specProfile=true --duration=1800 --trace=true --seed=$seed --outdir=$d "$@" > $d/run.log 2>&1
echo $? > $d/rc
