cd /home/kushal/ns/ns-3-dev
export LD_LIBRARY_PATH=build/lib
# Phase B needs per-cycle traces; the 40-run sweep used --trace=false.
# Runs are deterministic and the caches are inert (cache-independence test), so
# arm_summary.csv must come back byte-identical to the untraced sweep.
run_one() {
  s=$1
  d=results/trace3000/seed_$s
  mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
     --seed=$s --boxHalf=3000 --duration=3600 --outdir="$d" \
     > "$d/run.log" 2>&1
  echo "done s=$s"
}
export -f run_one
seq 1 20 | xargs -P 10 -n 1 bash -c 'run_one "$0"'
echo "TRACE3000 DONE"
