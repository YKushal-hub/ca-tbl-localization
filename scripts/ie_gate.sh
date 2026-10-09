#!/usr/bin/env bash
# G101 runner: Plan A, A2, the error-state filter (--imuErrorStates). What each run is for, and
# what a firing looks like on disk, is in scripts/ie_gate.py, which reads them.
#
# usage: bash scripts/ie_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test to build its fixtures from;
#   without one they go to a mktemp directory removed on exit.
#
# A2v: scratch/ca_tbl_ie_test (the C++ filter on a fixed stream) against
# analysis/accuracy_replay/make_ie_vectors.py (the Python reference). Then the preset's
# endgame, ca_tbl_full and static_2p4, seed 1, 300 s: the flag absent, false, true; and the
# refusal without --imuPredict.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
TEST="$(pwd)/build/scratch/ns3.47-ca_tbl_ie_test-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g101.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
"$TEST" > "$D/ie_cpp.txt" 2> "$D/ie_cpp.err"
echo $? > "$D/ie_cpp.rc"
python3 -I analysis/accuracy_replay/make_ie_vectors.py > "$D/ie_py.txt" 2> "$D/ie_py.err"
echo $? > "$D/ie_py.rc"
B="--specProfile=true --arms=ca_tbl_full,static_2p4 --seed=1 --duration=300 --trace=true --boxHalf=750 --finderMode=1"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
run base $B &
run off $B --imuErrorStates=false &
run on $B --imuErrorStates=true &
run r_noimu --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --imuErrorStates=true &
wait

python3 scripts/ie_gate.py "$D"
