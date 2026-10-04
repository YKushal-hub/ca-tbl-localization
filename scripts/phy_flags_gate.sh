#!/usr/bin/env bash
# G36 runner: tier-1 and tier-2 spreading factor and bandwidth from the CLI
# (docs/SPEC_CONFORMANCE_PLAN.md SY-3). What each run is for, and what a firing looks like
# on disk, is in scripts/phy_flags_gate.py, which reads them.
#
# usage: bash scripts/phy_flags_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Four 600 s runs at seed 7 (static_868, static_2p4, ca_tbl_full): the default, the four
# flags given at their defaults, --sf868=9, and --bw868=250000 --sf2p4=9 --bw2p4=812500.
# Plus two refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g36.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=static_868,static_2p4,ca_tbl_full --seed=7 --duration=600 --trace=true"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}

run def $B &
run expl $B --sf868=7 --bw868=125000 --sf2p4=7 --bw2p4=800000 &
run sf9 $B --sf868=9 &
run bw $B --bw868=250000 --sf2p4=9 --bw2p4=812500 &
wait
run r_sf --arms=ca_tbl_full --seed=7 --duration=30 --trace=false --sf868=13
run r_bw --arms=ca_tbl_full --seed=7 --duration=30 --trace=false --bw868=300000

python3 scripts/phy_flags_gate.py "$D"
