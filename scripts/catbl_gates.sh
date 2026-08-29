#!/usr/bin/env bash
# CA-TBL regression gates G1-G4. Run after every code change.
# Exits non-zero on the FIRST failing gate, so the caller stops and reverts.
#
# G1 build / G2 determinism / G3 null ablation / G4 verifier / G5 verifier self-test.
#
# G3 is expected to start failing once Phase 5 makes `suspect` fire. Until then a
# G3 failure means RNG contamination has returned. Export CATBL_SKIP_G3=1 to
# demote it to a warning once fault injection lands.

set -u
# WITHOUT pipefail, `./ns3 run ... | tail -3 || fail` tests tail's exit status, not
# the run's -- and tail essentially always succeeds. Every run below would then be
# treated as passing however it exited. Found by auditing this script against V3.1
# after it had become the primary gate harness. G1 already used ${PIPESTATUS[0]}
# explicitly; the run sites did not.
set -o pipefail
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)

# ---------------------------------------------------------------------------
# The suite USED to regenerate results/{causal,det_a,det_b} in place. Those three
# directories are tracked evidence, and the suite is the thing that verifies them:
# overwriting them with output from whatever binary is current makes it impossible
# to ask whether they were correct BEFORE the suite ran, and silently re-dates
# their sidecars to the last gate run rather than the producing run. Found by the
# 2026-08-29 diagnostic, whose own start/end tree check caught six modified files.
#
# All regeneration now goes to a temporary directory. The tracked copies are read,
# never written. G6 compares the two and says which way it disagrees.
WORK=$(mktemp -d "${TMPDIR:-/tmp}/catbl_gates.XXXXXX") || exit 2
cleanup() { rm -rf "$WORK"; }
trap cleanup EXIT

SEED=${CATBL_SEED:-7}
DUR=${CATBL_DURATION:-3600}

# ---------------------------------------------------------------------------
# The suite USED to exit on the first failing gate. That made every gate after the
# first failure unobservable -- and once G6 went deliberately red (pre-D1 evidence
# drift, cleared only by baseline_v7), G7, G8 and G9 could not be seen to run at
# all. "A check that cannot be observed is not a check" applies to the harness as
# much as to the checks.
#
# Every gate now runs. Failures are recorded and reported in a summary; the overall
# exit code is still non-zero if anything failed. This changes OBSERVABILITY, not
# strictness.
#
# FAIL and ERROR are kept distinct throughout: a gate that ran and found a problem
# is a different fact from a gate that could not run because its input was missing.
GATE_ORDER=""
declare -A GATE_ST
declare -A GATE_MSG

mark() {  # mark <gate> <FAIL|ERROR> <message>
    case "${GATE_ST[$1]:-}" in
        ERROR) ;;                        # ERROR is stickiest
        FAIL)  [ "$2" = ERROR ] && { GATE_ST[$1]=$2; GATE_MSG[$1]=$3; } ;;
        *)     GATE_ST[$1]=$2; GATE_MSG[$1]=$3 ;;
    esac
}
fail()  { mark "$1" FAIL  "${2:-}"; printf '\n--- %s recorded FAIL ---\n' "$1"; return 1; }
gerror(){ mark "$1" ERROR "${2:-}"; printf '\n--- %s recorded ERROR: %s ---\n' "$1" "${2:-}"; return 1; }

finish() {  # finish <gate>  -- close a gate, printing whatever it accumulated
    local g
    local st
    g=$1
    st=${GATE_ST[$g]:-PASS}
    GATE_ST[$g]=$st
    GATE_ORDER="$GATE_ORDER $g"
    printf '\n=== %s %s ===\n' "$g" "$st"
}
pass() { finish "$1"; }

# ---------------------------------------------------------------- G1: build
printf '\n--- G1 build ---\n'
./ns3 build 2>&1 | tail -40
[ "${PIPESTATUS[0]}" -eq 0 ] || fail G1
finish G1

# --------------------------------------------------- G2: determinism
# Identical flags on both runs. --trace=false, so this pass leaves the traced
# CSVs in results/causal untouched -- G3 rewrites them below (see v3 section 3.10).
printf '\n--- G2 determinism ---\n'
mkdir -p "$WORK/det_a" "$WORK/det_b"
./ns3 run "ca_tbl_causal_sim --seed=$SEED --duration=$DUR --outdir=$WORK/det_a --trace=false" 2>&1 | tail -3 || fail G2
./ns3 run "ca_tbl_causal_sim --seed=$SEED --duration=$DUR --outdir=$WORK/det_b --trace=false" 2>&1 | tail -3 || fail G2
for f in "$WORK/det_a/arm_summary.csv" "$WORK/det_b/arm_summary.csv"; do
    [ -s "$f" ] || { printf 'missing or empty: %s\n' "$f"; fail G2; }
done
diff "$WORK/det_a/arm_summary.csv" "$WORK/det_b/arm_summary.csv" || fail G2
finish G2

# ------------------------------------- G3: traced run + null ablation
# This run is ALSO the traced pass that G4 reads. Running it here, immediately
# before the verifier, is what stops the verifier reading a stale trace.
printf '\n--- G3 traced run + null ablation ---\n'
mkdir -p "$WORK/causal"
# A fresh mktemp directory cannot contain stale output, which is what the old
# `rm -f` guarded against when this ran in place.
./ns3 run "ca_tbl_causal_sim --seed=$SEED --duration=$DUR --outdir=$WORK/causal" 2>&1 | tail -5 || fail G3

FULL=$WORK/causal/trace_ca_tbl_full_seed$SEED.csv
NOINT=$WORK/causal/trace_ca_tbl_no_integrity_seed$SEED.csv
for f in "$FULL" "$NOINT"; do
    [ -s "$f" ] || { printf 'missing or empty: %s\n' "$f"; fail G3; }
done

# A bare diff cannot say WHY the arms differ, and the two reasons need opposite
# verdicts: RNG contamination voids every arm-vs-arm claim, while an integrity
# rejection propagating through the estimator is the mechanism working. The old
# gate failed on both and asserted the first in its message. It went red the
# moment the check started firing, and CATBL_SKIP_G3=1 was the only way out --
# a waiver that switches off the contamination detector as well.
#
# catbl_g3_ablation.py separates them on causal order: a rejection cannot explain
# a divergence that predates it. See the header there.
python3 scripts/catbl_g3_ablation.py "$FULL" "$NOINT" || fail G3
finish G3

# ------------------------------------------------------------- G4: verifier
printf '\n--- G4 verifier ---\n'
if [ ! -s "$WORK/causal/arm_summary.csv" ]; then
    gerror G4 "G3 produced no arm_summary -- the verifier had nothing to read"
else
    python3 scratch/python_handoff/verify_causal_sim.py --scratch "$WORK/causal" || fail G4
fi
finish G4

# --------------------------------------------------- G5: verifier self-test
# G4 asks whether the verifier passes. G5 asks whether it FAILS when it should --
# which nothing did until the audit, and which is why two holes (a missing arm
# trace, a stale summary) survived in it undetected. Runs in seconds against the
# directory G3 just produced; no simulation.
printf '\n--- G5 verifier self-test ---\n'
if [ ! -s "$WORK/causal/arm_summary.csv" ]; then
    gerror G5 "G3 produced no arm_summary -- the self-test had nothing to run against"
else
    bash scripts/catbl_verifier_selftest.sh "$WORK/causal" || fail G5
fi
finish G5

# ------------------------------- G6: tracked evidence vs a fresh run (read-only)
# The tracked copies are EVIDENCE. This gate reads them and never writes them.
# The two ways it can go red need opposite responses, so it says which:
#   * the fresh run errored           -> the binary or the scenario is broken
#   * the tracked evidence disagrees  -> the committed numbers were produced by a
#                                        different engine; regenerate deliberately
#                                        to a NEW path and repoint citations.
printf '\n--- G6 tracked evidence vs fresh run (read-only) ---\n'
G6_MISMATCH=0
# Repointed to the post-D1 regeneration (baseline_v7, 2026-08-29). The pre-D1
# directories results/{causal,det_a,det_b} are KEPT, with superseded notes; they are
# not compared against a current-engine run because they were produced by a different
# engine and would fail forever. Nothing was overwritten to make this gate green.
# tracked path and fresh path are mapped EXPLICITLY. They are not the same name:
# the tracked evidence lives at results/<x>_v7 while the fresh run writes to
# $WORK/<x>. Deriving one from the other made the gate compare a real file against a
# path that never existed, and report a disagreement where there was none.
for spec in "causal_v7/arm_summary.csv:causal/arm_summary.csv" \
            "det_a_v7/arm_summary.csv:det_a/arm_summary.csv"; do
    tracked="results/${spec%%:*}"
    fresh="$WORK/${spec##*:}"
    if [ ! -s "$fresh" ]; then
        printf 'THE FRESH RUN ERRORED: %s is absent or empty.\n' "$fresh"
        printf 'This is a broken binary or scenario, NOT a disagreement about evidence.\n'
        gerror G6 "fresh run produced no $fresh"
        # `continue` is load-bearing. fail/gerror no longer exit, so without it control
        # falls through to the cmp below and prints "the tracked evidence disagrees"
        # about a file that was never written -- reporting an ERROR as a FAILURE, which
        # is the one distinction this gate exists to preserve.
        continue
    fi
    if [ ! -s "$tracked" ]; then
        printf 'no tracked copy at %s -- nothing to compare against, skipping\n' "$tracked"
        continue
    fi
    if cmp -s "$tracked" "$fresh"; then
        printf 'match: %s\n' "$tracked"
    else
        G6_MISMATCH=1
        printf '\nTHE TRACKED EVIDENCE DISAGREES WITH A FRESH RUN: %s\n' "$tracked"
        printf 'The fresh run completed normally. The committed file was produced by a\n'
        printf 'different engine or configuration than the one built now.\n'
        printf 'Remedy: regenerate deliberately to a NEW path (baseline_v7) and repoint\n'
        printf 'the citations. Do NOT overwrite the tracked copy -- documents cite the\n'
        printf 'numbers the original run produced.\n'
        diff <(head -3 "$tracked") <(head -3 "$fresh") | head -8
    fi
done
[ "$G6_MISMATCH" -eq 0 ] || fail G6
finish G6

# ------------------------- G7: the suite must not have mutated the tree
# A gate suite that rewrites its own subject fails its own run. This is the check
# that would have caught the defect G6's comment describes, the first time it
# happened rather than after two campaigns.
printf '\n--- G7 suite left the tree clean ---\n'
# --untracked-files=no is load-bearing: results/ holds untracked campaign output
# at all times, so the unqualified form reports dozens of `??` entries and the
# gate would fail on every run for a reason that has nothing to do with mutation.
# The question is whether the suite modified TRACKED evidence.
DIRTY=$(git status --porcelain --untracked-files=no -- results/ 2>/dev/null)
if [ -n "$DIRTY" ]; then
    printf 'THE SUITE MUTATED TRACKED RESULTS:\n%s\n' "$DIRTY"
    fail G7
fi
printf 'git status --porcelain -- results/ is empty\n'
finish G7

# ------------------ G8: files the rules require to stay untracked, are untracked
# Enforced by memory until 2026-08-29, when it failed: 35466c0 tracked
# CA_TBL_HANDOFF_v4.md for the first time and aa8328b had to untrack it. A rule
# enforced by recall is not enforced. The list is duplicated in .gitignore, which
# stops the mistake; this gate catches it if .gitignore is bypassed with `git add -f`
# or if a path is already in the index when the ignore rule is added.
printf '\n--- G8 must-stay-untracked files are untracked ---\n'
G8_BAD=0
for f in CA_TBL_HANDOFF_v4.md CA_TBL_HANDOFF_v5.md SESSION_CHANGELOG.md CA_TBL_SIMULATION_REPORT.md; do
    if git ls-files --error-unmatch "$f" >/dev/null 2>&1; then
        printf 'TRACKED BUT MUST NOT BE: %s\n' "$f"
        printf '  remedy: git rm --cached %s   (keeps the file on disk)\n' "$f"
        G8_BAD=1
    else
        printf 'untracked, correct: %s\n' "$f"
    fi
done
[ "$G8_BAD" -eq 0 ] || fail G8
finish G8

# ---------------------- G9: every named deliverable IS tracked, and is committed
# G8 asserts certain files are NOT tracked. G9 is the opposite direction, and nothing
# in the suite could see it: Simulation.md -- one of the two named deliverables --
# had ZERO commits across the entire project and was invisible for its whole life.
# A deliverable that exists only on one machine is not a deliverable.
#
# The second assertion matters as much as the first. A tracked-but-modified
# deliverable means a cited number is sitting uncommitted in the working tree, which
# is how a citation comes to point at something the repository does not contain.
printf '\n--- G9 named deliverables are tracked and committed ---\n'
G9_BAD=0
for f in Architecture.md Simulation.md; do
    if ! git ls-files --error-unmatch "$f" >/dev/null 2>&1; then
        printf 'DELIVERABLE NOT TRACKED: %s\n' "$f"
        printf '  a deliverable that exists only on one machine is not a deliverable\n'
        printf '  remedy: git add %s\n' "$f"
        G9_BAD=1
        continue
    fi
    if [ -n "$(git status --porcelain --untracked-files=no -- "$f")" ]; then
        printf 'DELIVERABLE TRACKED BUT UNCOMMITTED: %s\n' "$f"
        printf '  a cited number is uncommitted in the working tree\n'
        G9_BAD=1
        continue
    fi
    printf 'tracked and clean: %s\n' "$f"
done
[ "$G9_BAD" -eq 0 ] || fail G9
finish G9

# ------------------- G10: every cited path resolves in the repository
# Simulation.md -- a named deliverable -- had zero commits for the whole project
# because nothing ever asked whether the repository contained the things its own
# documents cite. G9 fixed that for the two deliverables; G10 asks it of every
# backtick-cited path in every tracked document.
#
# The allowance list in scripts/citation_audit.py is ENUMERATED, with a reason per
# entry -- deliberately-untracked docs and files inside EXCLUDED.md directories. It
# is not a blanket exemption for anything that happens to be untracked.
printf '\n--- G10 cited paths resolve in the repository ---\n'
python3 scripts/citation_audit.py || fail G10
finish G10

# ------------------------------------------------------------------ summary
printf '\n\n================ GATE SUMMARY (seed=%s duration=%s) ================\n' "$SEED" "$DUR"
SUITE_BAD=0
for g in $GATE_ORDER; do
    st=${GATE_ST[$g]}
    printf '  %-4s %-6s %s\n' "$g" "$st" "${GATE_MSG[$g]:-}"
    [ "$st" = PASS ] || SUITE_BAD=1
done
if [ "${GATE_ST[G6]:-}" = FAIL ]; then
    printf '\n  G6 is FAIL BY DESIGN while pre-D1 evidence is still cited.\n'
    printf '  The remedy is baseline_v7: regenerate to NEW paths and move the citations.\n'
    printf '  Overwriting results/{causal,det_a,det_b} to make it green reintroduces the\n'
    printf '  defect e21666d removed -- a suite that rewrites the evidence it verifies.\n'
fi
if [ "$SUITE_BAD" -eq 0 ]; then
    printf '\n=== ALL GATES PASS (seed=%s duration=%s) ===\n' "$SEED" "$DUR"
    exit 0
fi
printf '\n=== SUITE FAILED -- see summary above ===\n'
exit 1
