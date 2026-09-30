# Scenario flags — `ca_tbl_causal_sim`

**Snapshot of 2026-09-30.** It lists 142 flags. It was generated from the binary built from
`ff4f8bb`'s `src/` and `scratch/`, which later commits up to this file's own leave
unchanged. **One row has been regenerated since:** `--relaySlots` (row 126 of §3, and its §1
row), whose default moved from `false` to `auto` on 2026-09-30. A fresh `--PrintHelp`
differs from the `ff4f8bb` one in that row only, and in no name.

**Why this file exists.** No tracked document listed the scenario's flags.
`HANDOFF_2026_09_09.md` §8 asked for `--gwArcDeg`, `--gwArcCentreDeg`, `--trailDeviateSide`
and `--searchLaunchAtS` to get "a line in whichever document lists scenario flags", and
there was no such document. Those four are rows 84, 85, 86 and 113 of §3.

**It is a snapshot, not the ground truth.** `RULES.md` §8 puts the ground truth in the
`cmd.AddValue` block of `scratch/ca_tbl_causal_sim.cc` and in the `run_id.txt` sidecars.
This file goes stale the same way every prose document does. Regenerate §3; do not edit it
by hand.

**How it was generated.**

```bash
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --PrintHelp
```

§3 takes, in registration order, each option's name and the bracketed default at the end of
its line. The help text is left out on purpose. It is the argument for each default, it is
long, and it belongs in the source; read it with `--PrintHelp`.

Two checks found what §1 lists:

- The names registered at `084a7ff` against those registered here: 10 added, 0 removed.
- The frozen `084a7ff` binary's `--PrintHelp` (`~/catbl_ref`) against this one's, flag by
  flag: 1 default changed.

---

## 1. Added or changed 2026-09-27 – 2026-09-29

All of these are on `work/2026-09-27-spec-b1`. Every added flag is default off, and its
off state was null-controlled against the frozen `084a7ff` reference (`docs/TECH_DEBT.md`
L.1 status). The one exception since is `--relaySlots`, which since 2026-09-30 is on in
relayed runs and still off in every other run (its row). "FATAL" means the run is refused
with exit 1 rather than run with a precondition missing.

| flag | type | default | commit | what it does | preconditions and notes |
|---|---|---|---|---|---|
| `--anchorGrantGossipS` | double | `0` (off) | `17ab7dd` | Seconds between rounds in which each gateway broadcasts the UWB grants it holds to neighbouring gateways. Two overlapping grants for one node are resolved by the lower node id deferring (L.1.4) | > 0 needs `--sendHandshakeFrames=true`, or FATAL. A period of at least the 10 s arbiter slot hold draws a warning. The done-when evidence file `grant_verdicts_*.csv` is digested since `ff4f8bb` |
| `--relayChain` | bool | `false` | `b390ed6` | A relay that ranged fewer than three anchors this cycle ranges a relay that holds a fix, and inherits depth + 1 (L.1.2) | Needs `--relayedRanging=true` and `--nTags >= 3`, or FATAL. Warns if `--maxRelayDepth < 2`. Reaches depth 2 only with the relays' round, `--relaySlots`, which is on by default in relayed runs since 2026-09-30 (`docs/TECH_DEBT.md` M.1) |
| `--relaySlots` | string: `auto`, `true`, `false` | **`auto`**, was `false` (a bool) | `d24690c`; default moved 2026-09-30 | Gives the relays' ranging round slots of its own (50 ms each) before the cycle instant, so relay exchanges no longer collide at one instant. `auto` is on exactly when `--relayedRanging=true` and `--nTags >= 2`, and off in every other run | `=false` is the old default and reproduces the commit before the change byte-for-byte in relayed runs, and at plain defaults nothing moves (NC1 and NC2 identical to `db2ac2e`). `=true` without a relay is FATAL, and a round that does not fit inside `--dt` is FATAL under `auto` as under `true`. A bare `--relaySlots` means true. **Owner decision taken 2026-09-30** (M.1). Every `--relayedRanging` figure recorded before it without `=true` was measured under the collision |
| `--fixRouting` | bool | `false` | `5f3ad42` | Routes each relay's position fix to the gateway backbone hop by hop. It selects on ETX and scores the confidence metric beside it (L.1.1) | Needs `--relayedRanging=true` and `--nTags >= 3`, or FATAL. Warns on `--relayTier=0` |
| `--fixRoutingConfidence` | bool | `false` | `5f3ad42` | Makes the confidence metric the active selector instead of ETX | Needs `--fixRouting=true`, or FATAL. The weights α, β, γ and the slack are ours and uncalibrated |
| `--tier1Tdoa` | bool | `false` | `7377ed0` | Tier-1 TDoA from surveyed gateways' PPS-disciplined stamps of the node's broadcast. Four or more gateways are solved by Chan seeding Foy; exactly three by a closed form that keeps every root (L.2.1) | FATAL with `--tier1ChargeReplies=false` and `--gatewayEdgeServer=false` together. It changes the tier-1 observable, so making it the default is a planned campaign (`docs/TECH_DEBT.md` J.2) |
| `--tdoaPpsSigmaNs` | double | `20` | `7377ed0` | The 1σ GPS-PPS residual of a gateway's stamp, in ns. It is taken from the u-blox M8T data sheet: a cited product figure, not a measurement | Read only under `--tier1Tdoa`. Must be ≥ 0, or FATAL |
| `--nlosClassifier` | bool | `false` | `86e43f2` | Scores every ranged link with the spec's tier-specific NLOS score, against the channel model's own draw. It writes `nlos_links_*` and `nlos_summary_*` (L.2.2) | The UWB CIR confusion matrix cannot fill under the channel model (M.2) |
| `--pcsNlosClassifier` | bool | `false` | `86e43f2` | PCS term 3 reads the classifier's score instead of the `--nlosSource` tag | Needs `--nlosClassifier=true`, and is FATAL under `--pcsForm=v2`. CAL-01 ran with it on |
| `--nlosScoreThreshold` | double | `0.5` | `86e43f2` | The score at which the confusion matrix calls a link NLOS. PCS reads the continuous score, never the verdict | Ours, uncited. Must lie in (0, 1), or FATAL |
| `--gatewayEdgeServer` | bool | **`true`**, was `false` | `db2ac2e` | Runs the estimator on the gateways. At defaults only the energy columns move (L.2.3) | `=false` reproduces every result in `results/`. The downlink-tier question is a pending DECISION (M.10) |

## 2. Help text corrected, default unchanged

- `--tier1ChargeReplies` (default `true`): it is read only with `--gatewayEdgeServer=false`.
  Its "FALSE by default" had been wrong since `4779bf7`, and `db2ac2e` corrected it
  (`docs/TECH_DEBT.md` M.5).
- `--queryStalenessBoundS` (default `120`): it now carries L.2.5's measurement (`f017da4`).

## 3. Every flag, with its default

Registration order, which is also `--PrintHelp`'s order. A default is shown as the binary
prints it.

| # | flag | default |
|---|---|---|
| 1 | `--env` | `1` |
| 2 | `--envFacility` | `255` |
| 3 | `--duration` | `3600` |
| 4 | `--dt` | `2.5` |
| 5 | `--seed` | `7` |
| 6 | `--seedStart` | `7` |
| 7 | `--seedCount` | `1` |
| 8 | `--outdir` | `results/causal` |
| 9 | `--trace` | `true` |
| 10 | `--probe` | `false` |
| 11 | `--traceDiagnostic` | `false` |
| 12 | `--boxHalf` | `1000` |
| 13 | `--baseCampFrac` | `0` |
| 14 | `--shsSnapPosition` | `false` |
| 15 | `--shsStepBounds` | `false` |
| 16 | `--batteryDepleteJ` | `0` |
| 17 | `--mobilityFromRange` | `false` |
| 18 | `--lifecycleTracking` | `false` |
| 19 | `--batchSearch` | `false` |
| 20 | `--sessionLayer` | `false` |
| 21 | `--sparseCaseA` | `false` |
| 22 | `--sparseCaseAMargin` | `1` |
| 23 | `--injectSilentId` | `0` |
| 24 | `--injectSilentStartS` | `0` |
| 25 | `--injectSilentEndS` | `0` |
| 26 | `--digestSummary` | `false` |
| 27 | `--tableFuseCI` | `false` |
| 28 | `--queryProxy` | `false` |
| 29 | `--queryStalenessBoundS` | `120` |
| 30 | `--reversePath` | `false` |
| 31 | `--imuIdeal` | `false` |
| 32 | `--imuFusion` | `false` |
| 33 | `--shsTrustedFixK` | `0` |
| 34 | `--maxTurnRate` | `0` |
| 35 | `--subjectKeepOutM` | `0` |
| 36 | `--uwbTriad` | `10` |
| 37 | `--t2Triad` | `150` |
| 38 | `--uwbAnchors` | `5` |
| 39 | `--t2Anchors` | `3` |
| 40 | `--speed` | `1.2` |
| 41 | `--dwell` | `90` |
| 42 | `--vegWide` | `3` |
| 43 | `--vegFacility` | `255` |
| 44 | `--beacon868` | `10` |
| 45 | `--beacon2p4` | `2` |
| 46 | `--beaconUwb` | `2` |
| 47 | `--nbrTtlMult` | `3` |
| 48 | `--fastDemote` | `off` |
| 49 | `--suspectRecovery` | `none` |
| 50 | `--suspectInflateFactor` | `4` |
| 51 | `--suspectRecoveryN` | `5` |
| 52 | `--escalationRequiresEvidence` | `true` |
| 53 | `--escalationRequiresEvidenceT2` | `false` |
| 54 | `--t2PredictRadius` | `156` |
| 55 | `--t2PredictMinAnchors` | `3` |
| 56 | `--uwbEvidenceFromNeighbours` | `false` |
| 57 | `--mahalUseR` | `false` |
| 58 | `--gwGrid` | `3` |
| 59 | `--multiStepDemote` | `off` |
| 60 | `--demoteStalenessS` | `60` |
| 61 | `--searchDuty` | `1` |
| 62 | `--arms` | `all` |
| 63 | `--injectRangeBias` | `0` |
| 64 | `--injectFaultAnchors` | `0` |
| 65 | `--injectBiasFraction` | `0` |
| 66 | `--floorOnDemand` | `false` |
| 67 | `--floorSkipK` | `1` |
| 68 | `--floorHeartbeatN` | `0` |
| 69 | `--fixedVarThreshold` | `-1` |
| 70 | `--pcsSigmaTargetM` | `-1` |
| 71 | `--deferEscalationOnDutyLimit` | `true` |
| 72 | `--arbitrateDutyBudget` | `true` |
| 73 | `--pcsForm` | `legacy` |
| 74 | `--pcsW1` | `-1` |
| 75 | `--pcsW2` | `-1` |
| 76 | `--pcsW3` | `-1` |
| 77 | `--pcsGateFloor` | `-1` |
| 78 | `--pcsHysteresisBand` | `-1` |
| 79 | `--nDeescalate` | `0` |
| 80 | `--sendHandshakeFrames` | `false` |
| 81 | `--terrainPerTierFreq` | `false` |
| 82 | `--intraCycleHandshakeDefer` | `false` |
| 83 | `--gwRidge` | `false` |
| 84 | `--gwArcDeg` | `360` |
| 85 | `--gwArcCentreDeg` | `90` |
| 86 | `--trailDeviateSide` | `-1` |
| 87 | `--gwRidgeY` | `0.55` |
| 88 | `--trailDeviateAtS` | `0` |
| 89 | `--trailStopAfterS` | `0` |
| 90 | `--vizTablePeriodS` | `0` |
| 91 | `--vizEvents` | `false` |
| 92 | `--uwbCoverageIncludesFinders` | `false` |
| 93 | `--finderDispersal` | `0` |
| 94 | `--processNoiseStdPos` | `1` |
| 95 | `--processNoiseStdVel` | `0.5` |
| 96 | `--gatewayEdgeServer` | `true` |
| 97 | `--tier1ChargeReplies` | `true` |
| 98 | `--bidirectionalDigest` | `false` |
| 99 | `--finderPosSigma` | `0` |
| 100 | `--finderPseudoAnchor` | `false` |
| 101 | `--hopProvenanceForm` | `0` |
| 102 | `--hopPenalty` | `0` |
| 103 | `--imuCrossCheck` | `false` |
| 104 | `--imuDriftMps` | `0.15` |
| 105 | `--imuConsistencyK` | `3` |
| 106 | `--imuSigmaFloorM` | `5` |
| 107 | `--corroborationCheck` | `false` |
| 108 | `--corroborationK` | `3` |
| 109 | `--corroborationRangeSigmaM` | `30` |
| 110 | `--finderDigestPeriodS` | `30` |
| 111 | `--finderSpeed` | `-1` |
| 112 | `--searchLaunchOnLoss` | `0` |
| 113 | `--searchLaunchAtS` | `0` |
| 114 | `--searchLaunchSigmaM` | `0` |
| 115 | `--finderStaleSteerBound` | `0` |
| 116 | `--finderMode` | `1` |
| 117 | `--nFinders` | `0` |
| 118 | `--nGateways` | `0` |
| 119 | `--gwRingFrac` | `0.6` |
| 120 | `--topology` | `0` |
| 121 | `--sitingMode` | `0` |
| 122 | `--relayedRanging` | `false` |
| 123 | `--maxRelayDepth` | `1` |
| 124 | `--relayVarBudget` | `0` |
| 125 | `--relayChain` | `false` |
| 126 | `--relaySlots` | `auto` |
| 127 | `--fixRouting` | `false` |
| 128 | `--fixRoutingConfidence` | `false` |
| 129 | `--tier1Tdoa` | `false` |
| 130 | `--tdoaPpsSigmaNs` | `20` |
| 131 | `--nlosClassifier` | `false` |
| 132 | `--pcsNlosClassifier` | `false` |
| 133 | `--nlosScoreThreshold` | `0.5` |
| 134 | `--relayTier` | `1` |
| 135 | `--nTags` | `1` |
| 136 | `--identityQueryPeriodS` | `0` |
| 137 | `--identityQueryTarget` | `2` |
| 138 | `--uwbArbitration` | `false` |
| 139 | `--arbBatteryCapJ` | `1000` |
| 140 | `--anchorGrantGossipS` | `0` |
| 141 | `--suspectForcesEscalation` | `true` |
| 142 | `--nlosSource` | `legacy` |
