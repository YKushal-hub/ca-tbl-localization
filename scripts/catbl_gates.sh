#!/usr/bin/env bash
# CA-TBL regression gates G1-G13. Run after every code change.
#
# Every gate RUNS; failures are recorded and reported in the summary block at the end,
# and the script exits 1 if any gate failed. It no longer stops at the first failure --
# fail-fast was masking G7-G9 entirely.
#
# HOW TO INVOKE IT. Run it UNPIPED:
#
#     bash scripts/catbl_gates.sh
#
# If you pipe it -- `| tail`, `| tee`, anything -- `$?` is the LAST command's status,
# not this script's, and a red suite reads green. Either check PIPESTATUS:
#
#     bash scripts/catbl_gates.sh | tee gates.log; [ "${PIPESTATUS[0]}" -eq 0 ]
#
# or redirect instead of piping. This project has now hit pipeline-status masking three
# times -- twice inside this script (see the pipefail note below) and once on the script
# itself, where `SUITE FAILED` was printed alongside a shell exit code of 0 from `tail`.
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
# REPOINTED 2026-09-08 to the post-topology generation, which is this gate's own
# documented remedy applied a second time. The 2026-09-07 correction changed the
# DEPLOYMENT -- one population of tri-band nodes instead of three disjoint anchor
# populations -- so results/{causal,det_a}_v7 were produced by a geometry the scenario
# no longer builds and disagreed with every fresh run, permanently and correctly.
#
# Nothing was overwritten to make this gate green: causal_v7 and det_a_v7 are still
# tracked, still cited, and still describe what those runs produced. causal_v8 and
# det_a_v8 are NEW paths, generated at exactly the flags G3 and G2 use so the
# comparison is like for like, on a clean tree at 233f98a with dirty 0.
#
# They are the v8 generation, same as baseline_v8, at DIFFERENT flags: baseline_v8
# carries --nlosSource=propagation --arbitrateDutyBudget=true --pcsForm=legacy and is
# the null-control reference, while these two are pure defaults and are the evidence
# this gate compares against. Do not substitute one for the other.
#
# ITS OWN RUN SINCE 2026-09-29, and the reason is the flag, not the gate. The v11 pair
# was produced when --gatewayEdgeServer defaulted false; it now defaults true, so G2's
# and G3's default runs no longer share the tracked evidence's configuration and a
# comparison with them would disagree on energy for a reason this gate is not about.
# One run at the evidence's recorded arguments with the flag at its ORIGINAL default,
# --trace=false because arm_summary.csv does not depend on tracing (causal_v11 and
# det_a_v11 carry the same digest, 26e85540...). G2-G5 keep testing the shipped default.
mkdir -p "$WORK/g6_nodeside"
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --seed="$SEED" --duration="$DUR" \
    --trace=false --gatewayEdgeServer=false --outdir="$WORK/g6_nodeside" \
    > "$WORK/g6_nodeside.log" 2>&1 || gerror G6 "node-side run exited non-zero"
for spec in "causal_v11/arm_summary.csv:g6_nodeside/arm_summary.csv" \
            "det_a_v11/arm_summary.csv:g6_nodeside/arm_summary.csv"; do
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
printf '\n--- G7 suite did not modify tracked files under results/ ---\n'
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
for f in CA_TBL_HANDOFF_v4.md CA_TBL_HANDOFF_v5.md CA_TBL_HANDOFF_v6.md SESSION_CHANGELOG.md CA_TBL_SIMULATION_REPORT.md; do
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

# --------------------- G12: no attribution in identity or message, any commit
# The rule was violated because nothing enforced it: 27 commit messages carried a
# trailer before the 2026-09-01 rewrite (SHA_MAP.md). Author and committer were
# always correct; the message body was not checked by anything.
printf '\n--- G12 no attribution in commit metadata or messages ---\n'
G12_BAD=0
# Two DIFFERENT patterns, because attribution in an identity field and attribution in
# a message body do not look the same.
#
# The first version used one broad topic-word pattern for both, and it failed on the
# very commit that DOCUMENTS this rule -- the message contained the word "assistant"
# while describing what was removed. A check that cannot survive its own policy being
# written down is too broad. Attribution is a FORM, not a subject: a trailer line, or
# a vendor identity. Prose describing the policy is not attribution.
G12_ID_PAT='cl[a]ude|anthropic|assistant|copilot|\bbot\b'
# NARROWED 2026-09-05 by owner decision: co-authored-by is no longer matched in
# message bodies. The identity-field check above is UNCHANGED and still blocks an
# assistant appearing as author or committer, which is the property that actually
# matters -- who the record says wrote the work. A trailer is a note in prose; an
# identity field is a claim of authorship.
#
# This is a deliberate NARROWING OF SCOPE, not a gate edited to make it pass: G12
# was already GREEN when this change was made, because the trailers had been
# removed by the 2026-09-05 rewrite. The standing project rule in RULES.md 2 is
# unchanged -- do not write attribution -- this only stops the gate enforcing the
# message-body half of it, which the owner deferred.
# REVERTED 2026-09-05, same day, and the reason is worth keeping. This briefly read
# (generated[- ]with|assisted-by:) so that "Generated with <tool>", which carries no
# colon, would be caught. That immediately turned G12 red on the very commit that
# DOCUMENTED the change, because its message contains that phrase at the start of a
# line -- the exact failure the note above already records, reintroduced.
#
# The note above is right and the widening was wrong. Attribution in a COMMIT MESSAGE
# is a git trailer, which is Key: Value. "Generated with X" is not a trailer, it is a
# pull-request body convention, and pull request bodies are not commit messages. The
# colon is what distinguishes the form from prose about the form, and requiring it is
# what lets this gate survive its own policy being written down.
# ANCHORED 2026-09-08, and it is the same lesson a third time. The vendor-handle
# alternatives were the only unanchored ones in this pattern, so they matched a handle
# ANYWHERE IN PROSE -- including in the commits that document this gate. e431eff says
# in its own body "those two alternatives are unanchored, unlike the trailer
# alternatives", diagnosing precisely this and never fixing it; 1164e5d trips on a line
# listing the pattern's own alternatives. Identity fields are clean across every commit.
#
# The note above already states the principle and the trailer alternatives already obey
# it: attribution is a FORM, not a subject, and the colon is what distinguishes the form
# from prose about the form. A vendor handle is attribution when it appears as a
# TRAILER VALUE -- Key: something@anthropic -- and is prose when it appears in a
# sentence. Anchored to that form, both documenting commits pass and a real trailer
# still does not.
#
# co-authored-by lines are stripped BEFORE matching, which preserves the 2026-09-05
# owner narrowing exactly: that trailer is deliberately not enforced in message bodies,
# and anchoring the handles to trailer form would otherwise have re-widened it by the
# back door.
# TIGHTENED 2026-09-08, hours later, because the anchoring commit tripped its own gate
# -- the FOURTH time this check has fired on a commit that documents it. 233f98a's body
# lists its negative fixtures as literal trailer lines, and "Signed-off-by: bot
# <x@anthropic.com>" is indistinguishable from the real thing unless indentation counts.
#
# It counts, and it is the right discriminator rather than a convenient one. A GIT
# TRAILER SITS AT COLUMN 0 in the trailer block; git's own parser will not read an
# indented line as a trailer. So an indented Key: Value is documentation quoting the
# form, and an unindented one is the form. Requiring column 0 is therefore STRICTER
# about what counts as a trailer, not laxer: nothing git would treat as attribution can
# hide behind indentation, and prose quoting a fixture no longer convicts.
#
# The leading [[:space:]]* was inherited from the original pattern and was never
# examined. It is what made every documenting commit an offender.
G12_MSG_PAT='^(generated[- ]with|assisted-by):|^[A-Za-z][A-Za-z0-9-]*:[[:space:]]*[^:]*@(anthropic|openai)'
n_id=$(git log --format='%an|%ae|%cn|%ce' | grep -icE "$G12_ID_PAT" || true)
if [ "${n_id:-0}" -ne 0 ]; then
    printf 'ATTRIBUTION IN IDENTITY FIELDS: %s commit(s)\n' "$n_id"
    G12_BAD=1
else
    printf 'identity fields clean across %s commits\n' "$(git rev-list --count HEAD)"
fi
n_msg=0
while read -r h; do
    if git log -1 --format='%B' "$h" | grep -viE '^[[:space:]]*co-authored-by:' | grep -qiE "$G12_MSG_PAT"; then
        n_msg=$((n_msg + 1))
        [ "$n_msg" -le 3 ] && printf '  offending commit: %s %s\n' "$(git rev-parse --short "$h")" "$(git log -1 --format='%s' "$h" | cut -c1-50)"
    fi
done < <(git log --format='%H')
if [ "$n_msg" -ne 0 ]; then
    printf 'ATTRIBUTION IN MESSAGE BODIES: %d commit(s)\n' "$n_msg"
    printf '  remedy: this requires a history rewrite and is authorised only by the owner\n'
    G12_BAD=1
else
    printf 'message bodies clean\n'
fi
[ "$G12_BAD" -eq 0 ] || fail G12
finish G12

# ------------------------ G11: the propagation study, under the gates
# Wired in 2026-09-02, after validate.py was rebuilt to COMPUTE the link budget rather
# than read it from the probe. It is RED, for an accuracy reason and not a
# construction one: the model omits foliage loss, which the engine applies. The
# precedent is G6, which sat red by design with its remedy recorded beside it.
# The propagation sweeps stay blocked while it is red -- that is the point.
printf '\n--- G11 propagation study: corpus, reconciliation, tracking ---\n'
python3 scripts/propagation_study_gate.py || fail G11
finish G11

# ---------------- G13: every sidecar SHA resolves, in history or via SHA_MAP.md
# A run_id.txt's `source <sha>` is the only link between a results directory and the
# code that produced it. The 2026-09-01 attribution rewrite moved 30 commits and left
# 1283 sidecars citing SHAs no longer in history; they resolve through SHA_MAP.md.
# This asserts there is no THIRD case -- a SHA that exists in neither place.
printf '\n--- G13 sidecar source SHAs resolve ---\n'
python3 scripts/sidecar_sha_audit.py || fail G13
finish G13

# ---------------- G14: the sensitivity tables and their SF index agree
# A7. The six-decibel SF-index defect -- `sf - 7` against tables whose index 0 is SF5 --
# sat on the tier carrying ~95% of active cycles and PASSED EVERY GATE, VERIFIER CHECK
# AND VALIDATION IN THIS PROJECT. What found it was an independent closed-form model
# refusing to reconcile, not a check.
#
# The existing fidelity checks verify ranging precision and energy accounting, both of
# which are downstream of whether a packet was received at all. None of them reads the
# sensitivity table. This one does, and it asserts the table's declared base against the
# index expression the engine actually uses.
#
# Its negative fixtures are in scripts/sensitivity_gate_selftest.sh and include D1
# itself. Run BOTH -- a gate that has only ever passed proves nothing (rule 6).
printf '\n--- G14 sensitivity table SF-index fidelity ---\n'
python3 scripts/sensitivity_gate.py || fail G14
bash scripts/sensitivity_gate_selftest.sh || fail G14
finish G14

# ---------------- G15: relayed-range admission (C5), A1-A5 with negative fixtures
# A relayed range is NOT a range. r_AB + r_BG is an upper bound on |A-G| by the triangle
# inequality, tight only when the three are collinear, and handing it to the filter as a
# range is the same category error as applying a ToA Cramer-Rao bound to RSSI inversion
# -- which this project has already made once. What a relay supplies is a pseudo-anchor
# with a covariance: sigma_eff^2 = sigma_range^2 + u^T P_B u.
#
# The five admission conditions each mark a place where the error model STOPS HOLDING,
# not a place where the measurement is merely poor. A poor measurement can be
# down-weighted; one whose error model is wrong cannot, because the weight would be
# computed from the same wrong model.
#
# The suite is its own negative fixture set (rule 6): each condition is violated in turn
# and must produce ITS OWN rejection code, so one over-broad condition cannot mask the
# absence of the other four. The positive control is the same measurement with the
# single violation removed.
printf '\n--- G15 relayed-range admission (C5) ---\n'
./build/scratch/ns3.47-ca_tbl_relay_admission_test-optimized || fail G15
finish G15

# ---------------- G16: the anchor grant tie-break is REACHED (L.1.4, Network sect 6)
# CaTblGrantCoordinator had a passing unit suite and zero scenario call sites: the rule
# was right and nothing ran it. A unit suite cannot see that; only a run can. This runs
# the arc deployment (NC2's flags, the regime in which one tier-1 ESC_REQUEST reaches
# several gateways at once) with --anchorGrantGossipS=5 and requires, in the verdict file
# the run writes, two gateways reaching OPPOSITE verdicts on one field node -- the higher
# id WIN, the lower id DEFER -- with every row obeying the lower-id-defers rule.
#
# Negative fixtures in scripts/grant_tiebreak_gate_selftest.sh, including the same run
# with the flag off, which must FAIL (rule 6). Both run; neither alone is the gate.
printf '\n--- G16 anchor grant tie-break reached (L.1.4) ---\n'
mkdir -p "$WORK/g16"
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --arms=ca_tbl_full --seed=7 \
    --duration=1800 --trace=false --nFinders=4 --finderPseudoAnchor=true --finderMode=2 \
    --finderDispersal=1.0 --sendHandshakeFrames=true --finderDigestPeriodS=20 \
    --boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180 \
    --anchorGrantGossipS=5 --outdir="$WORK/g16" > "$WORK/g16.log" 2>&1 \
    || gerror G16 "flag-on run exited non-zero"
python3 scripts/grant_tiebreak_gate.py "$WORK/g16/grant_verdicts_ca_tbl_full_seed7.csv" \
    || fail G16
bash scripts/grant_tiebreak_gate_selftest.sh || fail G16
finish G16

# ---------------- G17 / G17b: relay depth 2 (L.1.2, Network sect 5)
# CaTblComposeRelayChain, --maxRelayDepth and --relayVarBudget existed and the scenario
# hard-set hopDepth = 1. --relayChain lets a relay short of three anchors range a relay
# that holds a fix this cycle, and take that relay's depth + 1 when its step FUSES the
# range. This runs it (static_2p4, 600 s, four tags, budget 156^2 -- one tier-2 r80) and
# reads the decision log the run writes.
#
# G17 requires the chain wired and honest: every A2 label agreeing with its row's numbers,
# every admitted sigma_eff within [sigma_raw, sqrt(sigma_raw^2 + tr P)], every depth >= 2
# preceded by the fuse that earned it, and the budget half of A2 seen both admitting and
# refusing. GREEN.
#
# G17b is the L.1.2 done-when: depth 2 reached, and A2 admitting and refusing on the
# budget AT depth 2. It needs the relays' round (--relaySlots, C5_INSTANT). Without it every
# relay exchange in a cycle is stamped with the cycle's one instant and the engines'
# interference registry treats them as simultaneous, so no two relays hold a fix in one
# cycle and no relay-to-relay exchange delivers: that was G17's own run until 2026-09-30,
# and G17b was red from b390ed6 until the round got instants of its own. With it, 4 fuses
# and 40 depth-2 decisions (A2 at depth 2: pass 19, budget 21). --stdout recounts the run's
# own counters, so the green is traced to the round: every relay-to-relay exchange moved,
# the round inside the cycle, the CSV's fuse rows equal to the run's.
#
# SINCE 2026-09-30 THE ROUND IS THE DEFAULT in every relayed run (--relaySlots=auto), so
# neither gate names the flag: G17's run is at the default, and G17b reads that same run.
# Until then G17b ran its own copy with --relaySlots=true. The two runs are byte-identical
# (every output file; only the stderr NOTE that the default engaged differs), so reading
# G17's is the same evidence without the second run, and a default that reverted would
# turn G17b red through --stdout (no round line) and the fuse count -- which is N14.
#
# Negative fixtures in scripts/relay_chain_gate_selftest.sh, including the same run with
# the chain off, G17b's run with --relaySlots=false (N14), and the first draft's own
# depth-from-nowhere rows (rule 6).
printf '\n--- G17 relay chain wired, A2 and provenance honest (L.1.2) ---\n'
mkdir -p "$WORK/g17"
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --arms=static_2p4 --seed=7 \
    --duration=600 --trace=false --boxHalf=200 --nTags=4 --relayedRanging=true \
    --relayChain=true --maxRelayDepth=2 --relayVarBudget=24336 --outdir="$WORK/g17" \
    > "$WORK/g17.log" 2>&1 || gerror G17 "flag-on run exited non-zero"
python3 scripts/relay_chain_gate.py "$WORK/g17/relay_chain_static_2p4_seed7.csv" || fail G17
bash scripts/relay_chain_gate_selftest.sh || fail G17
finish G17

printf '\n--- G17b relay depth 2 reached, A2 on the budget at depth 2 (L.1.2 done-when) ---\n'
if [ ! -s "$WORK/g17/relay_chain_static_2p4_seed7.csv" ] || [ ! -s "$WORK/g17.log" ]; then
    gerror G17b "G17's run wrote no chain log or no stdout -- nothing to read"
else
    python3 scripts/relay_chain_gate.py --require-depth2 --stdout "$WORK/g17.log" \
        "$WORK/g17/relay_chain_static_2p4_seed7.csv" || fail G17b
fi
finish G17b

# ---------------- G18 / G18b: position fixes routed (L.1.1, Network sect 2-3)
# CaTblRoutingTable, the ETX baseline, the confidence metric and Divergences() had a
# passing unit suite and zero scenario call sites. --fixRouting builds a table on every
# relay from the links it has heard and routes each relay's ENTRY_PUSH to the gateway
# backbone hop by hop, choosing on ETX while scoring the confidence metric in the same
# call; --fixRoutingConfidence makes the confidence metric the selector. This runs both
# (static_2p4, 600 s, five relays, default box: the relays dwell out of 2.4 GHz reach of
# every gateway, so most fixes must cross a relay) and reads the hop log each writes.
#
# G18 is the L.1.1 done-when under ETX: a fix handed to a next hop that is not a gateway,
# a relayed fix delivered, Divergences() non-zero -- with every row consistent with the
# metric that routed it, every relayed hop continuous with the forward behind it, and the
# CSV recounted against the run's own counters (router = tables = CSV).
# G18b is the same file contract under the confidence selector, and requires a diverged
# hop that the fix actually crossed: traffic sent where ETX would not have sent it.
#
# SINCE 2026-09-30 both runs are at the --relaySlots default, which gives the relays'
# ranging round instants of its own in every relayed run (C5_INSTANT; TECH_DEBT M.1), and
# --require-round makes both gates TEST that default rather than merely run under it: the
# log must show the round. Until then both ran with every relay exchange at the cycle's one
# instant, which that run still reproduces byte-for-byte with --relaySlots=false.
#
# Negative fixtures in scripts/fix_routing_gate_selftest.sh, including the same run with
# the flag off, a real ETX-routed log offered to G18b, and G18's own run with
# --relaySlots=false offered with and without --require-round (P4/N14) (rule 6).
printf '\n--- G18 position fixes routed, ETX and confidence diverge (L.1.1) ---\n'
mkdir -p "$WORK/g18" "$WORK/g18b"
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --arms=static_2p4 --seed=7 \
    --duration=600 --trace=false --relayedRanging=true --nTags=6 --fixRouting=true \
    --outdir="$WORK/g18" > "$WORK/g18.log" 2>&1 || gerror G18 "flag-on run exited non-zero"
python3 scripts/fix_routing_gate.py --require-round --stdout "$WORK/g18.log" \
    "$WORK/g18/fix_routing_static_2p4_seed7.csv" || fail G18
bash scripts/fix_routing_gate_selftest.sh || fail G18
finish G18

printf '\n--- G18b position fixes routed BY the confidence metric (L.1.1 done-when) ---\n'
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --arms=static_2p4 --seed=7 \
    --duration=600 --trace=false --relayedRanging=true --nTags=6 --fixRouting=true \
    --fixRoutingConfidence=true --outdir="$WORK/g18b" > "$WORK/g18b.log" 2>&1 \
    || gerror G18b "flag-on run exited non-zero"
python3 scripts/fix_routing_gate.py --require-confidence --require-round \
    --stdout "$WORK/g18b.log" "$WORK/g18b/fix_routing_static_2p4_seed7.csv" || fail G18b
finish G18b

# ---------------- G19: tier-1 TDoA produced, fused and billed (L.2.1, Physical sect 1.1)
# The energy model has billed tier 1 as a TDoA exchange -- one broadcast the gateways
# timestamp, a reply per gateway -- while the estimator used RSSI inversion alone.
# --tier1Tdoa builds the observable that bill describes: surveyed gateways stamp the
# broadcast on GPS-PPS time, the differences are solved as hyperbolae (Chan, Foy, the
# three-anchor closed form), and the fix is fused with its covariance beside the same
# cycle's RSSI ranges. It adds no frame and no charge.
#
# G19 runs the solver's unit suite (33 checks, its refusals among them), then the
# mechanism in a run, in both billing modes -- replies billed (--gatewayEdgeServer=false,
# pinned since that flag's default became true on 2026-09-29, or this run would silently
# become a second edge-server run and the reply half of the billing check would go
# unexercised) and the edge server's single downlink -- and reads the per-cycle log: OK
# solves on at least half the
# tier-1 fix cycles, every OK fix on an accepted cycle fused, every stamp on a billed
# frame, and the CSV recounted against the scenario's and the policy's own counters.
#
# Negative fixtures in scripts/tdoa_gate_selftest.sh, including the same run with the flag
# off, the flag on in an arm that never uses tier 1, and the first draft's own runaway
# fix (rule 6). All run; none alone is the gate.
printf '\n--- G19 tier-1 TDoA produced, fused and billed (L.2.1) ---\n'
mkdir -p "$WORK/g19" "$WORK/g19e"
./build/scratch/ns3.47-ca_tbl_tdoa_test-optimized || fail G19
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --arms=static_868 --seed=7 \
    --duration=900 --trace=false --tier1Tdoa=true --gatewayEdgeServer=false \
    --outdir="$WORK/g19" > "$WORK/g19.log" 2>&1 || gerror G19 "flag-on run exited non-zero"
python3 scripts/tdoa_gate.py --stdout "$WORK/g19.log" \
    "$WORK/g19/tdoa_static_868_seed7.csv" || fail G19
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --arms=static_868 --seed=7 \
    --duration=900 --trace=false --tier1Tdoa=true --gatewayEdgeServer=true \
    --outdir="$WORK/g19e" > "$WORK/g19e.log" 2>&1 || gerror G19 "edge-server run exited non-zero"
python3 scripts/tdoa_gate.py --stdout "$WORK/g19e.log" \
    "$WORK/g19e/tdoa_static_868_seed7.csv" || fail G19
bash scripts/tdoa_gate_selftest.sh || fail G19
finish G19

# ---------------- G20: NLOS scores produced, checked and fed to PCS (L.2.2, Physical 1.3/2.5)
# PCS term 3 has read a boolean tag since it was written (K.8: the tag fires on 75-78% of
# links). --nlosClassifier gives every ranged link the spec's tier-specific NLOS_score --
# a synthesised UWB CIR's leading edge against its peak, the 2.4 GHz ToF distance against
# the RSSI, the 868 MHz link's RSSI/SNR variability -- and scores each against the channel
# model's own LOS/NLOS draw. --pcsNlosClassifier makes term 3 mean(1 - NLOS_score).
#
# G20 runs the unit suite, then the same short run three ways. OFF and CLASSIFIER-ONLY
# must write byte-identical arm_summary.csv: the scores decide nothing until fed, and the
# CIR draws from its own stream. Both flag-on runs are read by the gate: the matrix is its
# rows, the rows honour their detector, one tier has every cell >= 5, the term varies, and
# it is fed on every scored cycle exactly when the feed is on.
#
# THE CIR CANNOT FILL ITS OWN MATRIX IN ANY SCENARIO RUN, and G20 does not pretend it can:
# no decoded UWB link is NLOS in the model (the UWB NLOS penalty exceeds UWB's margin, and
# at UWB distances the outdoor NLOS probability is ~0.1%). The matrix G20 requires full is
# the 868 one. Negative fixtures, including the flag off, the classifier-only run offered
# as a feed run, and a UWB-only run, are in scripts/nlos_classifier_gate_selftest.sh (rule 6).
printf '\n--- G20 NLOS scores produced, checked and fed to PCS (L.2.2) ---\n'
mkdir -p "$WORK/g20off" "$WORK/g20d" "$WORK/g20f"
./build/scratch/ns3.47-ca_tbl_nlos_test-optimized || fail G20
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --arms=ca_tbl_full --seed=7 \
    --duration=900 --trace=false --outdir="$WORK/g20off" \
    > "$WORK/g20off.log" 2>&1 || gerror G20 "flag-off run exited non-zero"
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --arms=ca_tbl_full --seed=7 \
    --duration=900 --trace=false --nlosClassifier=true --outdir="$WORK/g20d" \
    > "$WORK/g20d.log" 2>&1 || gerror G20 "classifier-only run exited non-zero"
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --arms=ca_tbl_full --seed=7 \
    --duration=900 --trace=false --nlosClassifier=true --pcsNlosClassifier=true \
    --outdir="$WORK/g20f" > "$WORK/g20f.log" 2>&1 || gerror G20 "feed run exited non-zero"
if cmp -s "$WORK/g20off/arm_summary.csv" "$WORK/g20d/arm_summary.csv"; then
    printf '  G20 classifier alone: arm_summary.csv byte-identical to the flag off\n'
else
    printf '  G20 classifier alone CHANGED arm_summary.csv -- it must decide nothing unfed\n'
    fail G20
fi
python3 scripts/nlos_classifier_gate.py --forbid-fed --stdout "$WORK/g20d.log" \
    "$WORK/g20d/nlos_links_ca_tbl_full_seed7.csv" \
    "$WORK/g20d/nlos_summary_ca_tbl_full_seed7.csv" || fail G20
python3 scripts/nlos_classifier_gate.py --require-fed --stdout "$WORK/g20f.log" \
    "$WORK/g20f/nlos_links_ca_tbl_full_seed7.csv" \
    "$WORK/g20f/nlos_summary_ca_tbl_full_seed7.csv" || fail G20
bash scripts/nlos_classifier_gate_selftest.sh || fail G20
finish G20

# ---------------- G20b: --uwbNlosModel=ieee802154a is report/PREREG_UWB_NLOS.md sect 2 (M.2)
# The registered model: the UWB LOS/NLOS state from the 3GPP InF-SL law, and on an NLOS
# draw the IEEE 802.15.4a NLOS-minus-LOS path loss, deterministic in distance. The
# formula on EVERY evaluation, decoded or lost, is checked by the unit suite G20 has just
# run ("engine, ieee802154a"). This gate checks the scenario: in a short legacy-topology
# UWB run with the model on, the NLOS share of polled UWB links is far above anything the
# legacy model can produce (so the flag reaches the engine through the config macro), every
# decoded NLOS row carries dPL(distM) to 1e-6 dB and every LOS row 0 (the registration's P4).
# Its configuration is deliberately NOT one of the registered cells. The default stays
# legacy, whatever the registered runs show; G2-G6's null behaviour covers that.
#
# Negative fixtures in scripts/uwb_nlos_model_gate_selftest.sh, including the same run with
# the model off (rule 6).
printf '\n--- G20b UWB NLOS model is the registered one (M.2) ---\n'
mkdir -p "$WORK/g20b"
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --topology=1 --arms=static_uwb \
    --envFacility=0 --vegFacility=0 --uwbTriad=1 --nlosClassifier=true --duration=600 \
    --trace=false --seed=101 --uwbNlosModel=ieee802154a --outdir="$WORK/g20b" \
    > "$WORK/g20b.log" 2>&1 || gerror G20b "model-on run exited non-zero"
python3 scripts/uwb_nlos_model_gate.py "$WORK/g20b/nlos_links_static_uwb_seed101.csv" \
    "$WORK/g20b/nlos_summary_static_uwb_seed101.csv" || fail G20b
bash scripts/uwb_nlos_model_gate_selftest.sh || fail G20b
finish G20b

# ---------------- G21: the estimator-location default moves energy and nothing else (L.2.3)
# --gatewayEdgeServer became TRUE by default on 2026-09-29 on the evidence that it changes
# what the node is billed and not what the system does (B3 C6: seven accuracy columns
# byte-identical on 20/20 seeds). This keeps that true. It reads two runs the suite has
# already made, at the same seed and duration from the same binary: G3's, at defaults, and
# G6's, with --gatewayEdgeServer=false (arm_summary.csv does not depend on --trace). Every
# non-energy column must match, tx_/cad_/sleep_ included; every row with a fix must differ
# in totalJ, and every row without one must not; an unused tier must not be charged.
#
# AND WHICH RADIO THE DOWNLINK IS BILLED ON (TECH_DEBT M.10, 2026-09-30). Since then it is
# billed on the tier the fix came from, so a hybrid arm's floor-solved fix is an 868 MHz
# downlink rather than one on the active tier. One more run, the same seed and duration
# at --edgeDownlinkTier=active (the old bill), is compared with G3's: only energy may
# move, only as a transfer onto 868 MHz from the active tier, only where 868 MHz
# transmitted, and at least one row must move -- so a reverted default goes red here.
#
# Negative fixtures in scripts/edge_default_gate_selftest.sh, built from a fresh real
# triple, including the node-side run offered as both sides -- the edge path dead -- and
# the active-tier run offered as the defaults -- the downlink default reverted (rule 6).
printf '\n--- G21 the edge-server default moves energy and nothing else (L.2.3, M.10) ---\n'
mkdir -p "$WORK/g21_active"
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --seed="$SEED" --duration="$DUR" \
    --trace=false --edgeDownlinkTier=active --outdir="$WORK/g21_active" \
    > "$WORK/g21_active.log" 2>&1 || gerror G21 "active-tier run exited non-zero"
if [ ! -s "$WORK/causal/arm_summary.csv" ] || [ ! -s "$WORK/g6_nodeside/arm_summary.csv" ] \
   || [ ! -s "$WORK/g21_active/arm_summary.csv" ]; then
    gerror G21 "G3's, G6's or the active-tier run produced no arm_summary -- nothing to compare"
else
    python3 scripts/edge_default_gate.py "$WORK/causal/arm_summary.csv" \
        "$WORK/g6_nodeside/arm_summary.csv" \
        --active "$WORK/g21_active/arm_summary.csv" || fail G21
fi
bash scripts/edge_default_gate_selftest.sh || fail G21
finish G21

# ---------------- G28: the spec's calibrated PCS is expressible and reaches the policy (DC-6)
# The spec's PCS is w1*t1 + w2*t2 + w3*t3 with weights AND thresholds calibrated by
# leave-one-out (Physical 2.5). Until 2026-09-30 neither could be given: the thresholds
# had no flag and a weight of 0 -- LOOCV's answer is (3, 0, 0) -- was silently the default.
# 18 short runs, read by scripts/pcs_calibrated_gate.py: --pcsForm=calibrated at weights
# (1, 1, 1) and the legacy thresholds must reproduce --pcsForm=legacy byte-for-byte, trace
# included; each of the three observable thresholds, moved alone, must move the decision
# it owns; --pcsW2=0 must remove exactly the geometry term from every scored cycle; what
# was resolved must sit in each flag's own tier slot; and a missing value, an inverted
# pair (explicit or resolved against a built-in) and the 868 MHz down threshold, which
# decides nothing, are refused. The UWB thresholds' EFFECT is not checked, and the gate
# says why: no regime this build has delivers a UWB fix.
#
# Negative fixtures in scripts/pcs_calibrated_gate_selftest.sh, built from these runs,
# including --pcsW2=0 ignored (the tree before this gate), a threshold that reaches
# nothing, two tiers' slots swapped, and an inverted pair accepted (rule 6).
printf '\n--- G28 the calibrated PCS is expressible and reaches the policy (DC-6) ---\n'
bash scripts/pcs_calibrated_gate.sh "$WORK/g28" || fail G28
bash scripts/pcs_calibrated_gate_selftest.sh "$WORK/g28" || fail G28
finish G28

# ---------------- G29: a SUSPECT fix is downweighted, not rejected (DC-7, Physical 2.6)
# The spec flags a suspect fix, forbids it alone to justify a de-escalation, and has it
# "downweight[ed] in PCS regardless of raw trace(Sigma)". The default rejects it. Under
# --suspectRecovery=downweight it is fused at R x --suspectInflateFactor and its PCS is
# scaled by --suspectPcsFactor. G29 runs the integrity check's unit suite (21 checks, DC-8's included),
# then six runs at NC8's configuration, read by scripts/suspect_downweight_gate.py.
# Not one suspect fix is rejected, and every one is counted. At inflate 1 and factor 1,
# downweight IS bounded with N = 1, differing in the trace's `rejected` column only. The
# PCS factor alone moves pcs, only on suspect cycles, by exactly x 0.5. The inflate
# factor alone leaves a larger trace(P) at the first suspect cycle. The factor is refused
# where it would do nothing.
#
# Negative fixtures in scripts/suspect_downweight_gate_selftest.sh, built from these
# runs, including the default's run offered as the downweight run -- the tree before DC-7
# -- and each factor not applied (rule 6).
printf '\n--- G29 a SUSPECT fix is downweighted, not rejected (DC-7) ---\n'
./build/scratch/ns3.47-ca_tbl_integrity_test-optimized || fail G29
bash scripts/suspect_downweight_gate.sh "$WORK/g29" || fail G29
bash scripts/suspect_downweight_gate_selftest.sh "$WORK/g29" || fail G29
finish G29

# ---------------- G30: the integrity check's eligibility is read from the NLOS score (DC-8)
# The spec calls a fix SUSPECT on Mahalanobis disagreement "AND NLOS_score is LOW"
# (Physical 2.6). The default reads "every link tagged LOS", and under the default tag that
# set is empty -- the control below evaluates ~300 fixes at seed 7 and finds 0 all-LOS, so
# the check can never fire (TECH_DEBT I.1). --integrityNlosFromScore reads the mean score
# of the fix set's scored links. G29 has already run the unit suite, whose DC-8 section
# checks the rule itself. Three runs, read by scripts/integrity_score_gate.py:
#   - the policy's own record of each evaluated fix (integrity_*) is recounted from the
#     scorer's nlos_links_* rows -- the fix set, its scored links, their mean;
#   - every verdict follows the rule at the threshold the run was given (0.5 and 0.1);
#   - the trace, arm_summary and stdout agree with the record;
#   - only the checking arm writes it;
#   - the flag without the classifier is refused.
#
# Negative fixtures in scripts/integrity_score_gate_selftest.sh, built from these runs,
# including the control offered as the flag run (the tree before DC-8) and the 0.1 run's
# verdicts taken at 0.5 (a threshold that does not arrive) (rule 6).
printf '\n--- G30 the integrity check reads the NLOS score (DC-8) ---\n'
bash scripts/integrity_score_gate.sh "$WORK/g30" || fail G30
bash scripts/integrity_score_gate_selftest.sh "$WORK/g30" || fail G30
finish G30

# ---------------- G31: table entries carry the producer's velocity (DC-10, Physical 2.1)
# The spec's entry carries a velocity estimate. The wire format billed the two fields,
# and every producer published 0, 0. Under --entryVelocity:
#   - a field node publishes its EKF velocity;
#   - a finder publishes its GNSS velocity, the displacement over the time since its last
#     publication;
#   - a gateway publishes 0.
# Two pairs of runs, flag off and on, read by scripts/entry_velocity_gate.py: the NC2
# searcher regime, where entries travel by digest, and the NC4 relay regime, where peers
# publish too. Nothing reads the velocity yet, so the flag must move no other output, byte
# for byte. Every producer's read-back publication must equal what it was handed. Each
# finder's is recounted from its own published displacement. Every held entry must carry
# the velocity published under its sequence number. Gateways must carry 0.
#
# Negative fixtures in scripts/entry_velocity_gate_selftest.sh, built from these runs,
# including every velocity 0 (the tree before DC-10), a trace that moved when the flag
# came on, and a velocity changed in transit (rule 6).
printf '\n--- G31 table entries carry the producer velocity (DC-10) ---\n'
bash scripts/entry_velocity_gate.sh "$WORK/g31" || fail G31
bash scripts/entry_velocity_gate_selftest.sh "$WORK/g31" || fail G31
finish G31

# ---------------- G32: each tier's identity-query driver sends on its own radio (NW-3)
# Every tier's C3 query driver was bound to the first anchor MAC -- gateway 101's 868 MHz
# radio -- so a "2.4 GHz" query went out with 868 MHz's SF and bandwidth (TECH_DEBT L.2).
# --queryTierBinding=per-tier binds each to that tier's radio; given explicitly, either value
# prints the binding. Three runs in NC5's query regime, read by
# scripts/query_binding_gate.py:
#   - explicit first is the default, byte for byte;
#   - under first, the bound MAC's airtime decomposes EXACTLY into 868 MHz and UWB frame
#     times -- the defect, observed;
#   - under per-tier, each driver is on its tier's radio at that tier's time on air,
#     recomputed from the Semtech and 802.15.4z formulas;
#   - the flag without queries is refused.
#
# Negative fixtures in scripts/query_binding_gate_selftest.sh, built from these runs,
# including per-tier binding nothing (the tree before NW-3) and a 2.4 GHz driver billed at
# 868 MHz airtime (rule 6).
printf '\n--- G32 each query driver sends on its own tier radio (NW-3) ---\n'
bash scripts/query_binding_gate.sh "$WORK/g32" || fail G32
bash scripts/query_binding_gate_selftest.sh "$WORK/g32" || fail G32
finish G32

# ---------------- G33: the escalation handshake as MAC sect 8 specifies it (MC-1)
# By default an ACK clears its receiver's OLDEST pending frame, an anchor ACKs a request
# before arbitrating (so a denial is never retried), the pre-emption send carries no terms,
# the others carry the previous cycle's, and de-escalations are signalled too.
# --handshakeFixes fixes all five. Three runs in NC9's regime with arbitration and two peer
# tags, read by scripts/handshake_fixes_gate.py:
#   - --handshakeFixes=false is the default, byte for byte;
#   - the default's bare ACKs and off-UWB requests, observed;
#   - under the flag: every request says target=2 and sits on an escalation into UWB; every
#     ACK names a GRANT/RESPONSE; every GRANT names a request and says granted or denied;
#     denials are retried; the terms are this cycle's, recomputed from the trace;
#   - the flag without handshake frames is refused.
#
# Negative fixtures in scripts/handshake_fixes_gate_selftest.sh, built from these runs
# (rule 6). The MAC cases (ACK by id, deny then retry) are in ca_tbl_packet_validator [7].
printf '\n--- G33 the escalation handshake as MAC sect 8 specifies it (MC-1) ---\n'
bash scripts/handshake_fixes_gate.sh "$WORK/g33" || fail G33
bash scripts/handshake_fixes_gate_selftest.sh "$WORK/g33" || fail G33
finish G33

# ---------------- G34: the spec's triggered update and last gasp (MC-2)
# By default the only TRIGGERED_UPDATE is StartHandshake's RECEDING push and nothing counts
# it (TECH_DEBT M.9). --triggeredUpdates adds a large-correction push and a last gasp for
# the measured node. Three runs at seed 7 with a 3 J battery, read by
# scripts/triggered_updates_gate.py:
#   - --triggeredUpdates=false is the default, byte for byte;
#   - each large correction agrees with the trace (estimate, threshold, active tier);
#   - every reason-coded frame follows its trigger; the last gasp is the last frame the
#     node originated; the stdout counts equal the CSV's;
#   - --triggeredUpdateK without the flag is refused.
#
# Negative fixtures in scripts/triggered_updates_gate_selftest.sh, built from these runs
# (rule 6). The MAC cases are in ca_tbl_packet_validator [8].
printf '\n--- G34 the spec triggered update and last gasp (MC-2) ---\n'
bash scripts/triggered_updates_gate.sh "$WORK/g34" || fail G34
bash scripts/triggered_updates_gate_selftest.sh "$WORK/g34" || fail G34
finish G34

# ---------------- G35: the uplink of a tier-2/3 range to the gateway estimator (SY-1)
# With the estimator on the gateways (the default), a range measured by two-way time of
# flight is left at the node and the frame that carries it up was never billed (TECH_DEBT
# L.2.3). --edgeUplinkRanges bills it. Three runs of all six arms (seed 7, 900 s), read by
# scripts/edge_uplink_gate.py:
#   - --edgeUplinkRanges=false is the default, byte for byte;
#   - only energy moves: every non-energy summary column and every trace column but cumJ
#     is byte-identical; tier-2/3 tx_/airtime_ only rise; a silent tier is never charged;
#   - static_2p4 is billed; the extra joules per extra second are one TX power per radio;
#     the stdout airtime equals the bill;
#   - the flag with the estimator on the node is refused.
#
# Negative fixtures in scripts/edge_uplink_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G35 the uplink of a tier-2/3 range to the gateways is billed (SY-1) ---\n'
bash scripts/edge_uplink_gate.sh "$WORK/g35" || fail G35
bash scripts/edge_uplink_gate_selftest.sh "$WORK/g35" || fail G35
finish G35

# ---------------- G36: tier-1 and tier-2 SF and bandwidth from the CLI (SY-3)
# Physical 1.1 makes SF7-SF12 and 125/250/500 kHz configurable; phy[] was hard-coded.
# --sf868/--bw868/--sf2p4/--bw2p4 replace it when given, and a non-default bandwidth
# shifts the engine's sensitivity table by 10 log10(BW/BW0). Four runs (seed 7, 600 s),
# read by scripts/phy_flags_gate.py:
#   - the four flags at their defaults are the default, byte for byte;
#   - the SY-3 PHY line's offset and time on air, recomputed;
#   - every arm's engine sensitivity is the table entry plus the offset;
#   - static_868's airtime is cycles x ToA at the run's SF/BW, and static_2p4's a whole
#     number of frames;
#   - out-of-range values are refused.
#
# Negative fixtures in scripts/phy_flags_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G36 tier-1 and tier-2 SF and bandwidth from the CLI (SY-3) ---\n'
bash scripts/phy_flags_gate.sh "$WORK/g36" || fail G36
bash scripts/phy_flags_gate_selftest.sh "$WORK/g36" || fail G36
finish G36

# ---------------- G37: the transmit-power-drop and antenna-loss fault classes (SY-5)
# The docx's evaluation induces cold-soaked batteries and detuned or covered antennas;
# only range bias, silence and depletion existed. --injectTxPowerDropDb and
# --injectAntennaLossDb lower the measured node's link budget over the seeded
# --injectBiasFraction window. Four runs (seed 7, 900 s), read by
# scripts/link_fault_gate.py:
#   - magnitudes of 0 are no fault, byte for byte;
#   - the window is the seeded one, and the fault acts in it and not before;
#   - the tier-1 skew factor is 10^(dB/(10 n)), applied on > 0 links;
#   - the drop is transmit-only, the antenna loss reciprocal;
#   - a magnitude without its window, or a negative one, is refused.
#
# Negative fixtures in scripts/link_fault_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G37 the transmit-power-drop and antenna-loss fault classes (SY-5) ---\n'
bash scripts/link_fault_gate.sh "$WORK/g37" || fail G37
bash scripts/link_fault_gate_selftest.sh "$WORK/g37" || fail G37
finish G37

# ---------------- G38: gateway placement shapes (MS-2)
# GEO-01 compares gateway shapes the scenario could not build: perimeter, cluster, star,
# L-shape and an optimiser's file. --gwLayout builds them, in every tier list, ids 101+.
# Short runs at boxHalf 5000 with 5 gateways, read by scripts/gw_layout_gate.py:
#   - --gwLayout=legacy is the default, byte for byte;
#   - every shape's coordinates recomputed from its documented construction, camp
#     included (base camp, or the west midpoint);
#   - the deployment the run used has those gateways in all three tier lists;
#   - a file layout is its rows, clamped to the box;
#   - four refusals.
#
# Negative fixtures in scripts/gw_layout_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G38 gateway placement shapes (MS-2) ---\n'
bash scripts/gw_layout_gate.sh "$WORK/g38" || fail G38
bash scripts/gw_layout_gate_selftest.sh "$WORK/g38" || fail G38
finish G38

# ---------------- G39: Cases A/B solved inside Step(), after the prediction (DC-11)
# --sparseCaseA solves before Step() against the previous posterior, and Step() then counts
# the cycle a no-fix (and, with relays, updates on the same pair again). --sparseInStep
# solves after Predict() on the final set. NC3's regime four ways and NC4's relayed one,
# read by scripts/sparse_in_step_gate.py:
#   - --sparseInStep=false is the default, byte for byte;
#   - the old path's solved cycles recorded as no-fixes, observed;
#   - every accepted solve is one position fuse and zero range updates, a fix in the trace,
#     no demotion; the propagated prior, not the previous posterior; the census complete;
#   - relays completing a two-range set take the full path, never both;
#   - --sparseCaseA with --sparseInStep is refused.
#
# Negative fixtures in scripts/sparse_in_step_gate_selftest.sh, built from these runs
# (rule 6). The policy cases are in ca_tbl_sparse_anchor_test (G-less unit suite).
printf '\n--- G39 Cases A/B solved inside Step(), after the prediction (DC-11) ---\n'
bash scripts/sparse_in_step_gate.sh "$WORK/g39" || fail G39
bash scripts/sparse_in_step_gate_selftest.sh "$WORK/g39" || fail G39
finish G39

# ---------------- G40: the prediction takes a synthetic-IMU control input (DC-1)
# Physical 2.2 predicts with an IMU control input; the filter was constant velocity.
# --imuPredict drives PredictWithControl from a dead-reckoning error model whose covariance
# is the ACCUMULATED error since the last fused fix. NC3's regime, read by
# scripts/imu_predict_gate.py:
#   - --imuPredict=false is the default, byte for byte;
#   - every coasted cycle moved by exactly u;
#   - every residual and covariance is the stated model, recomputed with the cited constants;
#   - Q_u is the PSD increment of the accumulation, which resets exactly on a fused fix;
#   - NEES across 20 seeds inside the chi^2 band (one sample per seed: the errors are per run).
#
# Negative fixtures in scripts/imu_predict_gate_selftest.sh, built from these runs (rule 6),
# among them an independent per-step Q, which fails NEES. The model's unit cases are in
# ca_tbl_imu_test.
printf '\n--- G40 the prediction takes a synthetic-IMU control input (DC-1) ---\n'
bash scripts/imu_predict_gate.sh "$WORK/g40" || fail G40
bash scripts/imu_predict_gate_selftest.sh "$WORK/g40" || fail G40
finish G40

# ---------------- G41: Case D, a single range on the propagated state (DC-4)
# Step() discarded a single range and coasted, counting it toward demotion; Physical 2.4D
# applies it as a constraint on the IMU-propagated state. --caseD does, behind a NIS guard,
# and the cycle reaches the escalation decision. NC3's regime with --sparseInStep and
# --imuPredict, read by scripts/case_d_gate.py:
#   - --caseD=false is the flag absent, byte for byte;
#   - a range went in exactly when its NIS passed 6.63;
#   - every Case D cycle is a one-range non-fix IMU-predicted cycle that did not demote;
#   - every one reached the decision (PCS computed), and some wanted to escalate;
#   - the stdout counts equal the CSV's; one refusal and one warning.
#
# Negative fixtures in scripts/case_d_gate_selftest.sh, built from these runs (rule 6). The
# policy cases are in ca_tbl_sparse_anchor_test.
printf '\n--- G41 Case D, a single range on the propagated state (DC-4) ---\n'
bash scripts/case_d_gate.sh "$WORK/g41" || fail G41
bash scripts/case_d_gate_selftest.sh "$WORK/g41" || fail G41
finish G41

# ---------------- G42: Case B resolves against the IMU-propagated prior (DC-2)
# Physical 2.4 resolves Case B's orientation by IMU heading; the code chose with the prior,
# a recorded deviation. Under --imuPredict --sparseInStep the prior IS the last posterior +
# the IMU's u_k. NC3's regime with four finders, seeds 1-4, with and without --imuPredict,
# read by scripts/case_b_imu_gate.py:
#   - every solve labelled by its prior (IMU / constant-velocity);
#   - Case B occurs under --imuPredict;
#   - every prior there is the previous posterior + u, recounted from the imu_ file.
#
# Negative fixtures in scripts/case_b_imu_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G42 Case B resolves against the IMU-propagated prior (DC-2) ---\n'
bash scripts/case_b_imu_gate.sh "$WORK/g42" || fail G42
bash scripts/case_b_imu_gate_selftest.sh "$WORK/g42" || fail G42
finish G42

# ---------------- G43: held entries carried forward before use (DC-13)
# A held finder entry was used where it was reported, inflated isotropically by walk
# speed x age. --entryPrediction (with --entryVelocity) uses x + v age and a source-type Q.
# NC9's regime with pseudo-anchor finders and digests, read by
# scripts/entry_prediction_gate.py:
#   - --entryPrediction=false is the flag absent, byte for byte;
#   - every substitution is x0 + v age with (0.5 age)^2, recomputed; moving entries move;
#   - every substituted entry is one node 1 held (DC-10's entries_ record);
#   - the stdout counts equal the CSV's; the flag without --entryVelocity is refused.
#
# Negative fixtures in scripts/entry_prediction_gate_selftest.sh, built from these runs.
printf '\n--- G43 held entries carried forward before use (DC-13) ---\n'
bash scripts/entry_prediction_gate.sh "$WORK/g43" || fail G43
bash scripts/entry_prediction_gate_selftest.sh "$WORK/g43" || fail G43
finish G43

# ---------------- G44: the IMU witness against the raw radio fix (DC-9)
# --imuCrossCheck compared the diverging SHS track with the EKF estimate. Under
# --imuWitnessSource=model the synthetic IMU track is compared with the cycle's raw fix --
# not the estimate, which already contains u -- and the counters are split. NC9's regime,
# read by scripts/imu_witness_gate.py:
#   - --imuWitnessSource=shs is the default witness, byte for byte;
#   - the track is the anchor + u; disagreement and tolerance recomputed; the raw fix is not
#     the estimate;
#   - evaluations, refusals and absorbed refusals consistent, none absorbed with the
#     evidence gate off; two refusals.
#
# Negative fixtures in scripts/imu_witness_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G44 the IMU witness against the raw radio fix (DC-9) ---\n'
bash scripts/imu_witness_gate.sh "$WORK/g44" || fail G44
bash scripts/imu_witness_gate_selftest.sh "$WORK/g44" || fail G44
finish G44

# ---------------- G45: real mesh links (NW-0)
# The PHY provider linked only field nodes to anchors, so the "mesh" was a star with anchor
# reflection. --meshLinks=all links every registered pair on a shared tier, makes DISCOVERY
# one-hop, polls anchors only, sleeps the node's idle radios and bills its relays. NC9's
# regime with peers, handshakes and per-tier identity queries, read by
# scripts/mesh_links_gate.py:
#   - --meshLinks=star is the default, byte for byte;
#   - finder->gateway, gateway->gateway and tag->tag frames delivered under all, none under
#     star; no DISCOVERY relayed under all;
#   - nothing received or relayed at the node on a sleeping radio; floods terminate;
#   - the node's relays billed; the poll guard fired; one refusal.
#
# Negative fixtures in scripts/mesh_links_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G45 real mesh links (NW-0) ---\n'
bash scripts/mesh_links_gate.sh "$WORK/g45" || fail G45
bash scripts/mesh_links_gate_selftest.sh "$WORK/g45" || fail G45
finish G45

# ---------------- G46: every field node is a measured subject (MS-1)
# Only tag 0 ran the pipeline; the other tags were protocol peers or fixed-tier relays, and
# PeerRequestDriver sent constant requests in their name. --measuredSubjects=N runs the core
# of tag 0's cycle for tags 2..N on their own accounts, each in its own slot of a round.
# NC9's regime with five tags, frames and arbitration, read by scripts/subjects_gate.py:
#   - --measuredSubjects=1 writes nothing new;
#   - subject_summary.csv has N x arms x seeds rows; subject 1's row is arm_summary's;
#   - steps, policy cycles and trace rows all equal duration/dt per subject (a relay that is
#     a subject is stepped once); fixes, cycles, escalations and RMSE recounted from traces;
#   - sect 6 per subject: an unused tier is charged for sleep only;
#   - subjects send their own requests; files digested; two refusals and a warning.
#
# Negative fixtures in scripts/subjects_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G46 every field node is a measured subject (MS-1) ---\n'
bash scripts/subjects_gate.sh "$WORK/g46" || fail G46
bash scripts/subjects_gate_selftest.sh "$WORK/g46" || fail G46
finish G46

# ---------------- G47: finders search for specific people (MS-1 finder tasking)
# Every finder steered on tag 0's entry, so with five subjects four were never searched
# for. --finderTasking=roundrobin tasks finder k to subject (k mod N) + 1 and spreads each
# group over its subject's ellipse. G43's converging-finder regime with five tags, read by
# scripts/finder_tasking_gate.py:
#   - --finderTasking=tag0 is the default, byte for byte;
#   - every finder-cycle names the right subject, and a converging finder's entry is that
#     subject's own published estimate (recomputed from its trace), never tag 0's;
#   - the goal is the entry plus the within-group spread; one row per finder-cycle;
#   - four refusals.
#
# Negative fixtures in scripts/finder_tasking_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G47 finders search for specific people (MS-1 finder tasking) ---\n'
bash scripts/finder_tasking_gate.sh "$WORK/g47" || fail G47
bash scripts/finder_tasking_gate_selftest.sh "$WORK/g47" || fail G47
finish G47

# ---------------- G48: the reverse path really exists (NW-8)
# The MAC was never told who transmitted a frame, so --reversePath cached a QUERY's
# originator and sent the RESPONSE one hop to it on hit and miss alike. --reversePathHop
# caches the previous hop, unicasts the RESPONSE hop by hop, floods on a miss and absorbs
# retransmitted copies. G32's query regime under --meshLinks=all with the event log, read
# by scripts/reverse_path_gate.py:
#   - every unicast hop goes to the sender's previous hop, recomputed from QUERY rows;
#   - forwarded, delivered and duplicates recounted per (query, answerer);
#   - paths of more than one hop exist; --reversePath alone is unchanged; two refusals.
#
# Negative fixtures in scripts/reverse_path_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G48 the reverse path really exists (NW-8) ---\n'
bash scripts/reverse_path_gate.sh "$WORK/g48" || fail G48
bash scripts/reverse_path_gate_selftest.sh "$WORK/g48" || fail G48
finish G48

# ---------------- G49: a UWB escalation commits only on a confirmed grant (NW-7)
# The tier changed inside Step() and nothing at the node read ESC_GRANT, so a node could run
# on UWB with no anchor's grant. --grantGatedUwb makes the escalation a pending request
# committed by a delivered GRANT(granted). G33's handshake regime with the event log, read
# by scripts/grant_gated_gate.py:
#   - every UWB entry of every subject has a confirm in the slice before it, matched to a
#     delivered GRANT frame; requests are exactly the trace's pending cycles;
#   - no healthy demotion or escalation while pending; counts recounted; static arms exempt;
#   - without the flag some UWB entries have no grant at all; a refusal.
#
# Negative fixtures in scripts/grant_gated_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G49 a UWB escalation commits only on a confirmed grant (NW-7) ---\n'
bash scripts/grant_gated_gate.sh "$WORK/g49" || fail G49
bash scripts/grant_gated_gate_selftest.sh "$WORK/g49" || fail G49
finish G49

# ---------------- G50: QUERY and RESPONSE carry what the spec says (NW-9)
# A QUERY carried one manifest, a RESPONSE no entry and no query id, nothing at the
# requester handled it, and an answering target still rebroadcast. --queryV2 answers every
# listed manifest with the entry and relays only what is left. G32's query regime under mesh
# links and the reverse path, five subjects, a five-manifest batch, read by
# scripts/query_v2_gate.py:
#   - without the flag nothing v2 is written or carried;
#   - every answer is the named subject's own published state, recomputed from its trace;
#   - answered + relayed manifests partition each received list; relay iff something left;
#   - the NW-9 line recounted; digests; three refusals.
#
# Negative fixtures in scripts/query_v2_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G50 QUERY and RESPONSE carry what the spec says (NW-9) ---\n'
bash scripts/query_v2_gate.sh "$WORK/g50" || fail G50
bash scripts/query_v2_gate_selftest.sh "$WORK/g50" || fail G50
finish G50

# ---------------- G51: query-triggered escalation and arbitration as specified (NW-4 + NW-5)
# No path led from a QUERY to escalation, and the C4 arbiter scored the battery spent, a
# constant T_query, forgot denials, let only gateways arbitrate and took a 2.4 GHz request
# as UWB reach. The five-subject regime with URGENT v2 queries, --grantGatedUwb and the
# event log, 1 200 s, read by scripts/arbitration_v2_gate.py:
#   - every score recomputed; every grant from the queue the argmax of its cycle;
#   - UWB-reach evidence behind every grant and queueing, none behind a refusal;
#   - grants from the queue, one committed at its subject; finders arbitrate;
#   - URGENT queries latched, hook escalations, requests carrying is_query_triggered;
#   - the NW-5 line recounted; the C4 control writes nothing; three refusals.
#
# Negative fixtures in scripts/arbitration_v2_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G51 query-triggered escalation and arbitration as specified (NW-4 + NW-5) ---\n'
bash scripts/arbitration_v2_gate.sh "$WORK/g51" || fail G51
bash scripts/arbitration_v2_gate_selftest.sh "$WORK/g51" || fail G51
finish G51

# ---------------- G52: anchors gossip their queue as well as their grants (NW-6)
# GRANT_GOSSIP carried grants only, so two arbiters could hold the same request in their
# queues and each grant it. --gossipQueue gossips the queue and extends the lower-id
# tie-break to it; finders gossip over the mesh. G51's regime at seed 1, 1 800 s, with grant
# gossip and mesh links, read by scripts/queue_gossip_gate.py:
#   - queue items gossiped and heard by arbiters, finders among the senders;
#   - every yield had a higher-id holder; a yielder never grants the request after;
#   - the NW-6 line recounted; grants-only gossip carries no queue; a refusal.
#
# Negative fixtures in scripts/queue_gossip_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G52 anchors gossip their queue as well as their grants (NW-6) ---\n'
bash scripts/queue_gossip_gate.sh "$WORK/g52" || fail G52
bash scripts/queue_gossip_gate_selftest.sh "$WORK/g52" || fail G52
finish G52

# ---------------- G53: hop provenance reaches the decision and the RESULT (NW-1, steps 1-2)
# The measured tag's Step() was passed a literal 0 and the RESULT said 0, though C5 knew each
# relayed range's depth. --hopProvenance passes the fused set's deepest leg and carries it
# as hop_count_at_fix (hopWeight stays 0). G17's relay-chain regime, static_2p4 and
# ca_tbl_full, read by scripts/hop_provenance_gate.py:
#   - every cycle's depth and relayed count recounted from relay_chain_*'s admitted tag legs;
#   - fused fixes carry depths 1 and 2; the NW-1 line equals the log; the control; a refusal.
#
# Negative fixtures in scripts/hop_provenance_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G53 hop provenance reaches the decision and the RESULT (NW-1) ---\n'
bash scripts/hop_provenance_gate.sh "$WORK/g53" || fail G53
bash scripts/hop_provenance_gate_selftest.sh "$WORK/g53" || fail G53
finish G53

# ---------------- G54: the base station's session is fed by delivered RESPONSEs (TA-1a)
# The session layer was opened by the measured tag for itself and fed its own EKF every
# cycle. --sessionOverMesh opens the base station's session (requester 0) over what its
# queries ask, fed only by RESPONSEs delivered to a gateway. G50's regime with
# --sessionLayer, read by scripts/session_gate.py:
#   - every result is a gateway delivery, once per answer, and every delivery is fed;
#   - each RESULT is the delivered entry; the state chain holds and leaves SEARCHING on
#     an answer; time-to-first-fix is the first delivered result; the control; refusals.
#
# Negative fixtures in scripts/session_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G54 the base station session is fed by delivered RESPONSEs (TA-1a) ---\n'
bash scripts/session_gate.sh "$WORK/g54" || fail G54
bash scripts/session_gate_selftest.sh "$WORK/g54" || fail G54
finish G54

# ---------------- G55: LOCATED_PRECISE on a confirmed UWB grant (TA-3), batch spread (TA-4)
# The session went precise on any result from a UWB fix. --sessionPreciseOnGrant makes it
# precise on a delivered entry naming a confirmed grant (NW-7), and the TA-1 line reports
# the batch's precision spread. G49's five-subject regime with v2 batch queries, mesh links
# and the base station's session, 1 200 s, read by scripts/precise_gate.py:
#   - every first-precise time is the first granted result, and PRECISE is entered on one;
#   - no target stamped precise by an ungranted UWB fix; the spread is max - min;
#   - without the flag the default rule holds; a refusal.
#
# Negative fixtures in scripts/precise_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G55 LOCATED_PRECISE on a confirmed UWB grant (TA-3, TA-4) ---\n'
bash scripts/precise_gate.sh "$WORK/g55" || fail G55
bash scripts/precise_gate_selftest.sh "$WORK/g55" || fail G55
finish G55

# ---------------- G56: every finder runs its own search (TA-1b)
# Only the base station searched. --finderSessions gives each finder its own session for its
# round-robin subject, fed only by the answers that reach it over the reverse path, and
# cancels it once the finder is within --finderCancelM of its target. G54's regime with
# finders tasked round-robin, plus G47's converging regime for the cancel, read by
# scripts/finder_session_gate.py:
#   - every finder result is an answer that reached that finder, once; every one is fed;
#   - finders search for their assigned subjects and leave SEARCHING on an answer;
#   - cancels within the distance close the session for good (and do occur at 30 m);
#   - lines recounted; the control; a refusal.
#
# Negative fixtures in scripts/finder_session_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G56 every finder runs its own search (TA-1b) ---\n'
bash scripts/finder_session_gate.sh "$WORK/g56" || fail G56
bash scripts/finder_session_gate_selftest.sh "$WORK/g56" || fail G56
finish G56

# ---------------- G57: the requester drives escalation (TA-2)
# Only a node's own PCS asked for UWB. --requesterEscalation makes the base station submit an
# escalation request for a target whose delivered answer has a low PCS, routed to the arbiter
# the target's own entry names as in UWB reach (never the nearest gateway), scored there with
# query_triggered set; the on-behalf GRANT is taken by the target as its escalation. G47's
# converging regime, read by scripts/reqesc_gate.py:
#   - every request follows the rule (a clean low-PCS answer, one per slot hold);
#   - every request sent went to the arbiter the target named, which had reach evidence;
#   - the arbiter scored each as logged; delivered grants accepted, then UWB the next cycle;
#   - the TA-2 line recounted; the control; a refusal.
#
# Negative fixtures in scripts/reqesc_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G57 the requester drives escalation (TA-2) ---\n'
bash scripts/reqesc_gate.sh "$WORK/g57" || fail G57
bash scripts/reqesc_gate_selftest.sh "$WORK/g57" || fail G57
finish G57

# ---------------- G58: per-hop variance and NLOS enter a relayed range sigma (NW-1 step 3)
# C5 composed sigma_eff from the engine's sigma and the relay's covariance alone.
# --hopVarianceTerms adds each leg's RSSI/SNR variability, converted to range by the tier's
# path-loss slope, and inflates a leg the tier's NLOS score calls NLOS. G53's relayed regime
# with the NLOS classifier, read by scripts/hop_variance_gate.py:
#   - every decision row carries the leg's readings and terms;
#   - the variability, its conversion, the NLOS verdict and sigma_leg are recomputed;
#   - sigma_eff is composed from sigma_leg within [0, trace P];
#   - both terms fire on admitted legs, at depths 1 and 2; lines recounted; the control;
#     two refusals.
#
# Negative fixtures in scripts/hop_variance_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G58 per-hop variance and NLOS enter a relayed range sigma (NW-1 step 3) ---\n'
bash scripts/hop_variance_gate.sh "$WORK/g58" || fail G58
bash scripts/hop_variance_gate_selftest.sh "$WORK/g58" || fail G58
finish G58

# ---------------- G59: every measured subject routes its own fixes, on its own tier (NW-2)
# --fixRouting routed the relays on --relayTier; the measured tag never routed, and subjects
# 2..N routed on --relayTier whatever tier they ranged on. --fixRoutingSubjects runs one
# router per tier and lets a node take part only on the tier it ranged on that cycle;
# --routeAlpha/Beta/Gamma/Slack weight the confidence cost. G18b's regime on the full arm
# with five subjects, read by scripts/subject_routing_gate.py:
#   - sect 6 per routed frame, against tag 0's trace and each subject's subjtrace;
#   - a hop to a next hop on another tier is refused, never sent;
#   - the measured tag and subjects deliver fixes, on two tiers;
#   - every routed row's cost recomputed from the run's weights (and beta 0 drops a term);
#   - router blocks recounted; the control (no subjects); three refusals, one of them the
#     old breach, --fixRouting with subjects and without the flag (TECH_DEBT N.12, refused
#     since 2026-10-03).
#
# Negative fixtures in scripts/subject_routing_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G59 every measured subject routes its own fixes on its own tier (NW-2) ---\n'
bash scripts/subject_routing_gate.sh "$WORK/g59" || fail G59
bash scripts/subject_routing_gate_selftest.sh "$WORK/g59" || fail G59
finish G59

# ---------------- G60: Case C's relative shape at the base station (DC-3)
# A 0-anchor cycle was counted as Case C and coasted. --caseCRelative lets such a subject
# range the peer subjects it hears; the ranges reach the base station in its answers, which
# reconstructs each connected set by classical MDS (a pair as its distance) and fuses
# nothing. G54's regime, read by scripts/case_c_gate.py:
#   - every peer range on a 0-anchor cycle of its node, on its tier, to another subject;
#   - delivery checked against the gateways' answers, inside the 10 s window;
#   - the base station's sets and shapes recomputed from the delivered ranges;
#   - nothing fused; pairs and shapes both assembled; the line recounted; the control;
#     a refusal.
#
# Negative fixtures in scripts/case_c_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G60 Case C relative shape at the base station (DC-3) ---\n'
bash scripts/case_c_gate.sh "$WORK/g60" || fail G60
bash scripts/case_c_gate_selftest.sh "$WORK/g60" || fail G60
finish G60

# ---------------- G24: the preset is conformant (P6, plan sect 3.2)
# --specProfile sets every spec flag the user did not give. G24 runs the preset, its
# endgame variant (finders converging in a 750 m box; the owner's decision after MS-3) and
# the last-gasp run, and the preset with one flag off per mechanism, read by
# scripts/spec_profile_gate.py:
#   - the preset reproduces its own settings given as flags, byte for byte;
#   - spec_profile.txt lists every setting, and an explicit flag is kept;
#   - every router runs the preset's routing weights, CAMP-SPEC's pick (0,0,1,1), and
#     the preset with routing turned off explicitly still runs;
#   - --hopVarianceTerms is not in the preset (owner decision, N.11);
#   - every mechanism has a non-zero population in its registered run (a zero is FAIL);
#   - with each mechanism's flag off its population is 0; the --topology=1 refusal, and
#     the preset with --fixRoutingSubjects=false refused (N.12).
# FAIL BY FINDING today: four mechanisms have no population under the preset (TECH_DEBT N.15).
#
# Negative fixtures in scripts/spec_profile_gate_selftest.sh, built from these runs (rule 6).
printf '\n--- G24 the preset is conformant (P6) ---\n'
bash scripts/spec_profile_gate.sh "$WORK/g24" || fail G24
bash scripts/spec_profile_gate_selftest.sh "$WORK/g24" || fail G24
finish G24

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
if [ "${GATE_ST[G24]:-}" = FAIL ]; then
    printf '\n  G24 is FAIL BY FINDING: mechanisms with no population under the preset\n'
    printf '  (TECH_DEBT N.15). It passes when they are exercised or the owner accepts them.\n'
fi
if [ "$SUITE_BAD" -eq 0 ]; then
    printf '\n=== ALL GATES PASS (seed=%s duration=%s) ===\n' "$SEED" "$DUR"
    exit 0
fi
printf '\n=== SUITE FAILED -- see summary above ===\n'
exit 1
