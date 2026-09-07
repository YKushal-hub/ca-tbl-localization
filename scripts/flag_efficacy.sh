#!/usr/bin/env bash
# Does every flag actually change the output?
#
# This is the generalisation of edf73a5 and c236834: a mechanism that cannot change its
# own output is not enabled, whatever its flag says. Both defects were invisible to the
# gate suite and to reading the code -- one because an accumulator was cleared between
# its two contributors, the other because the gate interrogated a radio that is asleep.
# Both were found by asking this one question.
#
# Run each boolean BOTH ways in one configuration and compare arm_summary.csv byte for
# byte. Identical output means "no effect IN THIS CONFIGURATION", which is a question to
# answer and NOT a verdict: a flag may need a precondition this configuration does not
# meet. Answering it is the point. As of 2026-09-08 the answers were:
#
#   9 flags        differ immediately
#   4 flags        need --sendHandshakeFrames=true (bidirectionalDigest,
#                  finderPseudoAnchor, corroborationCheck, and the digest chain)
#   terrainPerTier need a scale where terrain bites: identical at boxHalf=1000,
#   Freq           differs at boxHalf=5000
#   mahalUseR,     inert because suspects == 0 -- the Mahalanobis integrity check has an
#   suspectForces  EMPTY ELIGIBLE POPULATION under the corrected topology, which the
#   Escalation     verifier also reports as "all-LOS on 0/1431 fix cycles"
#   uwbEvidence    WAS GENUINELY BROKEN. Fixed in c236834.
#   FromNeighbours
#
# Not a gate: it costs 32 runs. Run it after adding a flag, and before believing one.
set -u
REPO=$HOME/projects/ca-tbl-localization
B=$REPO/build/scratch/ns3.47-ca_tbl_causal_sim-optimized
OUT=$HOME/catbl_work/flagsweep
BASE="--seed=7 --duration=300 --boxHalf=1000 --nFinders=4 --finderMode=1 --nTags=2 --arms=ca_tbl_full"
FLAGS="escalationRequiresEvidence uwbEvidenceFromNeighbours mahalUseR floorOnDemand
deferEscalationOnDutyLimit arbitrateDutyBudget sendHandshakeFrames terrainPerTierFreq
intraCycleHandshakeDefer bidirectionalDigest finderPseudoAnchor imuCrossCheck
corroborationCheck relayedRanging uwbArbitration suspectForcesEscalation"
job(){ f=$1; v=$2
  cd "$HOME/projects/ca-tbl-localization" || exit 1
  d=$OUT/${f}_${v}; mkdir -p "$d"
  $B $BASE --${f}=${v} --outdir="$d" > "$d/run.log" 2>&1
  echo "$f=$v exit=$?"
}
export -f job; export B BASE OUT
for f in $FLAGS; do for v in true false; do echo "$f $v"; done; done \
  | xargs -P 3 -n 2 bash -c 'job "$0" "$1"' > $OUT.log 2>&1
echo FLAGSWEEP_DONE
