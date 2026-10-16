#!/usr/bin/env bash
# E3 -- CAL-05 with the inertial prior, exactly as report/PREREG_E3_IMU_INTEGRITY.md
# registers it.
#
#   {cv, imu} x bias {10,20,40,80,160} m x {F1, F3} x seeds 1..20 = 400 invocations,
#   arms ca_tbl_full and ca_tbl_no_integrity, traced.
#   One directory per invocation: <root>/<cond>_b<bias>_<F>_s<seed>/
#
# The cv half regenerates CAL-05 under current code; the scorer checks each cv cell's
# digests against the tracked CAL-05 sidecar (R1).
#
# Refuses a dirty tree (src/ scratch/ tracked changes) and any existing output
# directory. Never writes into results/c9_stage2 or results/c9_escalate, whose
# sidecars are the evidence R1 is checked against.

set -u
set -o pipefail
cd "$(dirname "$0")/../.." || exit 2

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
ROOT=${E3_ROOT:-results/e3_imu_integrity}
JOBS=${E3_JOBS:-11}
COMMON="--duration=3600 --boxHalf=2000 --gwGrid=5 --escalationRequiresEvidence=true --nlosSource=propagation --pcsForm=v2 --mahalUseR=true --injectBiasFraction=0.2 --suspectRecovery=bounded --suspectForcesEscalation=true --arms=ca_tbl_full,ca_tbl_no_integrity --trace=true"

[ -x "$BIN" ] || { echo "no binary at $BIN -- build with --build-profile=optimized"; exit 2; }
dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
if [ "$dirty" -ne 0 ]; then
    echo "REFUSING: $dirty tracked modification(s) under src/ scratch/ -- commit first"
    exit 2
fi

mkdir -p "$ROOT"
JOBFILE=$(mktemp)
trap 'rm -f "$JOBFILE"' EXIT
for cond in cv imu; do
    if [ "$cond" = imu ]; then extra="--imuPrior=true"; else extra=""; fi
    for bias in 10 20 40 80 160; do
        for fl in F1 F3; do
            # fault label mapping, as CAL-05: --injectFaultAnchors 1 is F1, 0 is F3
            if [ "$fl" = F1 ]; then fa=1; else fa=0; fi
            for s in $(seq 1 20); do
                d="$ROOT/${cond}_b${bias}_${fl}_s${s}"
                [ -e "$d" ] && { echo "REFUSING: $d exists"; exit 2; }
                echo "$d|--seed=$s $COMMON --injectRangeBias=$bias --injectFaultAnchors=$fa $extra" >> "$JOBFILE"
            done
        done
    done
done
echo "$(wc -l < "$JOBFILE") invocations, $JOBS in parallel, into $ROOT"
run_one() {
    d=${1%%|*}
    args=${1#*|}
    mkdir -p "$d"
    # shellcheck disable=SC2086
    if "$BIN" $args --outdir="$d" > "$d.stdout" 2> "$d.stderr"; then echo "ok   $d"; else echo "FAIL $d"; fi
}
export -f run_one
export BIN
xargs -P "$JOBS" -I{} bash -c 'run_one "$@"' _ {} < "$JOBFILE"
echo "done; score with: python3 scripts/e3_imu_integrity_analysis.py $ROOT <outdir>"
