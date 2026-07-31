cd /home/kushal/ns/ns-3-dev
export LD_LIBRARY_PATH=build/lib
# Every job line is EXACTLY 4 fields: kind arg1 arg2 arg3. xargs -n 4 groups by
# token count, not by line, so a 2-field line silently swallows the next job's
# fields -- that dropped every even seed on the first attempt.
job() {
  case "$1" in
    e0) d=results/mahalR3000/seed_$2; mkdir -p "$d"
        ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
          --seed=$2 --boxHalf=3000 --duration=3600 --mahalUseR=true \
          --outdir="$d" > "$d/run.log" 2>&1; echo "e0 s=$2" ;;
    fd) d=results/fd_$4_b$2/seed_$3; mkdir -p "$d"
        ./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
          --seed=$3 --boxHalf=$2 --duration=3600 --fastDemote=$4 --trace=false \
          --outdir="$d" > "$d/run.log" 2>&1; echo "fd $4 b=$2 s=$3" ;;
  esac
}
export -f job
{
  for s in $(seq 1 20); do echo "e0 $s . ."; done
  for m in proposed all; do for b in 1000 3000; do for s in $(seq 1 20); do echo "fd $b $s $m"; done; done; done
} | xargs -P 10 -n 4 bash -c 'job "$0" "$1" "$2" "$3"'
echo "E0_PHASEA DONE"
