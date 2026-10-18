#!/usr/bin/env bash
# G28 runner: the spec's calibrated PCS is expressible, and what is expressed reaches the
# policy (docs/SPEC_CONFORMANCE_PLAN.md DC-5/DC-6). What each run is for, and what a
# firing looks like on disk, is in scripts/pcs_calibrated_gate.py, which reads them.
#
# usage: bash scripts/pcs_calibrated_gate.sh [<workdir>]
#   With <workdir> the runs are left there (the suite passes one so the self-test can
#   build its fixtures from these real runs rather than make its own). Without one they
#   go to a mktemp directory that is removed on exit.
#
# 14 scenario runs of 900 s at seed 7 (about 3 s each), four of 30 s, and five refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g28.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
BASE="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --boxHalf=1000"  # pinned: TD2P4 needs the pre-DD-16 default box, where 2.4 GHz de-escalation is reached
SHORT="--arms=ca_tbl_full --seed=7 --duration=30 --trace=false --boxHalf=1000"
EV="--escalationRequiresEvidence=false"
LEG_THR="--pcsThrUp868=1.55 --pcsThrUp2p4=1.70 --pcsThrUpUwb=2.30 --pcsThrDown2p4=1.90 --pcsThrDownUwb=2.55"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run legacy $BASE --pcsForm=legacy
run cal111 $BASE --pcsForm=calibrated --pcsW1=1 --pcsW2=1 --pcsW3=1 $LEG_THR
run never $BASE --pcsForm=calibrated --pcsW1=3 --pcsW2=0 --pcsW3=0 --pcsThrUp868=-1 \
    --pcsThrUp2p4=-1 --pcsThrUpUwb=-1 --pcsThrDown2p4=-0.5 --pcsThrDownUwb=-0.5
run always $BASE --pcsForm=calibrated --pcsW1=3 --pcsW2=0 --pcsW3=0 --pcsThrUp868=10 \
    --pcsThrUp2p4=10 --pcsThrUpUwb=10 --pcsThrDown2p4=11 --pcsThrDownUwb=11
run wdef $BASE --pcsForm=weighted
run w0 $BASE --pcsForm=weighted --pcsW2=0
run ev_leg $BASE $EV
run ev_up868 $BASE $EV --pcsThrUp868=-1
run ev_up2p4 $BASE $EV --pcsThrUp2p4=-1
run ev_dn2p4 $BASE $EV --pcsThrDown2p4=10
run slots $SHORT --pcsForm=calibrated --pcsW1=0.31 --pcsW2=0.32 --pcsW3=0.33 \
    --pcsThrUp868=0.11 --pcsThrUp2p4=0.12 --pcsThrUpUwb=0.13 --pcsThrDown2p4=0.24 \
    --pcsThrDownUwb=0.25
run slots_v2 $SHORT --pcsForm=v2 --pcsThrUp2p4=0.1
run slots_band $SHORT --pcsHysteresisBand=0.3 --pcsThrDownUwb=3
run r_up868_ok $SHORT --pcsThrUp868=2.0
run r_missing_thr $SHORT --pcsForm=calibrated --pcsW1=3 --pcsW2=0 --pcsW3=0 \
    --pcsThrUp868=0.1 --pcsThrUp2p4=0.1 --pcsThrUpUwb=0.1 --pcsThrDown2p4=0.2
run r_missing_w $SHORT --pcsForm=calibrated --pcsW1=3 --pcsW2=0 $LEG_THR
run r_inv_explicit $SHORT --pcsThrUp2p4=1.6 --pcsThrDown2p4=1.5
run r_inv_resolved $SHORT --pcsThrUp2p4=2.0
run r_down868 $SHORT --pcsThrDown868=2

python3 scripts/pcs_calibrated_gate.py "$D"
