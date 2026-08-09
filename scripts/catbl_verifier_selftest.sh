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
    # run_id.txt belongs to a healthy directory and is copied FIRST, with the
    # traces, because a real run writes it at startup. Omitting it made the
    # "healthy input passes" control fail as soon as provenance became a hard
    # requirement -- correctly, since a directory that cannot say what produced
    # it is not healthy. The control has to be a genuinely healthy fixture, not
    # a waiver, or it stops being a control for the other four cases.
    [ -f "$SRC/run_id.txt" ] && cp "$SRC/run_id.txt" "$TMP/d/"
    cp "$SRC"/trace_*.csv "$TMP/d/"
    sleep 1                      # summary must be newest, as a real run leaves it
    cp "$SRC/arm_summary.csv" "$TMP/d/"
}

# expect_args <want_exit> <label> [verifier flags...]
expect_args() {
    local want=$1 label=$2 got
    shift 2
    python3 "$V" "$TMP/d" "$@" >"$TMP/out" 2>&1
    got=$?
    if [ "$got" -eq "$want" ]; then
        printf '  G5 ok    %-34s exit=%d\n' "$label" "$got"
    else
        printf '  G5 FAIL  %-34s exit=%d want=%d\n' "$label" "$got" "$want"
        sed -n '1,20p' "$TMP/out"
        FAILED=1
    fi
}

# expect <want_exit> <label> -- the no-flags form, which is how a real run invokes
# the verifier and therefore how most cases below should exercise it.
expect() { expect_args "$1" "$2"; }

FAILED=0

# What this source directory can actually support. Three cases below depend on it
# having a digest block, and one on knowing which mode produced it. Hardcoding
# either makes the self-test valid for exactly one directory while appearing to be
# general: run against results/baseline_v6 with those assumptions baked in and it
# reports two FAILs that are properties of the fixture, not of the verifier.
# `awk ... | head -1` would be the same SIGPIPE-under-pipefail shape as the guard
# below. Harmless here only because the status of a bare assignment is never
# tested -- which is a property of today's code, not of the construct. awk exits
# after the first match instead, so there is no pipe to break.
SRCSHA=$(awk '{for(i=1;i<NF;i++) if($i=="source"){print $(i+1); exit}}' \
             "$SRC/run_id.txt" 2>/dev/null)
HAS_DIGEST=0
grep -q '^digest sha256$' "$SRC/run_id.txt" 2>/dev/null && HAS_DIGEST=1
SRC_NLOS=legacy; SRC_PCS=legacy
if [ -s "$SRC/run_id.txt" ]; then
    grep -q -- '--nlosSource=propagation' "$SRC/run_id.txt" && SRC_NLOS=propagation
    grep -q -- '--pcsForm=v2' "$SRC/run_id.txt" && SRC_PCS=v2
fi

# skip <label> <reason> -- an unrunnable case must be visibly absent, not silently
# counted as passing. Silence is how a gate that never fires looks identical to one
# that always passes.
skip() { printf '  G5 skip  %-34s (%s)\n' "$1" "$2"; }

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

# F. Provenance. The sidecar gate was promoted from warn to check, and a promoted
#    gate with no failing test is indistinguishable from one that never fires.
fresh
rm -f "$TMP/d/run_id.txt"
expect 1 "missing run_id.txt is caught"

# G. The escape hatch itself. --no-require-provenance waives the sidecar, and it
#    used to also let the mode silently default to legacy -- which is how a
#    propagation-produced directory could pass six NLOS gates keyed to the wrong
#    population. Waiving the sidecar must NOT waive the mode.
fresh
rm -f "$TMP/d/run_id.txt"
expect_args 1 "waiver without a stated mode is refused" --no-require-provenance

# H. ...and with the mode stated, the same directory verifies. Without this, G is
#    satisfied by a hatch that refuses everything.
fresh
rm -f "$TMP/d/run_id.txt"
expect_args 0 "waiver WITH a stated mode proceeds" \
    --no-require-provenance --nlosSource=$SRC_NLOS --pcsForm=$SRC_PCS

# I. The output digest. This is the only check in the harness that can see a
#    result edited after its run, or a sidecar that belongs to a different run --
#    both states in which every mtime-ordering gate passes, because the edited
#    file is NEWER than the sidecar. Appending one byte is the minimal version of
#    exactly that.
if [ "$HAS_DIGEST" = 1 ]; then
    fresh
    printf '\n' >> "$TMP/d/trace_static_2p4_seed7.csv"
    expect 1 "a modified output is caught by the digest"
else
    skip "modified output" "$(basename "$SRC") has no digest block"
fi

# J. ...and a recorded output that vanished. Distinct from case B: there the file
#    is missing from the arm coverage too, whereas a digest entry with no file is
#    detectable even for outputs no coverage gate enumerates.
if [ "$HAS_DIGEST" = 1 ]; then
    fresh
    rm -f "$TMP/d/trace_static_868_seed7.csv"
    expect 1 "a recorded output that vanished is caught"
else
    skip "vanished recorded output" "$(basename "$SRC") has no digest block"
fi

# K. A stripped digest block. Only a failure when the source this directory was
#    built from actually CONTAINS the digest writer -- otherwise absence is the
#    legitimate pre-digest state and tolerating it is the correct behaviour. So
#    the case is guarded by that same question rather than assumed, which also
#    keeps the self-test usable against a historical results directory.
#    THREE outcomes, not two. "git says the writer is absent" and "git could not be
#    asked" are different facts and only the first justifies skipping with that
#    reason. The first version collapsed them and printed "predates the digest
#    writer" for a case where the truth was "the guard failed" -- a skip line
#    asserting a cause it had not established.
#
#    It failed for the reason this file's own harness warns about: `git cat-file |
#    grep -q` under `set -o pipefail`. grep -q exits at the first match, git dies of
#    SIGPIPE (141), and pipefail promotes that to the pipeline's status -- so a
#    MATCH reported as a non-match. Testing it by hand missed it because an
#    interactive shell has no pipefail. Capture first, match second: no pipe, no
#    early exit, and empty output is distinguishable from no match.
SRCSRC=""
if [ -n "$SRCSHA" ]; then
    SRCSRC=$(git cat-file -p "$SRCSHA:scratch/ca_tbl_causal_sim.cc" 2>/dev/null) || SRCSRC=""
fi
case "$SRCSRC" in
    *"digest sha256"*)
        fresh
        sed -i '/^digest sha256$/,$d' "$TMP/d/run_id.txt"
        expect 1 "a stripped digest block is caught"
        ;;
    "")
        # Name the sha and the likely cause. A rewrite makes every recorded source
        # unresolvable at once, so this is the expected state right after one -- and
        # it must not be reported as "predates the writer", which is a different
        # fact that happens to produce the same skip.
        skip "stripped digest block" \
             "recorded source ${SRCSHA:-<none recorded>} is not in this repository; history may have been rewritten, so the question could not be asked"
        ;;
    *)
        skip "stripped digest block" "source $SRCSHA predates the digest writer"
        ;;
esac

if [ "$FAILED" -eq 0 ]; then
    printf '\n=== G5 PASS (verifier fails when it should) ===\n'
    exit 0
fi
printf '\n=== G5 FAIL ===\n'
exit 1
