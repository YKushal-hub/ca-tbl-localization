#!/usr/bin/env bash
# Pre-push completeness manifest. Run before any push to a publication remote.
#
# WHY THIS EXISTS
# ---------------
# The publication run of 2026-08-08 produced six defects, none caught by any
# acceptance criterion. They split into two kinds needing different remedies:
#
#   INCOMPLETENESS -- something absent from the artefact
#     * the technical report on main was three baselines stale (the file was
#       PRESENT, so a presence-only manifest would have passed it)
#     * the run_id provenance writer and its verifier gate were both stranded on
#       an unrelated WIP branch by a reset
#     * results/ was never staged: 1746 files, while the README asserted every
#       table was sourced from a file in the repository
#
#   MISREADING -- a check returned something plausible and wrong
#     * a Pareto count of 12/12 against a measured 13/13
#     * pareto_sweep.py's verdict string answering a question the section did
#       not ask
#     * `git log | grep -c` returning 0 because git errored under dubious
#       ownership, read as "zero matches"
#
# The clone test covers neither kind. A clone builds and reproduces baseline_v6
# perfectly with no results/ directory, a stale report and no provenance gate.
# It validates that the artefact WORKS; it says nothing about whether it is
# COMPLETE. All three incompleteness defects were found by someone happening to
# look at the specific thing, which is not a repeatable process. This is.
#
# EVERY ASSERTION BELOW STATES ITS OWN DENOMINATOR, so none can pass by silence.
# `cmd | grep -c` cannot distinguish "zero matches" from "cmd failed", which is
# how defect six reported a false pass.

set -u
cd "$(dirname "$0")/.." || exit 2
fail=0
note() { printf '  %-7s %s\n' "$1" "$2"; }

printf '\n--- presence: files that must be TRACKED, not merely present ---\n'
for f in CA-TBL_Technical_Report_SelfContained.md \
         report/PCS_REDESIGN_RESULTS.md \
         CA_TBL_AUDIT_REPORT.md \
         README.md \
         scripts/catbl_gates.sh \
         scripts/catbl_verifier_selftest.sh \
         scripts/catbl_g3_ablation.py \
         scratch/python_handoff/verify_causal_sim.py \
         scratch/ca_tbl_causal_sim.cc \
         src/lora-physics-engine/model/ca-tbl-policy.h; do
    if git ls-files --error-unmatch "$f" >/dev/null 2>&1; then
        note ok "$f"
    else
        note MISSING "$f  (untracked -- will not appear in a clone)"
        fail=1
    fi
done

printf '\n--- freshness: the report must cite the CURRENT baseline ---\n'
# Presence is not enough. The stale report was present and readable; it cited
# baseline_v3 and described the Pareto sweep as pending.
newest=$(ls -d results/baseline_v* 2>/dev/null | sort -V | tail -1)
if [ -z "$newest" ]; then
    note SKIP "no results/baseline_v* on disk to check against"
else
    b=$(basename "$newest")
    n=$(grep -c "$b" CA-TBL_Technical_Report_SelfContained.md 2>/dev/null || true)
    if [ "${n:-0}" -gt 0 ]; then
        note ok "report cites $b ($n mentions)"
    else
        note STALE "report does not cite $b -- newest on disk is $b"
        fail=1
    fi
fi

printf '\n--- completeness: summaries STAGED, not merely un-ignored ---\n'
# git add -An proved the ignore patterns were correct and proved nothing about
# whether anything was staged. The count is the denominator that would have
# exposed it.
n=$(git ls-files | grep -c 'arm_summary\.csv$' || true)
if [ "${n:-0}" -gt 1000 ]; then
    note ok "$n arm_summary.csv tracked"
else
    note SHORT "only ${n:-0} arm_summary.csv tracked -- expected >1000"
    fail=1
fi
t=$(git ls-files | grep -c 'trace_.*\.csv$' || true)
if [ "${t:-0}" -eq 0 ]; then
    note ok "0 trace_*.csv tracked (correct -- ~13 GB, regenerable)"
else
    note BLOAT "$t trace_*.csv tracked -- an exclusion did not take"
    fail=1
fi

printf '\n--- pairs: both halves, never one ---\n'
# The provenance writer and its verifier gate are useless apart. Checking either
# alone would have passed while the mechanism was half-absent.
if git grep -q 'source " << head << " dirty "' -- scratch/ca_tbl_causal_sim.cc 2>/dev/null; then
    note ok "run_id provenance WRITER present"
else
    note MISSING "run_id provenance writer"
    fail=1
fi
if git grep -q 'built from a clean tree' -- scratch/python_handoff/verify_causal_sim.py 2>/dev/null; then
    note ok "provenance CHECKER present"
else
    note MISSING "provenance checker -- the sidecar would be written and never read"
    fail=1
fi

printf '\n--- oversize: GitHub rejects >100 MB outright ---\n'
big=$(git ls-files -z | xargs -0 -r stat -c '%s %n' 2>/dev/null | awk '$1>45000000 {print $2}')
if [ -z "$big" ]; then
    note ok "no tracked file over 45 MB"
else
    note OVERSIZE "$big"
    fail=1
fi

printf '\n--- profile: the README must instruct the profile the results were built with ---\n'
# The headline result is byte-reproducible only under the profile it was produced
# with; NS_ASSERT and NS_LOG exist or do not depending on it. A README that names
# a different profile sends every reader down a path that cannot reproduce.
newest=$(ls -d results/baseline_v* 2>/dev/null | sort -V | tail -1)
rid="$newest/run_id.txt"
if [ ! -s "$rid" ]; then
    note SKIP "no run_id.txt in ${newest:-<none>} to read a profile from"
else
    prof=$(awk '{for(i=1;i<NF;i++) if($i=="profile") print $(i+1)}' "$rid" | head -1)
    if [ -z "$prof" ]; then
        note SKIP "$(basename "$newest")/run_id.txt predates the profile field"
    elif grep -q -- "--build-profile=$prof" README.md 2>/dev/null; then
        note ok "README instructs --build-profile=$prof, matching $(basename "$newest")"
    else
        note MISMATCH "results built --build-profile=$prof; README does not instruct it"
        fail=1
    fi
fi

printf '\n--- ignored-but-tracked: files git would refuse to restore consistently ---\n'
# A tracked file that .gitignore also matches is a trap: it survives in the repo
# but any workflow that re-adds it silently drops it, and `git status` never
# mentions it. This is how the harness came to need `git add -f` in the first
# place. Empty output is the only passing state.
ig=$(git ls-files -z | xargs -0 -r git check-ignore 2>/dev/null)
if [ -z "$ig" ]; then
    note ok "no tracked file is matched by .gitignore"
else
    note TRAPPED "$(printf '%s' "$ig" | wc -l) tracked file(s) are gitignored:"
    printf '%s\n' "$ig" | head -5 | sed 's/^/            /'
    fail=1
fi

if [ "$fail" -eq 0 ]; then
    printf '\n=== PUBLICATION READY ===\n'; exit 0
fi
printf '\n=== NOT READY -- see above ===\n'; exit 1
