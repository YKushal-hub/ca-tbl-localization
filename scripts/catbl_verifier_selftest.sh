#!/usr/bin/env bash
# G5 -- verifier self-test.
#
# The audit that produced CA_TBL_AUDIT_REPORT.md found five defects, ALL of them in
# validation code and none in the simulator. That asymmetry is not evidence the
# simulator is clean -- this session found the nNlos conflation, the missing R in
# the Mahalanobis, unweighted multilateration, the stream-index defect, the
# zero-cost search charge, the ClearNeighbours trap and the boxHalf/density
# confound, all in the simulator. The asymmetry has a cause: the gates test the
# simulator on every run, and nothing tested the gates. 260 runs of scrutiny on one
# side, none on the other.
#
# This file is the other side. It asserts that verify_causal_sim.py FAILS when its
# inputs are broken, not merely that it passes when they are not. A validator that
# has only ever been observed passing is untested.
#
# Runs in seconds against an existing results directory; takes no simulation.
#
#   bash scripts/catbl_verifier_selftest.sh [healthy_results_dir]

set -u
set -o pipefail
cd "$(dirname "$0")/.." || exit 2

SRC=${1:-results/causal}
V=scratch/python_handoff/verify_causal_sim.py
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

if [ ! -s "$SRC/arm_summary.csv" ]; then
    printf 'G5: no healthy source directory at %s -- run G3 first\n' "$SRC"
    exit 1
fi
if ! ls "$SRC"/trace_*.csv >/dev/null 2>&1; then
    printf 'G5: %s has no traces -- G5 needs a traced run\n' "$SRC"
    exit 1
fi

fresh() {
    rm -rf "$TMP/d"; mkdir -p "$TMP/d"
    cp "$SRC"/trace_*.csv "$TMP/d/"
    sleep 1                      # summary must be newest, as a real run leaves it
    cp "$SRC/arm_summary.csv" "$TMP/d/"
}

# expect <want_exit> <label>
expect() {
    local want=$1 label=$2 got
    python3 "$V" "$TMP/d" >"$TMP/out" 2>&1
    got=$?
    if [ "$got" -eq "$want" ]; then
        printf '  G5 ok    %-34s exit=%d\n' "$label" "$got"
    else
        printf '  G5 FAIL  %-34s exit=%d want=%d\n' "$label" "$got" "$want"
        sed -n '1,20p' "$TMP/out"
        FAILED=1
    fi
}

FAILED=0
printf '\n--- G5 verifier self-test ---\n'

# A. Healthy input must PASS. Without this the other four could be satisfied by a
#    verifier that fails unconditionally.
fresh
expect 0 "healthy input passes"

# B. D1 -- a missing arm trace. Before the fix this scored 54 checks, printed
#    "All invariants hold" and exited 0 while auditing four of six arms.
fresh
rm -f "$TMP/d"/trace_static_uwb_*.csv
expect 1 "missing arm trace is caught"

# C. D2 mode 1 -- summary rewritten over stale traces by a later --trace=false run.
#    This is the v3 section 3.10 incident, caught then by accident, ungated until now.
fresh
touch -d "+5 minutes" "$TMP/d/arm_summary.csv"
expect 1 "summary newer than traces is caught"

# D. D2 mode 2 -- the reverse: a run wrote traces and died before the summary, so a
#    stale summary is read against fresh traces.
fresh
touch -d "-2 hours" "$TMP/d/arm_summary.csv"
expect 1 "summary older than traces is caught"

# E. D2 mode 3 -- traces mixed from two runs, e.g. a batch resumed after a crash
#    that regenerated some arms and not others.
fresh
touch -d "-1 day" "$TMP/d"/trace_static_868_*.csv
expect 1 "traces from two runs are caught"

if [ "$FAILED" -eq 0 ]; then
    printf '\n=== G5 PASS (verifier fails when it should) ===\n'
    exit 0
fi
printf '\n=== G5 FAIL ===\n'
exit 1
