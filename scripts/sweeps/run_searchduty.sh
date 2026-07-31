cd /home/kushal/ns/ns-3-dev
export LD_LIBRARY_PATH=build/lib
for s in 0.0 0.1 0.25 0.5 1.0; do
  mkdir -p results/sd_$s
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
     --seed=7 --searchDuty=$s --duration=3600 --outdir=results/sd_$s --trace=false \
     > results/sd_$s/run.log 2>&1 &
done
wait
echo "SEARCHDUTY SWEEP DONE"
