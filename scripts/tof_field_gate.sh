#!/usr/bin/env bash
# G120 runner: the tier-2 field-error flags (--tofFieldSigmaM, --tofFieldBiasM) and the
# covariance trace columns (--traceCovariance). What each run is for, and what a firing looks
# like on disk, is in scripts/tof_field_gate.py, which reads them.
#
# usage: bash scripts/tof_field_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# static_2p4 and ca_tbl_full at the default deployment, seed 7, 600 s: the control, the three
# flags given explicitly at their defaults, the covariance columns on, and the sigma floor at
# Mueller et al.'s rural 83.6 m. The pre-flight probe with and without a 1000 m bias.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g120.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=static_2p4,ca_tbl_full --seed=7 --duration=600 --trace=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run base $B &
run zero $B --tofFieldSigmaM=0 --tofFieldBiasM=0 --traceCovariance=false &
run cov $B --traceCovariance=true &
run sig $B --tofFieldSigmaM=83.6 &
wait
run probe_off --probe=true &
run probe_bias --probe=true --tofFieldBiasM=1000 &
wait

python3 scripts/tof_field_gate.py "$D"
