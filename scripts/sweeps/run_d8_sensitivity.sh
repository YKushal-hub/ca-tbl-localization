#!/usr/bin/env bash
# D-8 stage 1, as report/PREREG_D8_SENSITIVITY.md registers it.
#   probe, fix off and on                         -> <root>/probe_{off,on}/
#   {ref, sparse} x {off, on} x seeds 701..720    -> <root>/<cfg>_<cond>_s<seed>/
# Refuses a dirty tree and any existing output directory.
set -u
set -o pipefail
cd "$(dirname "$0")/../.." || exit 2
BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
ROOT=${D8_ROOT:-results/d8_sensitivity}
JOBS=${D8_JOBS:-11}
FIX="--t2SensitivityRowKHz=812.5"
COMMON="--duration=3600 --gwGrid=5 --nlosSource=propagation --escalationRequiresEvidence=true --pcsForm=min --trace=true"
[ -x "$BIN" ] || { echo "no binary at $BIN"; exit 2; }
dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
[ "$dirty" -eq 0 ] || { echo "REFUSING: $dirty tracked modification(s) under src/ scratch/"; exit 2; }
mkdir -p "$ROOT"
JOBFILE=$(mktemp); trap 'rm -f "$JOBFILE"' EXIT
for cond in off on; do
    o="$ROOT/probe_$cond"; [ -e "$o" ] && { echo "REFUSING: $o exists"; exit 2; }
    extra=""; [ "$cond" = on ] && extra="$FIX"
    echo "$o|--probe=true $extra" >> "$JOBFILE"
done
for cfg in ref sparse; do
    if [ "$cfg" = ref ]; then box=2000; else box=3000; fi
    for cond in off on; do
        extra=""; [ "$cond" = on ] && extra="$FIX"
        for s in $(seq 701 720); do
            o="$ROOT/${cfg}_${cond}_s${s}"
            [ -e "$o" ] && { echo "REFUSING: $o exists"; exit 2; }
            echo "$o|--seed=$s --boxHalf=$box $COMMON $extra" >> "$JOBFILE"
        done
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
echo "done; score with: python3 scripts/d8_analysis.py $ROOT <outdir>"
