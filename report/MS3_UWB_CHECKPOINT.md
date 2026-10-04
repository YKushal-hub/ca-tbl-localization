# MS-3 — the P5 UWB-reach checkpoint: failed

**What was asked** (docs/SPEC_CONFORMANCE_PLAN.md, P5 and MS-3). Before P6 is built on the
preset, a run of the preset minus geometry must show `cycles_uwb > 0` **and UWB-delivered
fixes** (`fixTier = UWB`) with the evidence gate on (decision 7). Otherwise the owner is
consulted. Run on 2026-10-03 at `f062b5e` (P4 complete). These are probes, not a
registered campaign.

**The configuration** is the preset's settings as they stand without `--specProfile`,
which is not built yet. The flags are listed in §4. Geometry is left at the defaults, as
"minus geometry" asks. Two settings differ from the plan's table:
- `--sparseCaseA` is left out: it is FATAL together with `--sparseInStep`, which replaces
  it.
- `--identityQueryPeriodS=30` is added: `--queryV2` is FATAL without a query period.

`--radioBasis` does not exist yet (SY-2 is blocked on the LR1121 datasheet).

## 1. Result

Full arm, five measured subjects, 4 finders (`--finderMode=2`, round-robin tasking),
`--boxHalf=5000`, 1 800 s. A node-cycle is one cycle of tag 0 or of one subject (3 600 per
run).

| seed | UWB node-cycles | UWB-delivered fixes | tag 0 cycles with a UWB neighbour |
|---|---|---|---|
| 1 | 0 | 0 | 0 |
| 2 | 0 | 0 | 0 |
| 3 | 10 | 0 | 0 |
| 4 | 22 | 0 | 0 |
| 5 | 5 | 0 | 0 |

**The checkpoint fails.** No seed delivers a UWB fix.

## 2. Diagnosis (3 seeds each, same configuration with one change)

| variant | UWB node-cycles | UWB fixes | tag 0 UWB-neighbour cycles |
|---|---|---|---|
| A: `--escalationRequiresEvidence=false` (decision 7 reversed) | 0, 0, 5 | 0 | 0 |
| B: `--finderMode=1` (finders converge) | 0, 0, 10 | 0 | 0 |
| C: `--boxHalf=750` | 219, 291, 287 | 0 | 20, 9, 36 |

**The evidence gate is not the cause.** Variant A behaves like the preset. Only a small box
reaches the UWB tier, and even there no UWB fix forms. Tag 0's cycles in variant C, read
from its trace, give two structural reasons:

1. **A subject has at most one finder within UWB reach.** Round-robin tasking gives each
   subject one finder (subject 5 none, with 4 finders and 5 subjects), and gateways are
   far outside UWB reach. Over 2 160 cycles, tag 0 had one UWB neighbour on 49 and two on
   16. It never had the three a UWB fix needs, or the two Case A needs.
2. **Floor-first fix selection discards the UWB range.** When the active tier returns
   fewer than three ranges and the 868 MHz floor solves, the floor's fix is used. In the
   750 m box the floor reaches 9-13 gateways. The 1-2 UWB ranges are not fused at all:
   on every UWB cycle with a UWB neighbour, `nRangesEkf` equals `nAnchors`, all of them
   floor ranges. Case A and Case D on the active tier run only when the floor fails. That
   rule was chosen on measured evidence for 2.4 GHz Case A (the census comment in the
   scenario: noFix 1 -> 13, RMSE 16.9 -> 23.6 m when a two-anchor solve beat the floor).
   But it means a centimetre-level UWB range never improves a fix the floor can make.

Neither is a coding slip. Each is a consequence of a rule that was written for something
else.

## 3. The owner's decision (2026-10-03)

"Investigate, then proceed":
- the investigation is §2;
- decision 7 and the 5 km preset stay;
- MS-3 is recorded as failed with these numbers;
- CAMP-SPEC gets a registered small-scale "endgame" arm (converging finders in a small
  box), so the UWB tier is evaluated somewhere.

A remedy for reason 2, not built: fuse the active tier's 1-2 ranges together with the
floor's set, behind its own flag with a null control. That is an estimator change, and it
waits for the owner's word (TECH_DEBT N.14).

## 4. Commands

The preset flags, as run:

```
--tier1Tdoa=true --nlosClassifier=true --pcsNlosClassifier=true --nlosSource=propagation
--terrainPerTierFreq=true --imuPredict=true --sparseInStep=true --caseD=true --caseCRelative=true
--suspectRecovery=downweight --suspectForcesEscalation=false --integrityNlosFromScore=true
--imuCrossCheck=true --corroborationCheck=true --imuWitnessSource=model --entryVelocity=true
--entryPrediction=true --escalationRequiresEvidence=true --uwbEvidenceFromNeighbours=true
--uwbCoverageIncludesFinders=true --sendHandshakeFrames=true --handshakeFixes=true
--triggeredUpdates=true --mobilityFromRange=true --bidirectionalDigest=true --digestSummary=true
--tableFuseCI=true --meshLinks=all --relayedRanging=true --relayChain=true --maxRelayDepth=2
--hopVarianceTerms=true --hopProvenance=true --fixRouting=true --fixRoutingConfidence=true
--fixRoutingSubjects=true --queryV2=true --queryTierBinding=per-tier --queryProxy=true
--reversePath=true --reversePathHop=true --queryTriggeredEscalation=true --uwbArbitration=true
--arbitrationV2=true --anchorGrantGossipS=5 --gossipQueue=true --grantGatedUwb=true
--sessionLayer=true --sessionOverMesh=true --batchSearch=true --finderSessions=true
--identityQueryUrgent=true --identityQueryManifests=1,2,3,4,5 --requesterEscalation=true
--sessionPreciseOnGrant=true --vizEvents=true --edgeUplinkRanges=true --measuredSubjects=5
--nTags=5 --boxHalf=5000 --nFinders=4 --finderMode=2 --finderDispersal=1.0
--finderPseudoAnchor=true --finderDigestPeriodS=20 --finderTasking=roundrobin
--identityQueryPeriodS=30
```

Each run: `--arms=ca_tbl_full --seed=<s> --duration=1800 --trace=true <flags> [<variant>]`. The
counts are read from `trace_*` and `subjtrace_*`: `activeTier`, `fixTier` where `hadFix`
is 1, and `nbrUwb`. A 1 800 s preset run takes about 1 min of wall time.
