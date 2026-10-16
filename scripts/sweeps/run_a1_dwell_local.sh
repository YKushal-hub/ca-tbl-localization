#!/usr/bin/env bash
# A1 -- coverage fraction against dwell, exactly as report/PREREG_A1_DWELL.md registers it.
# 5 dwells x seeds 501..520 = 100 invocations, four arms, traced.
# One directory per invocation: <root>/d<dwell>_s<seed>/
# Refuses a dirty tree and any existing output directory.
set -u
set -o pipefail
cd "$(dirname "$0")/../.." || exit 2
BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
ROOT=${A1_ROOT:-results/a1_dwell}
JOBS=${A1_JOBS:-11}
COMMON="--duration=3600 --boxHalf=3000 --gwGrid=5 --nlosSource=propagation --escalationRequiresEvidence=true --pcsForm=min --trace=true --arms=ca_tbl_full,static_2p4,fixed_threshold,static_868"
[ -x "$BIN" ] || { echo "no binary at $BIN"; exit 2; }
dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
[ "$dirty" -eq 0 ] || { echo "REFUSING: $dirty tracked modification(s) under src/ scratch/"; exit 2; }
mkdir -p "$ROOT"
JOBFILE=$(mktemp); trap 'rm -f "$JOBFILE"' EXIT
for d in 90 150 245 400 800; do
    for s in $(seq 501 520); do
        o="$ROOT/d${d}_s${s}"
        [ -e "$o" ] && { echo "REFUSING: $o exists"; exit 2; }
        echo "$o|--seed=$s $COMMON --dwell=$d" >> "$JOBFILE"
    done
done
echo "$(wc -l < "$JOBFILE") invocations into $ROOT"
run_one() {
    o=${1%%|*}; args=${1#*|}; mkdir -p "$o"
    # shellcheck disable=SC2086
    if "$BIN" $args --outdir="$o" > "$o.stdout" 2> "$o.stderr"; then echo "ok   $o"; else echo "FAIL $o"; fi
}
export -f run_one; export BIN
xargs -P "$JOBS" -I{} bash -c 'run_one "$@"' _ {} < "$JOBFILE"
echo "done; score with: python3 scripts/a1_dwell_analysis.py $ROOT <outdir>"
