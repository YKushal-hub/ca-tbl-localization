# Scenario flags — `ca_tbl_causal_sim`

**Snapshot of 2026-09-30.** It lists 149 flags. It was generated from the binary built from
`ff4f8bb`'s `src/` and `scratch/`, which later commits up to this file's own leave
unchanged. **Rows have changed since, all on 2026-09-30, and §3 was regenerated for each
change:**

- `--relaySlots` (row 132 of §3, and its §1 row): its default moved from `false` to
  `auto`.
- `--edgeDownlinkTier` (row 102 of §3, and its §1 row): added with its default, `fix`
  (M.10). Every row after it moved down one.
- `--uwbNlosModel` (row 140 of §3, and its §1 row): added with its default, `legacy`
  (M.2, `report/PREREG_UWB_NLOS.md`). Every row after it moved down one.
- `--pcsThrUp868`, `--pcsThrUp2p4`, `--pcsThrDown2p4`, `--pcsThrUpUwb`, `--pcsThrDownUwb`
  (rows 79–83 of §3, and their §1 row): added with the default `nan`, "not given"
  (`docs/SPEC_CONFORMANCE_PLAN.md` DC-6). Every row after them moved down five.

A fresh `--PrintHelp` differs from the `ff4f8bb` one in those rows only, and adds the
seven names. Two flags changed meaning without changing their printed default, and §1
says how: `--pcsForm` gained the value `calibrated`, and `--pcsW1..3` take 0.

**Why this file exists.** No tracked document listed the scenario's flags.
`HANDOFF_2026_09_09.md` §8 asked for `--gwArcDeg`, `--gwArcCentreDeg`, `--trailDeviateSide`
and `--searchLaunchAtS` to get "a line in whichever document lists scenario flags", and
there was no such document. Those four are rows 89, 90, 91 and 119 of §3.

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

## 1. Added or changed 2026-09-27 – 2026-09-30

All of these are on `work/2026-09-27-spec-b1`. Every added flag is default off, and its
off state was null-controlled against the frozen `084a7ff` reference (`docs/TECH_DEBT.md`
L.1 status). There are two exceptions since, both from 2026-09-30:

- `--relaySlots` is on in relayed runs, and still off in every other run.
- `--edgeDownlinkTier` was added with its new bill as the default. Its old value, `active`,
  reproduces the commit before it.

Each has its own row. "FATAL" means the run is refused with exit 1 rather than run with a
precondition missing.

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
| `--gatewayEdgeServer` | bool | **`true`**, was `false` | `db2ac2e` | Runs the estimator on the gateways. At defaults only the energy columns move (L.2.3) | `=false` reproduces every result in `results/` except C6's edge cell, which ran with it `true`. Which radio the downlink is billed on is `--edgeDownlinkTier` (M.10, decided 2026-09-30) |
| `--edgeDownlinkTier` | string: `fix`, `active` | **`fix`**; the old bill is `active` | added 2026-09-30, with this default | Picks which radio the gateways' downlink is billed on. `fix` bills the radio whose measurements produced the fix, so a hybrid arm's floor-solved fix is an 868 MHz downlink (0.9042 mJ) instead of one on the active tier (0.2005 mJ on 2.4 GHz, 0.3224 mJ on UWB) (M.10) | Read only with `--gatewayEdgeServer=true`. `=active` reproduces the commit before the change byte-for-byte, and C6's `results/regen27_edge/edge` (seed 7 checked). At defaults only energy moves, on hybrid arms' floor-solved fix cycles. Any other value is FATAL. **Owner decision taken 2026-09-30** (M.10) |
| `--uwbNlosModel` | string: `legacy`, `ieee802154a` | `legacy` | added 2026-09-30 | The UWB engine's LOS/NLOS state model. `ieee802154a` is `report/PREREG_UWB_NLOS.md` sect 2: the state from the 3GPP TR 38.901 InF-SL law (P_NLOS = 1 - exp(-d_2D / 44.81 m)) and, on an NLOS draw, the IEEE 802.15.4a NLOS-minus-LOS path loss (CM5/CM6 outdoor, CM3/CM4 office), deterministic in distance (M.2) | Default `legacy`, byte-identical to the build before the change in 7 configurations. It stays the default whatever the registered runs show. Under `ieee802154a` every arm moves, static_868 included, through the shared MAC random stream, so only populations compare across the two settings. Any other value is FATAL |
| `--pcsThrUp868`, `--pcsThrUp2p4`, `--pcsThrUpUwb`, `--pcsThrDown2p4`, `--pcsThrDownUwb` | double | `nan`, not given | added 2026-09-30 | Explicit per-tier PCS thresholds: escalate below the up one, count a cycle healthy above the down one. Applied after the form's own and after `--pcsHysteresisBand`, so an explicit value wins (DC-6) | Not given keeps the form's own, so the default is byte-identical to the build before (NC1–NC9). A resolved down below its up is FATAL whichever was given; the resolved values are printed as a `NOTE` on stderr. There is no `--pcsThrDown868`: nothing is below 868 MHz, so that threshold decides nothing. In no current regime does a UWB cycle deliver a UWB fix, so the UWB pair's effect is unobserved (gate G28) |
| `--pcsForm` | string | `legacy` | value `calibrated` added 2026-09-30 | `calibrated` is the spec's PCS: the weighted sum with `--pcsW1..3`, the three `--pcsThrUp*` and the two `--pcsThrDown*` all given explicitly, the values a leave-one-out calibration produced (DC-5/6, CAL-02) | FATAL unless all eight are given. At weights (1, 1, 1) and the legacy thresholds it reproduces `legacy` byte-for-byte, trace included (G28) |
| `--pcsW1`, `--pcsW2`, `--pcsW3` | double | `-1`, not given | changed 2026-09-30 | Weights of the `weighted` and `calibrated` forms | Any value ≥ 0 is taken, 0 included. Until 2026-09-30 only a value > 0 was, so a requested 0, which is CAL-01's answer (3, 0, 0), silently became the hand-set default. No tracked `run_id.txt` passes `--pcsW1..3` at all |

## 2. Help text corrected, default unchanged

- `--tier1ChargeReplies` (default `true`): it is read only with `--gatewayEdgeServer=false`.
  Its "FALSE by default" had been wrong since `4779bf7`, and `db2ac2e` corrected it
  (`docs/TECH_DEBT.md` M.5).
- `--queryStalenessBoundS` (default `120`): it now carries L.2.5's measurement (`f017da4`).
- `af492ca`, 2026-09-30: `--pcsForm` listed only `legacy|v2`, though `weighted` and `gated`
  exist. `--pcsHysteresisBand` claimed a 0.20 gap on every tier; UWB's is 0.25.
  `--imuCrossCheck` called itself a displacement check; it compares absolute positions.
  `--sparseCaseAMargin` is in Mahalanobis units. `--nTags` said peers have no policy or
  energy model, which is false under `--relayedRanging`. `--uwbArbitration` without
  `--sendHandshakeFrames` is now FATAL, and `--imuFusion` without its stated requirements
  draws a warning.
- 2026-09-30, DC-6: `--pcsHysteresisBand` said the down thresholds had no flag; they have
  one per tier now, and it wins over the band.

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
| 79 | `--pcsThrUp868` | `nan` |
| 80 | `--pcsThrUp2p4` | `nan` |
| 81 | `--pcsThrDown2p4` | `nan` |
| 82 | `--pcsThrUpUwb` | `nan` |
| 83 | `--pcsThrDownUwb` | `nan` |
| 84 | `--nDeescalate` | `0` |
| 85 | `--sendHandshakeFrames` | `false` |
| 86 | `--terrainPerTierFreq` | `false` |
| 87 | `--intraCycleHandshakeDefer` | `false` |
| 88 | `--gwRidge` | `false` |
| 89 | `--gwArcDeg` | `360` |
| 90 | `--gwArcCentreDeg` | `90` |
| 91 | `--trailDeviateSide` | `-1` |
| 92 | `--gwRidgeY` | `0.55` |
| 93 | `--trailDeviateAtS` | `0` |
| 94 | `--trailStopAfterS` | `0` |
| 95 | `--vizTablePeriodS` | `0` |
| 96 | `--vizEvents` | `false` |
| 97 | `--uwbCoverageIncludesFinders` | `false` |
| 98 | `--finderDispersal` | `0` |
| 99 | `--processNoiseStdPos` | `1` |
| 100 | `--processNoiseStdVel` | `0.5` |
| 101 | `--gatewayEdgeServer` | `true` |
| 102 | `--edgeDownlinkTier` | `fix` |
| 103 | `--tier1ChargeReplies` | `true` |
| 104 | `--bidirectionalDigest` | `false` |
| 105 | `--finderPosSigma` | `0` |
| 106 | `--finderPseudoAnchor` | `false` |
| 107 | `--hopProvenanceForm` | `0` |
| 108 | `--hopPenalty` | `0` |
| 109 | `--imuCrossCheck` | `false` |
| 110 | `--imuDriftMps` | `0.15` |
| 111 | `--imuConsistencyK` | `3` |
| 112 | `--imuSigmaFloorM` | `5` |
| 113 | `--corroborationCheck` | `false` |
| 114 | `--corroborationK` | `3` |
| 115 | `--corroborationRangeSigmaM` | `30` |
| 116 | `--finderDigestPeriodS` | `30` |
| 117 | `--finderSpeed` | `-1` |
| 118 | `--searchLaunchOnLoss` | `0` |
| 119 | `--searchLaunchAtS` | `0` |
| 120 | `--searchLaunchSigmaM` | `0` |
| 121 | `--finderStaleSteerBound` | `0` |
| 122 | `--finderMode` | `1` |
| 123 | `--nFinders` | `0` |
| 124 | `--nGateways` | `0` |
| 125 | `--gwRingFrac` | `0.6` |
| 126 | `--topology` | `0` |
| 127 | `--sitingMode` | `0` |
| 128 | `--relayedRanging` | `false` |
| 129 | `--maxRelayDepth` | `1` |
| 130 | `--relayVarBudget` | `0` |
| 131 | `--relayChain` | `false` |
| 132 | `--relaySlots` | `auto` |
| 133 | `--fixRouting` | `false` |
| 134 | `--fixRoutingConfidence` | `false` |
| 135 | `--tier1Tdoa` | `false` |
| 136 | `--tdoaPpsSigmaNs` | `20` |
| 137 | `--nlosClassifier` | `false` |
| 138 | `--pcsNlosClassifier` | `false` |
| 139 | `--nlosScoreThreshold` | `0.5` |
| 140 | `--uwbNlosModel` | `legacy` |
| 141 | `--relayTier` | `1` |
| 142 | `--nTags` | `1` |
| 143 | `--identityQueryPeriodS` | `0` |
| 144 | `--identityQueryTarget` | `2` |
| 145 | `--uwbArbitration` | `false` |
| 146 | `--arbBatteryCapJ` | `1000` |
| 147 | `--anchorGrantGossipS` | `0` |
| 148 | `--suspectForcesEscalation` | `true` |
| 149 | `--nlosSource` | `legacy` |
