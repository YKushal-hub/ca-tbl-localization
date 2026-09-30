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
