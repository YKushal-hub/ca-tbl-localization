#!/usr/bin/env bash
# The C3 IMU-prior campaign, exactly as report/PREREG_IMU_PRIOR.md registers it.
#
#   2 configurations x {cv, imu} x seeds 301..330 = 120 invocations, all six arms each.
#   One directory per invocation: <root>/<cfg>_<cond>_s<seed>/
#
# Refuses to start on a dirty tree (src/ scratch/ tracked changes), because a sidecar
# that records dirty > 0 is not citable and the campaign would have to be re-run.
# Refuses to write into any directory that already exists: regeneration goes to a NEW
# path, never over evidence (working rule 7).
#
# The registered run uses no overrides. IMU_SEEDS / IMU_DURATION / IMU_ROOT exist ONLY
# so the runner and the analysis can be exercised on seeds outside the registered range
# before the registration is committed; a registered cell is never produced with them.

set -u
set -o pipefail
cd "$(dirname "$0")/../.." || exit 2

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
ROOT=${IMU_ROOT:-results/imu_prior}
SEEDS=${IMU_SEEDS:-$(seq 301 330)}
DUR=${IMU_DURATION:-3600}
JOBS=${IMU_JOBS:-11}

COMMON="--duration=$DUR --gwGrid=5 --nlosSource=propagation --escalationRequiresEvidence=true --pcsForm=min --trace=true"
declare -A CFG=( [ref]="--boxHalf=2000" [sparse]="--boxHalf=3000" )
declare -A COND=( [cv]="" [imu]="--imuPrior=true" )

[ -x "$BIN" ] || { echo "no binary at $BIN -- build with --build-profile=optimized"; exit 2; }
dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
if [ "$dirty" -ne 0 ]; then
    echo "REFUSING: $dirty tracked modification(s) under src/ scratch/ -- commit first"
    git status --short --untracked-files=no -- src scratch
    exit 2
fi

mkdir -p "$ROOT"
JOBFILE=$(mktemp)
trap 'rm -f "$JOBFILE"' EXIT
for cfg in ref sparse; do
    for cond in cv imu; do
        for s in $SEEDS; do
            d="$ROOT/${cfg}_${cond}_s${s}"
            if [ -e "$d" ]; then
                echo "REFUSING: $d exists -- this runner never writes over a result"
                exit 2
            fi
            echo "$d|--seed=$s $COMMON ${CFG[$cfg]} ${COND[$cond]}" >> "$JOBFILE"
        done
    done
done
echo "$(wc -l < "$JOBFILE") invocations, $JOBS in parallel, into $ROOT"

run_one() {
    d=${1%%|*}
    args=${1#*|}
    mkdir -p "$d"
    # shellcheck disable=SC2086
    if "$BIN" $args --outdir="$d" > "$d.stdout" 2> "$d.stderr"; then
        echo "ok   $d"
    else
        echo "FAIL $d (exit $?)"
    fi
}
export -f run_one
export BIN
xargs -P "$JOBS" -I{} bash -c 'run_one "$@"' _ {} < "$JOBFILE"
echo "done; score with: python3 scripts/imu_prior_analysis.py $ROOT <outdir>"
