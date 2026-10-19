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

printf '\n--- provenance: no cited result may come from a dirty tree ---\n'
# "No dirty run backs a cited claim" is a PUBLICATION-READINESS property, not a
# simulator-correctness one, so it is asserted here rather than in
# catbl_gates.sh. The suite tests whether the simulator works and regenerates its
# own scratch directories every run; this tests whether the evidence is fit to
# publish.
#
# THIS CORRECTS A LIVE FALSE POSITIVE, not a hypothetical one. This script
# reported PUBLICATION READY while every headline number in the technical report
# came from `results/p10_plane_fix`, whose twenty sidecars each record
# `source a059203 dirty 3` -- a binary rebuildable from no commit in the
# repository. Same defect class as the clone test described above: a green that
# is true of what it checks and false of what a reader infers from it.
#
# Scope is the directories a CITED result comes from. Cheap and non-clever:
# every results directory named in the report or the data pack. A directory
# nothing cites cannot make a claim unreproducible.
#
# No exemption is available here. The verifier's ALLOW_DIRTY marker and
# --scratch flag both exist for working sessions and ephemeral scratch; neither
# is honoured at this gate, because the question here is not "was this run
# deliberate" but "can a reader regenerate it".
# An assertion that can never PASS is as broken as one that can never fail. One document
# -- the data pack -- cites superseded campaigns deliberately, because it records what
# those runs produced; repointing it would describe one run's output under another run's
# path. Without an exception this assertion reports NOT READY forever regardless of how
# correct the tree is. The exception is enumerated BY PATH with a reason in EXCLUDED.md's
# reviewed-dirty-citations block, parsed rather than duplicated here, and it is not a
# blanket exemption: a dirty directory cited by ANY non-exempt document still fails.
CITING_DOCS='CA-TBL_Technical_Report_SelfContained.md report/REPORT_DATA_PACK.md'
exempt_docs=$(awk '/^```reviewed-dirty-citations$/{f=1;next} /^```$/{f=0} f' EXCLUDED.md 2>/dev/null)
enforced_docs=''
for doc in $CITING_DOCS; do
    skip=0
    for ex in $exempt_docs; do [ "$doc" = "$ex" ] && skip=1; done
    [ "$skip" -eq 0 ] && enforced_docs="$enforced_docs $doc"
done
cited=$(grep -rhoE 'results/[A-Za-z0-9_.-]+' $CITING_DOCS 2>/dev/null | sort -u)
enforced=$(grep -rhoE 'results/[A-Za-z0-9_.-]+' $enforced_docs 2>/dev/null | sort -u)
n_cited=0; n_dirty=0; n_exempt=0; dirty_list=''
for d in $cited; do
    [ -d "$d" ] || continue
    for sc in "$d"/run_id.txt "$d"/*/run_id.txt; do
        [ -f "$sc" ] || continue
        n_cited=$((n_cited + 1))
        nd=$(awk '/^source /{print $4; exit}' "$sc")
        if [ -n "$nd" ] && [ "$nd" != "0" ]; then
            if printf '%s\n' $enforced | grep -qxF "$d"; then
                n_dirty=$((n_dirty + 1))
                dirty_list="$dirty_list$(dirname "$sc") ($nd files)
"
            else
                n_exempt=$((n_exempt + 1))
            fi
        fi
    done
done
if [ "$n_cited" -eq 0 ]; then
    note UNKNOWN "no cited results directory carries a run_id sidecar -- cannot verify"
    fail=1
elif [ "$n_dirty" -eq 0 ]; then
    note ok "all $n_cited cited result sidecar(s) record a clean tree, or are cited only by a reviewed exception ($n_exempt)"
else
    note DIRTY "$n_dirty of $n_cited cited result sidecar(s) were built from a dirty tree:"
    printf '%s' "$dirty_list" | head -5 | sed 's/^/            /'
    fail=1
fi

printf '\n--- ignored-but-source: files git is hiding that look like source ---\n'
# The INVERSE of assertion 12. That one catches tracked-and-ignored; this catches
# ignored-and-should-be-tracked, which nothing tested for until an unanchored
# `build/` pattern swallowed src/lorawan/.github/actions/build/action.yml -- a YAML
# source file, invisible to every check except a disk-vs-index count comparison.
# An unanchored directory pattern matches at ANY depth, so every vendored subtree
# can reintroduce this. Extension-based, because intent is not inspectable.
# The reviewed-exclusion prefixes come from EXCLUDED.md's fenced block, parsed
# rather than duplicated here: two copies of the same list drift, and a drifted
# allow-list silently re-hides whatever fell out of sync.
excl=$(awk '/^```reviewed-exclusions$/{f=1;next} /^```$/{f=0} f' EXCLUDED.md 2>/dev/null)
if [ -z "$excl" ]; then
    note MISSING "EXCLUDED.md has no reviewed-exclusions block -- cannot judge ignored files"
    fail=1
    excl='build/'
fi
pat=$(printf '%s' "$excl" | sed 's/[].[^$\\*]/\\&/g' | paste -sd'|' -)
# Only the TRACKED ignore rules are judged: they are what a fresh clone hides. A
# clone's own .git/info/exclude (machine-local tool settings, never published) is
# not part of the publication and is deliberately outside this check's population.
hidden=$(git ls-files --others --ignored --exclude-per-directory=.gitignore 2>/dev/null \
    | grep -E '\.(yml|yaml|cc|cpp|h|hpp|py|sh|cmake|rst|md|json|toml)$' \
    | grep -vE "^(${pat})" || true)
if [ -z "$hidden" ]; then
    note ok "no source-extension file is being ignored"
else
    # `wc -l` counts NEWLINES: a single entry with no trailing newline reads as 0, so
    # this assertion used to fire announcing "0 file(s) ignored". grep -c counts LINES.
    note HIDDEN "$(printf '%s\n' "$hidden" | grep -c .) source-extension file(s) ignored:"
    printf '%s\n' "$hidden" | head -8 | sed 's/^/            /'
    fail=1
fi

if [ "$fail" -eq 0 ]; then
    printf '\n=== PUBLICATION READY ===\n'; exit 0
fi
printf '\n=== NOT READY -- see above ===\n'; exit 1
