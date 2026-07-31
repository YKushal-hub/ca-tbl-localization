cd /home/kushal/ns/ns-3-dev
export LD_LIBRARY_PATH=build/lib
# STANDING RULE: every job line is EXACTLY 4 fields. xargs -n groups by token, not
# by line, so a short line silently swallows the next job's fields.
# fields: <fastDemote> <multiStepDemote> <seed> <tag>
job() {
  d=results/ms_$4/seed_$3; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed=$3 --boxHalf=3000 --duration=3600 \
    --fastDemote=$1 --multiStepDemote=$2 --trace=false \
    --outdir="$d" > "$d/run.log" 2>&1
  echo "ms $4 s=$3"
}
export -f job
{
  for s in $(seq 1 20); do echo "off proposed $s c1_msonly"; done
  for s in $(seq 1 20); do echo "proposed proposed $s c2_both"; done
  for s in $(seq 1 20); do echo "all all $s c3_fair"; done
} | xargs -P 10 -n 4 bash -c 'job "$0" "$1" "$2" "$3"'
echo "MULTISTEP DONE"
