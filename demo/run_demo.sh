#!/bin/bash
#
# The exact invocation that produced demo/run.json. Run from the repository root.
#
# WHY EACH FLAG THAT IS NOT OBVIOUS -- most of these were found the hard way, and a
# reader changing one should know what it is holding up.
#
#   --arms=ca_tbl_full      Runs ONE arm instead of six: ~6x faster, and byte-identical to
#                           that arm's row in a full run because RunArm() resets the RNG
#                           stream index per arm. A demo needs one arm.
#
#   --sendHandshakeFrames   REQUIRED, and it defaults to FALSE. The finder digest is gated
#                           on `finderDigestPeriodS > 0 && sendHandshakeFrames`. Without
#                           this no searcher ever polls for entries, 0/4 hold the casualty,
#                           and finderMode=2 never finds an entry that beats its own sweep
#                           -- so the party spirals outward forever and leaves the anchor
#                           footprint. This single omission is what made the demo look
#                           broken for an entire session.
#
#   --finderDispersal=1.0   Without it all four searchers occupy one point and move as a
#                           single node. They look like one triangle on the map.
#
#   --finderMode=2          Sweep from the last known position, then converge once a held
#                           entry is more informative than the sweep. Mode 1 converges
#                           directly and, with no entry, deliberately HOLDS -- a good
#                           diagnostic, not a search.
#
#   --gwArcDeg=140          Arc containment. See README section 3 for why neither a grid
#   --gwArcCentreDeg=180    nor a ridge is a fair deployment, and why the arc is centred
#                           on the trailhead the subject walks from.
#
#   --imuIdeal              A PERFECT dead reckoner, and a CEILING rather than a result.
#                           Never --imuCrossCheck for a demo: anomaly A.3, the IMU witness
#                           refuses 440 escalations in the full-stack configuration.
#
#   --searchLaunchSigmaM    Deploy when the network can still HEAR the subject but no
#   --searchLaunchOnLoss    longer PLACE them, sustained for 60 s. Read the caveat in
#                           README section 4 before quoting this as an alarm: on this
#                           deployment the bound sits inside the noise band.
#
# --outdir MUST ALREADY EXIST or the simulator exits 1 on run_id.txt.
set -eu

OUT="${1:-demo/out}"
mkdir -p "$OUT"

./build/scratch/ns3.47-ca_tbl_causal_sim-optimized \
    --arms=ca_tbl_full --seed=7 --duration=1800 --trace=true \
    --nFinders=4 --finderPseudoAnchor=true --finderMode=2 \
    --finderStaleSteerBound=180 --finderDispersal=1.0 \
    --sendHandshakeFrames=true --finderDigestPeriodS=20 \
    --searchLaunchSigmaM=100 --searchLaunchOnLoss=60 \
    --imuIdeal=true --imuFusion=true \
    --vizEvents=true --vizTablePeriodS=25 \
    --boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180 \
    --gwRingFrac=0.75 --gwRidgeY=0.10 \
    --trailDeviateAtS=150 --trailStopAfterS=420 --trailDeviateSide=-1 \
    --outdir="$OUT"

echo
echo "Run complete. Expected markers:"
grep -E "deployed at|TABLE: [0-9]+/" "$OUT/run.log" 2>/dev/null || true
grep -E "^  ca_tbl_full" "$OUT/run.log" 2>/dev/null || true
