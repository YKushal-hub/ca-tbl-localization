#!/usr/bin/env bash
# G91 runner: the S2-B accuracy flags (TECH_DEBT N.35-N.38): --dualBaseRelayTier,
# --relayCovScale, --hybridNoLone868, --caseDInfoRatio. What each run is for, and what a firing looks like on
# disk, is in scripts/acc_b_gate.py, which reads them.
#
# usage: bash scripts/acc_b_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# The preset, ca_tbl_full, seed 1, 600 s. Endgame (--boxHalf=750 --finderMode=1): flags
# absent, all three given at their off values, the relay tier, the relay tier with the
# covariance scale. 5 km (where tag 0 takes most of its lone 868 MHz Case D updates):
# flags absent, --hybridNoLone868 and --caseDInfoRatio=3.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g91.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
A="--specProfile=true --arms=ca_tbl_full --seed=1 --duration=600 --trace=true"
B="$A --boxHalf=750 --finderMode=1"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run base $B &
run off $B --dualBaseRelayTier=false --relayCovScale=1 --hybridNoLone868=false --caseDInfoRatio=0 &
run relay $B --dualBaseRelayTier=true &
run scale $B --dualBaseRelayTier=true --relayCovScale=4 &
wait
run a_base $A &
run a_lone $A --hybridNoLone868=true &
run a_info $A --caseDInfoRatio=3 &
wait

python3 scripts/acc_b_gate.py "$D"
