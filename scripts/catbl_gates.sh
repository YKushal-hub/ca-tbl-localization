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
rm -rf results/det_a results/det_b
mkdir -p results/det_a results/det_b
./ns3 run "ca_tbl_causal_sim --seed=$SEED --duration=$DUR --outdir=results/det_a --trace=false" 2>&1 | tail -3 || fail G2
./ns3 run "ca_tbl_causal_sim --seed=$SEED --duration=$DUR --outdir=results/det_b --trace=false" 2>&1 | tail -3 || fail G2
for f in results/det_a/arm_summary.csv results/det_b/arm_summary.csv; do
    [ -s "$f" ] || { printf 'missing or empty: %s\n' "$f"; fail G2; }
done
diff results/det_a/arm_summary.csv results/det_b/arm_summary.csv || fail G2
pass G2

# ------------------------------------- G3: traced run + null ablation
# This run is ALSO the traced pass that G4 reads. Running it here, immediately
# before the verifier, is what stops the verifier reading a stale trace.
printf '\n--- G3 traced run + null ablation ---\n'
mkdir -p results/causal
# Clear first, so a failed run cannot be masked by stale output from an earlier
# one. The [ -s ] checks below only detect absent or empty files; a leftover
# non-empty trace would sail through them and through the verifier's freshness
# check too, since a stale summary alongside stale traces is self-consistent.
rm -f results/causal/arm_summary.csv results/causal/trace_*.csv
./ns3 run "ca_tbl_causal_sim --seed=$SEED --duration=$DUR --outdir=results/causal" 2>&1 | tail -5 || fail G3

FULL=results/causal/trace_ca_tbl_full_seed$SEED.csv
NOINT=results/causal/trace_ca_tbl_no_integrity_seed$SEED.csv
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
python3 scratch/python_handoff/verify_causal_sim.py results/causal || fail G4
pass G4

# --------------------------------------------------- G5: verifier self-test
# G4 asks whether the verifier passes. G5 asks whether it FAILS when it should --
# which nothing did until the audit, and which is why two holes (a missing arm
# trace, a stale summary) survived in it undetected. Runs in seconds against the
# directory G3 just produced; no simulation.
printf '\n--- G5 verifier self-test ---\n'
bash scripts/catbl_verifier_selftest.sh results/causal || fail G5
pass G5

printf '\n=== ALL GATES PASS (seed=%s duration=%s) ===\n' "$SEED" "$DUR"
