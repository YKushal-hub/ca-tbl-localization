#!/usr/bin/env bash
# G38 runner: gateway placement shapes (docs/SPEC_CONFORMANCE_PLAN.md MS-2, --gwLayout). What
# each run is for, and what a firing looks like on disk, is in scripts/gw_layout_gate.py.
#
# usage: bash scripts/gw_layout_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# Short runs (static_868, 30 s) at the spec scale, --boxHalf=5000 and 5 gateways: the
# default and --gwLayout=legacy (a ring), each shape, the cluster and star again from a
# base camp, and a file layout. Plus four refusals.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g38.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=static_868 --seed=7 --duration=30 --trace=true --vizEvents=true --boxHalf=5000 --nGateways=5"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
printf 'x,y\n-4000.5,-1000\n0,4999\n3000,-2500\n# a comment\n7000,0\n' > "$D/layout.csv"

run def $B &
run legacy $B --gwLayout=legacy &
run perimeter $B --gwLayout=perimeter &
run cluster $B --gwLayout=cluster &
run star $B --gwLayout=star &
run lshape $B --gwLayout=lshape &
run cluster_bc $B --gwLayout=cluster --baseCampFrac=0.75 &
run star_bc $B --gwLayout=star --baseCampFrac=0.75 --gwStarArms=2 --gwStarSpacingM=1200 &
run file $B --gwLayout=file --gwLayoutFile="$D/layout.csv" &
wait
run r_bad $B --gwLayout=hexagon
run r_non --arms=static_868 --seed=7 --duration=30 --trace=false --gwLayout=perimeter
run r_file $B --gwLayout=file --gwLayoutFile="$D/absent.csv"
run r_topo $B --gwLayout=perimeter --topology=1

python3 scripts/gw_layout_gate.py "$D"
