cd /home/kushal/ns/ns-3-dev
export LD_LIBRARY_PATH=build/lib
# STANDING RULE: uniform 4-field job lines.
job() {
  d=results/tracegw9/seed_$1; mkdir -p "$d"
  ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --seed=$1 --boxHalf=3000 --gwGrid=9 --duration=3600 \
    --outdir="$d" > "$d/run.log" 2>&1
  echo "tgw9 s=$1 $2 $3"
}
export -f job
{ for s in $(seq 1 20); do echo "$s . . ."; done; } | xargs -P 10 -n 4 bash -c 'job "$0" "$1" "$2"'
echo "TRACEGW9 DONE"
