#!/usr/bin/env bash
# A3 -- the escalation handshake inside the cycle, as report/PREREG_A3_INTRA_CYCLE_HANDSHAKE.md
# registers it. {off, on} x seeds 601..620 = 40 invocations, six arms, traced.
# One directory per invocation: <root>/<cond>_s<seed>/ ; stderr (A3_CENSUS) beside it.
# Refuses a dirty tree and any existing output directory.
set -u
set -o pipefail
cd "$(dirname "$0")/../.." || exit 2
BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
ROOT=${A3_ROOT:-results/a3_handshake}
JOBS=${A3_JOBS:-11}
COMMON="--duration=3600 --boxHalf=2000 --gwGrid=5 --nlosSource=propagation --escalationRequiresEvidence=true --pcsForm=min --trace=true"
[ -x "$BIN" ] || { echo "no binary at $BIN"; exit 2; }
dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
[ "$dirty" -eq 0 ] || { echo "REFUSING: $dirty tracked modification(s) under src/ scratch/"; exit 2; }
mkdir -p "$ROOT"
JOBFILE=$(mktemp); trap 'rm -f "$JOBFILE"' EXIT
for cond in off on; do
    extra=""; [ "$cond" = on ] && extra="--intraCycleHandshake=true"
    for s in $(seq 601 620); do
        o="$ROOT/${cond}_s${s}"
        [ -e "$o" ] && { echo "REFUSING: $o exists"; exit 2; }
        echo "$o|--seed=$s $COMMON $extra" >> "$JOBFILE"
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
echo "done; score with: python3 scripts/a3_handshake_analysis.py $ROOT <outdir>"
