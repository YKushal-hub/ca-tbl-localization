#!/usr/bin/env bash
# Run every campaign registered after the IMU primary, in one queue, at ONE commit.
#
# Why one queue: each invocation records `git rev-parse HEAD` at start-up. Committing
# between campaigns while any of them runs gives a campaign mixed source SHAs -- the
# c9_escalate incident of 2026-09-05. So every registration is committed first, this
# queue runs them all, and nothing is committed until it ends.
#
# Order is cheapest-first so partial progress is maximally useful if the queue is
# interrupted; each runner refuses existing directories, so a re-launch after an
# interruption must first move a partial directory aside and record that it did.
#
#   1. IMU bias sensitivity       report/PREREG_IMU_PRIOR.md §8      20 invocations, 2 arms
#   2. A3 intra-cycle handshake   report/PREREG_A3_INTRA_CYCLE_HANDSHAKE.md   40, 6 arms
#   3. A1 dwell                   report/PREREG_A1_DWELL.md          100, 4 arms
#   4. E3 CAL-05 with the IMU     report/PREREG_E3_IMU_INTEGRITY.md  400, 2 arms
#   5. reconstruction of cited evidence (not a registration -- a provenance repair)
set -u
set -o pipefail
cd "$(dirname "$0")/../.." || exit 2
HEAD0=$(git rev-parse --short HEAD)
dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
[ "$dirty" -eq 0 ] || { echo "REFUSING: tree dirty under src/ scratch/"; exit 2; }
echo "queue at $HEAD0, $(date -Iseconds)"
for step in imu_prior_sensitivity.sh run_a3_handshake.sh run_a1_dwell.sh \
            run_e3_imu_integrity.sh regen_cited_evidence.sh; do
    now=$(git rev-parse --short HEAD)
    if [ "$now" != "$HEAD0" ]; then
        echo "ABORT: HEAD moved from $HEAD0 to $now mid-queue -- later cells would carry another SHA"
        exit 3
    fi
    echo "=== $step  $(date -Iseconds)"
    bash "scripts/sweeps/$step" 2>&1 | grep -E '^(FAIL|REFUSING|no binary|[0-9]+ invocations|done)' || true
done
echo "queue done $(date -Iseconds)"
