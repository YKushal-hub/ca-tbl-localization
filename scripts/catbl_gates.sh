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

pass() { printf '\n=== %s PASS ===\n' "$1"; }
fail() { printf '\n=== %s FAIL ===\n' "$1"; exit 1; }

# ---------------------------------------------------------------- G1: build
printf '\n--- G1 build ---\n'
./ns3 build 2>&1 | tail -40
[ "${PIPESTATUS[0]}" -eq 0 ] || fail G1
pass G1

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
pass G2

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
pass G3

# ------------------------------------------------------------- G4: verifier
printf '\n--- G4 verifier ---\n'
python3 scratch/python_handoff/verify_causal_sim.py --scratch "$WORK/causal" || fail G4
pass G4

# --------------------------------------------------- G5: verifier self-test
# G4 asks whether the verifier passes. G5 asks whether it FAILS when it should --
# which nothing did until the audit, and which is why two holes (a missing arm
# trace, a stale summary) survived in it undetected. Runs in seconds against the
# directory G3 just produced; no simulation.
printf '\n--- G5 verifier self-test ---\n'
bash scripts/catbl_verifier_selftest.sh "$WORK/causal" || fail G5
pass G5

# ------------------------------- G6: tracked evidence vs a fresh run (read-only)
# The tracked copies are EVIDENCE. This gate reads them and never writes them.
# The two ways it can go red need opposite responses, so it says which:
#   * the fresh run errored           -> the binary or the scenario is broken
#   * the tracked evidence disagrees  -> the committed numbers were produced by a
#                                        different engine; regenerate deliberately
#                                        to a NEW path and repoint citations.
printf '\n--- G6 tracked evidence vs fresh run (read-only) ---\n'
G6_MISMATCH=0
for pair in "causal/arm_summary.csv" "det_a/arm_summary.csv"; do
    tracked="results/$pair"
    fresh="$WORK/$pair"
    if [ ! -s "$fresh" ]; then
        printf 'THE FRESH RUN ERRORED: %s is absent or empty.\n' "$fresh"
        printf 'This is a broken binary or scenario, NOT a disagreement about evidence.\n'
        fail G6
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
pass G6

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
pass G7

printf '\n=== ALL GATES PASS (seed=%s duration=%s) ===\n' "$SEED" "$DUR"
