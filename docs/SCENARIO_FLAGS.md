# Scenario flags — `ca_tbl_causal_sim`

**Snapshot of 2026-09-30.** It lists 291 flags. It was generated from the binary built from
`ff4f8bb`'s `src/` and `scratch/`, which later commits up to this file's own leave
unchanged. **Rows have changed since, on 2026-09-30, 2026-10-01 and 2026-10-02, and §3 was regenerated
for each change.** Row numbers below are as of the latest regeneration:

- `--relaySlots` (row 228 of §3, and its §1 row): its default moved from `false` to
  `auto`.
- `--edgeDownlinkTier` (row 198 of §3, and its §1 row): added with its default, `fix`
  (M.10). Every row after it moved down one.
- `--uwbNlosModel` (row 238 of §3, and its §1 row): added with its default, `legacy`
  (M.2, `report/PREREG_UWB_NLOS.md`). Every row after it moved down one.
- `--pcsThrUp868`, `--pcsThrUp2p4`, `--pcsThrDown2p4`, `--pcsThrUpUwb`, `--pcsThrDownUwb`
  (rows 120–124 of §3, and their §1 row): added with the default `nan`, "not given"
  (`docs/SPEC_CONFORMANCE_PLAN.md` DC-6). Every row after them moved down five.
- `--suspectPcsFactor` (row 133 of §3, and its §1 row): added with the default `nan`,
  "not given" (DC-7). Every row after it moved down one.
- `--integrityNlosFromScore` (row 237 of §3, and its §1 row): added with the default
  `false` (DC-8). Every row after it moved down one.
- `--entryVelocity` (row 236 of §3, and its §1 row): added with the default `false`
  (DC-10). Every row after it moved down one.
- `--queryTierBinding` (row 286 of §3, and its §1 row): added with an empty default,
  which ns-3 prints with no bracket and which means `first` (NW-3). Every row after it
  moved down one.
- `--handshakeFixes` (row 285 of §3, and its §1 row): added with the default `false`
  (MC-1). Every row after it moved down one.
- `--triggeredUpdates` (row 283 of §3, and its §1 row): added with the default `false`
  (MC-2). Every row after it moved down one.
- `--triggeredUpdateK` (row 284 of §3, and its §1 row): added with the default `1`
  (MC-2). Every row after it moved down one.
- `--edgeUplinkRanges` (row 269 of §3, and its §1 row): added with the default `false`
  (SY-1). Every row after it moved down one.
- `--sf868`, `--bw868`, `--sf2p4`, `--bw2p4` (rows 196–199 of §3, and their §1 row): added with the
  default 0, "not given" (SY-3). Every row after them moved down four.
- `--injectTxPowerDropDb`, `--injectAntennaLossDb` (rows 204–205 of §3, and their §1 row): added
  with the default `0` (SY-5). Every row after them moved down two.
- `--gwLayout`, `--gwLayoutFrac`, `--gwClusterRadiusM`, `--gwStarArms`, `--gwStarSpacingM`,
  `--gwLayoutFile` (rows 131–136 of §3, and their §1 row): added with the defaults
  `legacy`, `0.9`, `300`, `3`, `1500` and empty (MS-2). Every row after them moved down six.
- `--sparseInStep` (row 267 of §3, and its §1 row): added with the default `false`
  (DC-11). Every row after it moved down one.
- `--imuPredict` (row 257 of §3, and its §1 row): added with the default `false`
  (DC-1). Every row after it moved down one.
- `--caseD` (row 256 of §3, and its §1 row): added with the default `false` (DC-4).
  Every row after it moved down one.
- `--entryPrediction` (row 255 of §3, and its §1 row): added with the default `false`
  (DC-13). Every row after it moved down one.
- `--imuWitnessSource` (row 254 of §3, and its §1 row): added with an empty default,
  which means `shs` (DC-9). Every row after it moved down one.
- `--meshLinks` (row 253 of §3, and its §1 row): added with an empty default, which
  means `star` (NW-0). Every row after it moved down one.
- `--measuredSubjects` (row 251 of §3, and its §1 row): added with the default `1`
  (MS-1). Every row after it moved down one.
- `--finderTasking` (row 252 of §3, and its §1 row): added with an empty default,
  which means `tag0` (MS-1). Every row after it moved down one.
- `--reversePathHop` (row 31 of §3, and its §1 row): added with the default `false`
  (NW-8). Every row after it moved down one.
- `--grantGatedUwb` (row 32 of §3, and its §1 row): added with the default `false`
  (NW-7). Every row after it moved down one.
- `--queryV2` and `--identityQueryManifests` (rows 33 and 70 of §3, and their §1 row): added with
  the defaults `false` and empty (NW-9). Every row after them moved down two.
- `--queryTriggeredEscalation`, `--identityQueryUrgent`, `--arbitrationV2` (rows 34–36 of §3, and their §1 row):
  added with the default `false` (NW-4, NW-5). Every row after them moved down three.
- `--arbSlotHoldS` and `--gossipQueue` (rows 68–69 of §3, and their §1 row): added with the
  defaults `10` and `false` (NW-6). Every row after them moved down two.
- `--hopProvenance` (row 100 of §3, and its §1 row): added with the default `false` (NW-1).
  Every row after it moved down one.
- `--sessionOverMesh`, `--sessionStalenessS`, `--sessionTimeoutS` (rows 37 and 42–43 of §3, and their §1 row):
  added with the defaults `false`, `120` and `600` (TA-1a). Every row after them moved down three.
- `--sessionPreciseOnGrant` (row 41 of §3, and its §1 row): added with the default `false`
  (TA-3). Every row after it moved down one.
- `--finderSessions` and `--finderCancelM` (rows 38 and 40 of §3, and their §1 row): added with the
  defaults `false` and `2` (TA-1b). Every row after them moved down two.
- `--requesterEscalation` (row 39 of §3, and its §1 row): added with the default `false`
  (TA-2). Every row after it moved down one.
- `--hopVarianceTerms` (row 101 of §3, and its §1 row): added with the default `false`
  (NW-1 step 3). Every row after it moved down one.
- `--fixRoutingSubjects`, `--routeAlpha`, `--routeBeta`, `--routeGamma`, `--routeSlack` (rows 61 and 64–67 of §3, and their §1 row): added
  with the defaults `false`, `1`, `1`, `1` and `1` (NW-2). Every row after them moved down five.
- `--caseCRelative` (row 104 of §3, and its §1 row): added with the default `false` (DC-3).
  Every row after it moved down one.
- `--specProfile` (row 103 of §3, and its §1 row): added with the default `false` (P6).
  Every row after it moved down one.
- `--radioBasis` (row 239 of §3, and its §1 row): added with the default `sx` (SY-2).
  Every row after it moved down one.
- `--subjectRelayChain` (row 99 of §3, and its §1 row): added with the default `false`
  (N.3, N.15). Every row after it moved down one.
- `--fuseFloorActive` (row 46 of §3, and its §1 row): added with the default `false`
  (N.14). Every row after it moved down one.
- `--finderRequesterEscalation` (row 44 of §3, and its §1 row): added with the default `false`
  (TA-1b). Every row after it moved down one.
- `--subjectEdgeUplink` (row 268 of §3, and its §1 row): added with the default `false`
  (row 55, N.3). Every row after it moved down one.
- `--subjectTriggeredUpdates` (row 282 of §3, and its §1 row): added with the default `false`
  (row 55, N.3). Every row after it moved down one.
- `--subjectBatteryDeplete` (row 281 of §3, and its §1 row): added with the default `false`
  (row 55, N.3). Every row after it moved down one.
- `--subjectFinderPseudoAnchor` (row 280 of §3, and its §1 row): added with the default `false`
  (N.21). Every row after it moved down one.
- `--finderPseudoAnchorStrict` (row 279 of §3, and its §1 row): added with the default `false`
  (N.21). Every row after it moved down one.
- `--silenceStopsRelay` (row 278 of §3, and its §1 row): added with the default `false`
  (N.1). Every row after it moved down one.
- `--deadBatteryNoDraw` (row 270 of §3, and its §1 row): added with the default `false`
  (N.20). Every row after it moved down one.
- `--digestVizOutputs` (row 190 of §3, and its §1 row): added with the default `false`
  (M.4). Every row after it moved down one.
- `--halfDuplex` (row 274 of §3, and its §1 row): added with the default `0`
  (N.6). Every row after it moved down one.
- `--reportRecedingPushes` (row 191 of §3, and its §1 row): added with the default `false`
  (M.9). Every row after it moved down one.
- `--stampBinaryHash` (row 187 of §3, and its §1 row): added with the default `false`
  (M.3). Every row after it moved down one.
- `--subjectTier1Tdoa` (row 273 of §3, and its §1 row): added with the default `false`
  (row 55, MS-1 step 2). Every row after it moved down one.
- `--subjectNlosClassifier` (row 272 of §3, and its §1 row): added with the default `false`
  (row 55, MS-1 step 2). Every row after it moved down one.
- `--subjectIntegrityWitness` (row 271 of §3, and its §1 row): added with the default `false`
  (row 55, MS-1 step 2). Every row after it moved down one.
- `--toaTable` (row 189 of §3, and its §1 row): added with the default `false`
  (MAC §2). Every row after it moved down one.
- `--requestOverMesh` (row 45 of §3, and its §1 row): added with the default `false`
  (N.10, register row 52). Every row after it moved down one.
- `--stampBuildCommit` (row 188 of §3, and its §1 row): added with the default `false`
  (M.3). Every row after it moved down one.
- `--collisionBothLost` (row 276 of §3, and its §1 row): added with the default `false`
  (N.22). Every row after it moved down one.
- `--halfDuplexChip` (row 277 of §3, and its §1 row): added with the default `false`
  (N.6, per chip). Every row after it moved down one.
- `--fuseFloorShort` (row 47 of §3, and its §1 row): added with the default `false`
  (N.14, FUSE-02). Every row after it moved down one.
- `--reportAirtime` (row 275 of §3, and its §1 row): added with the default `false`
  (MAC §2, Transport §7). Every row after it moved down one.
- `--pcsTerm1Inverse` (row 66 of §3, and its §1 row): added with the default `false`
  (register row 17). Every row after it moved down one.
- `--tofFieldSigmaM` (row 81 of §3, and its §1 row): added with the default `0` (HANDOFF_2026_10_08 item 1). Every row after it moved down one.
- `--tofFieldBiasM` (row 82 of §3, and its §1 row): added with the default `0` (HANDOFF_2026_10_08 item 1). Every row after it moved down one.
- `--traceCovariance` (row 84 of §3, and its §1 row): added with the default `false` (HANDOFF_2026_10_08 item 4; the NEES study is S2's, WORKSTREAMS_2026_10_09 §2 B). Every row after it moved down one.
- `--imuQuScale` (row 85 of §3, and its §1 row): added with the default `1` (S2 Part A, A1). Every row after it moved down one.
- `--unknownStart` (row 86 of §3, and its §1 row): added with the default `false` (S2-C). Every row after it moved down one.
- `--traceSubjectConsistency` (row 87 of §3, and its §1 row): added with the default `false` (S2-C). Every row after it moved down one.
- `--measVarScale` (row 88 of §3, and its §1 row): added with the default `1` (S2-C). Every row after it moved down one.
- `--nlosRSource` (row 89 of §3, and its §1 row): added with the default `operative` (S2-C). Every row after it moved down one.
- `--acquireMinAnchors` (row 95 of §3, and its §1 row): added with the default `0` (S2-C). Every row after it moved down one.
`--imuFilter` (row 257 of §3, and its §1 row): added 2026-10-17 with its default, `cv` (TECH_DEBT N.28).
`--ekf5Robust` (row 258 of §3, and its §1 row): added 2026-10-18 with its default, `0` (TECH_DEBT N.33).
`--imuStrideSigmaPct` (row 261 of §3, and its §1 row): added 2026-10-18 with its default, `0`.
`--acquireHandoverCurvature` (row 52 of §3, and its §1 row): added 2026-10-18 with its default, `false` (TECH_DEBT N.58).
`--imuMagSigmaDeg`, `--imuMagGmSigmaDeg`, `--imuMagGmTauS` (row 262 of §3 and the two after it, and their §1 row): added 2026-10-18, default `0` (off), `3`, `120`.
`--ekf5FinderConsider` (row 264 of §3, and its §1 row): added 2026-10-18 with its default, `false`.
`--ekf5RelayGate` (row 263 of §3, and its §1 row): added 2026-10-18 with its default, `false`.

A fresh `--PrintHelp` differs from the `ff4f8bb` one in those rows only, and adds the
104 names. Three flags changed meaning without changing their printed default, and §1
says how: `--pcsForm` gained the value `calibrated`, `--pcsW1..3` take 0, and
`--suspectRecovery` gained the value `downweight`.

**Why this file exists.** No tracked document listed the scenario's flags.
`HANDOFF_2026_09_09.md` §8 asked for `--gwArcDeg`, `--gwArcCentreDeg`, `--trailDeviateSide`
and `--searchLaunchAtS` to get "a line in whichever document lists scenario flags", and
there was no such document. Those four are rows 174, 181, 182 and 215 of §3.

**It is a snapshot, not the ground truth.** `RULES.md` §8 puts the ground truth in the
`cmd.AddValue` block of `scratch/ca_tbl_causal_sim.cc` and in the `run_id.txt` sidecars.
This file goes stale the same way every prose document does. Regenerate §3; do not edit it
by hand.

**How it was generated.**

```bash
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --PrintHelp
```

§3 takes, in registration order, each option's name and the bracketed default at the end of
its line. A flag whose default is an empty string has no bracket; §3 shows it as (empty). The help text is left out on purpose. It is the argument for each default, it is
long, and it belongs in the source; read it with `--PrintHelp`.

Two checks found what §1 lists:

- The names registered at `084a7ff` against those registered here: 10 added, 0 removed.
- The frozen `084a7ff` binary's `--PrintHelp` (`~/catbl_ref`) against this one's, flag by
  flag: 1 default changed.

---

## 1. Added or changed 2026-09-27 – 2026-10-02

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
| `--suspectRecovery` | string | `none` | value `downweight` added 2026-09-30 | `downweight` is the spec's response to a SUSPECT fix (Physical 2.6, DC-7): accepted at R x `--suspectInflateFactor` (the TDoA half too), still flagged so it cannot count toward a de-escalation, and its PCS scaled by `--suspectPcsFactor` | At inflate 1 and factor 1 it equals `bounded` with `--suspectRecoveryN=1` in every trace column but `rejected` (G29). `--suspectForcesEscalation` still applies; the spec's form sets it false. `none`, `inflate`, `bounded` and `reinit` are byte-identical to the build before |
| `--suspectPcsFactor` | double | `nan`, not given (0.5) | added 2026-09-30 | Under `downweight`, what a suspect cycle's PCS is multiplied by; under `--pcsForm=v2`, log10(1/factor) is subtracted instead, because a v2 score can be negative | 0.5 is OURS. FATAL outside (0, 1], and FATAL without `--suspectRecovery=downweight`, where it would do nothing |
| `--integrityNlosFromScore` | bool | `false` | added 2026-10-01 | The integrity check's eligibility from the classifier -- the spec's "NLOS_score is LOW" (Physical 2.6, DC-8) -- instead of "every link tagged LOS": eligible when the mean NLOS_score of the fix set's scored links is below `--nlosScoreThreshold`; with none scored the tag decides. Writes `integrity_ca_tbl_full_seed<N>.csv`, digested | Needs `--nlosClassifier=true`, or FATAL. At seed 7 the default tag leaves 0 of 305 evaluated fixes eligible; the score leaves 296 of 297, and 152 are called suspect (TECH_DEBT I.1: a population, and a new problem under the default reject response) |
| `--entryVelocity` | bool | `false` | added 2026-10-01 | Table entries carry the producer's velocity (Physical 2.1, DC-10): a field node its EKF velocity, a finder its GNSS velocity (displacement over the time since its last publication), a gateway 0. Writes `entries_<arm>_seed<N>.csv` (what each producer published, what every node holds, once per cycle), digested | The default publishes 0,0, as every result so far did. Nothing reads the fields yet, so the flag moves no other output byte for byte (G31) |
| `--queryTierBinding` | string: `first`, `per-tier` | empty (not given) = `first` | added 2026-10-01 | Which radio each tier's identity-query driver sends on (NW-3). `first` is every result so far: all three drivers on gateway 101's 868 MHz radio, so "2.4 GHz" queries go out at 868 MHz's time on air and are heard by 868 MHz radios. `per-tier` binds each to that tier's radio. Given explicitly, either value prints the binding on stdout | Needs `--identityQueryPeriodS > 0`, or FATAL; any other value is FATAL. Explicit `first` is the default byte for byte (G32) |
| `--handshakeFixes` | bool | `false` | added 2026-10-02 | The escalation handshake as MAC §8 specifies it (MC-1): an ACK names the frame it acknowledges; an anchor arbitrates first and answers every request with a GRANT saying granted or denied, never a bare ACK; a denial is retried on the ACK timeout; requests carry this cycle's terms (the pre-emption send carried none) and `target=2`; only escalations into UWB send a request. Prints one `MC-1 handshake:` line per arm | Needs `--sendHandshakeFrames=true`, or FATAL. Every frame, the energy and the duty move under it. In G33's regime (NC9 + arbitration + 2 peer tags, seed 1) it answers 205 denials, retries 162 requests and matches 196 ACKs by id, 0 unmatched |
| `--triggeredUpdates` | bool | `false` | added 2026-10-02 | The spec's triggered update and last gasp (MAC §3, §8; MC-2) for the measured node: a large correction (the posterior moves more than `--triggeredUpdateK` sqrt(trace P) from the prediction) pushes the corrected entry with the redundant class's 3 jittered repeats; when `--batteryDepleteJ` is crossed one unrepeated last gasp goes out before the radio is silenced. Billed on the tier that transmitted. Writes `triggered_<arm>_seed<N>.csv`, digested, and one `MC-2` line | Frames, energy and duty move. At seed 7 × 900 s with a 3 J battery: 1 large correction, 1 last gasp (G34). A silenced radio still forwards floods (TECH_DEBT N.1) |
| `--triggeredUpdateK` | double | `1` | added 2026-10-02 | How far, in sqrt(trace P) of the previous posterior, the posterior must move from the prediction to be a large correction (MC-2). 1 is OURS: the mesh's copy is then off by more than the uncertainty it was published with | FATAL unless > 0, and FATAL without `--triggeredUpdates`. The plan's draft 3 never fires: a Kalman update moves the mean by about one prior sigma at most (1.05 over 359 cycles) |
| `--edgeUplinkRanges` | bool | `false` | added 2026-10-02 | Bills the uplink that carries a tier-2 or tier-3 range from the node to the gateway estimator (SY-1, TECH_DEBT L.2.3): one frame per cycle with a range, on the tier that ranged, 9 B header + 6 B per answered anchor (OURS). One `SY-1` line per arm | Needs `--gatewayEdgeServer=true`, or FATAL. At defaults only energy moves, only on tiers 2 and 3 (G35): `static_2p4` +213 frames at seed 7 × 900 s. The UWB branch is unexercised (no regime returns a UWB range) |
| `--sf868`, `--bw868`, `--sf2p4`, `--bw2p4` | uint, double | `0`, not given (SF7, 125 kHz; SF7, 800 kHz) | added 2026-10-02 | Tier-1 and tier-2 spreading factor and bandwidth (Physical 1.1, SY-3). They replace the literal `phy[]` values, which flow into the engines, the MACs, the energy model and the relay-slot check. A bandwidth other than the table's (125 kHz; 800 kHz) shifts the engine's sensitivity table by 10 log10(BW/BW0) dB. Given, they print one `SY-3 PHY` line and one `SY-3 engine sensitivity` line per arm | FATAL outside SF7-SF12 / 125-250-500 kHz on 868 MHz, and outside SF5-SF12 / the SX1280 bandwidths (or 800 kHz) on 2.4 GHz. Explicit defaults are the default byte for byte (G36). 800 kHz is not an SX1280 setting; recorded, not changed |
| `--injectTxPowerDropDb`, `--injectAntennaLossDb` | double | `0` (off) | added 2026-10-02 | Fault classes for the evaluation (exec Phase 5; SY-5): the measured node's transmit power drops (cold-sagged battery), or its antenna loses gain on what it sends AND receives (ice, a covering hand), over the `--injectBiasFraction` window. Tier 1 inverts RSSI against the nominal power, so its ranges read long by 10^(dB/(10 n)); tiers 2 and 3 lose SNR and links. No random draw. One `SY-5` line | > 0 needs `--injectBiasFraction > 0`, negative is FATAL. Magnitudes OURS. 10 dB at the default n = 2.3 skews tier-1 ranges ×2.72 (G37) |
| `--gwLayout` (+ `--gwLayoutFrac`, `--gwClusterRadiusM`, `--gwStarArms`, `--gwStarSpacingM`, `--gwLayoutFile`) | string, double, uint | `legacy`; 0.9, 300, 3, 1500, empty | added 2026-10-02 | Gateway placement shapes GEO-01 compares (MS-2): `perimeter` round the frac square, `cluster` round the camp, `star` along roads out of the camp, `lshape` on two perpendicular roads, `file` from an x,y CSV. They replace the gateways in every tier list, ids 101+. The camp is the base camp if `--baseCampFrac` gives one, else the west midpoint of the frac square. One `MS-2` line with every coordinate | `legacy` is the default byte for byte (G38). Needs `--topology=0` and, except `file`, `--nGateways > 0`; FATAL otherwise. Every constant OURS |
| `--sparseInStep` | bool | `false` | added 2026-10-02 | Cases A/B solved inside `Step()`, after the prediction, against the propagated prior (Physical 2.4; DC-11). The point is fused once and the pair never also as ranges; a solved cycle is a fix (`fixKind` CASE_A/B) that does not count toward demotion; with relays appended the set takes the full path. Writes `sparse_<arm>_seed<N>.csv`, digested, and one `DC-11` line | Replaces `--sparseCaseA`: the two together are FATAL. Margin `--sparseCaseAMargin`. NC3 regime, seed 7: 73 of 131 two-range cycles solved, each a fix, against 64 recorded as no-fixes on the old path (G39) |
| `--imuPredict` | bool | `false` | added 2026-10-02 | The filter's prediction takes a synthetic-IMU displacement (Physical 2.2; DC-1): a dead-reckoning error model turns the walker's true displacement into what an IMU would report -- heading error from a per-run gyro bias plus angle random walk (MPU-6050, Rudyk 2020), a per-run 2.66% scale error (OURS) -- and `PredictWithControl` predicts with it, covariance the accumulated error since the last fused fix. Own random stream. Writes `imu_<arm>_seed<N>.csv`, digested, and one `DC-1` line | NEES over 20 seeds 2.15, inside the chi^2_40/20 band [1.22, 2.97]; an independent per-step Q gives 24.6 (G40). The SHS gait engine is untouched |
| `--caseD` | bool | `false` | added 2026-10-02 | Case D (Physical 2.4D; DC-4): a set of exactly one range is applied as a scalar measurement on the propagated state when its NIS <= 6.63 (chi^2 1 dof 0.99). Not a position fix, but the covariance is live: PCS is computed, the normal escalation decision runs, and the cycle does not count toward demotion (OURS). Writes `cased_<arm>_seed<N>.csv`, digested, and one `DC-4` line | Needs `--sparseInStep` (FATAL); without `--imuPredict` a WARNING (constant-velocity propagation). NC3 regime, seed 7: 65 cycles, 30 escalated, 0 demoted (G41) |
| `--entryPrediction` | bool | `false` | added 2026-10-02 | A held finder entry is carried forward before use (Physical 2.2; DC-13): position x + v·age with DC-10's velocity, added variance (0.5 m/s · age)² (OURS) instead of the isotropic (walk speed · age)². Gateways are never substituted. Writes `entrypred_<arm>_seed<N>.csv`, digested, and one `DC-13` line | Needs `--entryVelocity`, or FATAL. 504 of 504 substitutions moved in the converging-finder regime (G43) |
| `--imuWitnessSource` | string: `shs`, `model` | empty (not given) = `shs` | added 2026-10-02 | Which IMU track the `--imuCrossCheck` witness compares (final plan item 2; DC-9). `shs` is every result so far: the SHS absolute track against the EKF estimate. `model`: the synthetic IMU track (DC-1), anchored at the last fused fix and advanced by u, against the cycle's raw radio fix, tolerance K sqrt(trace R_fix + trace Sigma_acc). Counters split into evaluations, refusals and absorbed refusals. Writes `witness_<arm>_seed<N>.csv`, digested, and one `DC-9` line | `model` needs `--imuPredict` and `--imuCrossCheck`, or FATAL; any other value is FATAL. NC9 regime, seed 1: 25 evaluations and 1 refusal with the evidence gate off; with it on, 1 of 1 refusal absorbed (G44) |
| `--meshLinks` | string: `star`, `all` | empty (not given) = `star` | added 2026-10-03 | Which radio pairs can hear each other (the spec's mesh; NW-0). `star` is every result so far: a link only between a field node and an anchor. `all`: every registered pair on a tier both carry -- tag-tag, gateway-gateway, finder-gateway. Under it DISCOVERY is one-hop, the ranging poll addresses anchors only, the node's sleeping radios neither receive nor relay (the channel skips them), and its relays are billed. One `NW-0` line | Any other value is FATAL. About N^2 link evaluations per frame: 145 240 at G45's 12 nodes and 600 s, 102 776 of them on pairs the star model never evaluates; wall 15.1 s against 17.9 s for star (one-hop DISCOVERY removes frames as the extra pairs add evaluations) |
| `--measuredSubjects` | uint, 1..`--nTags` | `1` | added 2026-10-03 | How many field nodes the full pipeline localises (the spec's five field nodes; MS-1). `1` is every result so far. N > 1: tags 2..N run the core of tag 0's cycle on their own accounts -- ranging on the active tier, the tier-1 floor, `Step()` with their own escalation state machine, handshakes with their own terms (PeerRequestDriver retires), the downlink, listen/sleep -- each in its own 50 ms slot of a round before tag 0's instant (the relay round under `--relaySlots`). A relay that is a subject is ranged, charged and stepped once. Writes `subject_summary.csv` and `subjtrace_<arm>_seed<N>_subj<id>.csv`, both digested, and one `MS-1` line per arm and seed | N > `--nTags` or N = 0 is FATAL. SHS/IMU witness, TDoA, NLOS scorer, sessions, battery depletion, triggered updates, edge uplink and entry prediction run for tag 0 only, and a WARNING names any that is on. Wall 28.2 s against 27.5 s at N = 1 (ca_tbl_full, 600 s, five tags) |
| `--finderTasking` | string: `tag0`, `roundrobin` | empty (not given) = `tag0` | added 2026-10-03 | Which subject each finder searches for (the spec's rescuers searching for specific people; MS-1). `tag0` is every result so far: every finder steers on tag 0's entry. `roundrobin`: finder k (0-based) steers on subject (k mod N) + 1's freshest entry, N = `--measuredSubjects`, and the `--finderDispersal` spread is over the finders tasked to the same subject. The rule is OURS. Writes `tasking_<arm>_seed<N>.csv` (every finder-cycle after dispatch: target, phase, entry, goal), digested, and one `MS-1 tasking` line | Needs `--measuredSubjects` > 1, `--nFinders` > 0 and `--finderMode` 1 or 2, else FATAL. A finder that never hears its subject holds position (mode 1) or keeps sweeping (mode 2): at G47's regime with five subjects, three of four finders stayed blind for 600 s (TECH_DEBT N.4) |
| `--reversePathHop` | bool | `false` | added 2026-10-03 | With `--reversePath`, the reverse path is real (Transport §1; NW-8). `false` keeps `--reversePath` as it always was: the cache holds the QUERY's originator and both branches send one one-hop frame. `true`: each node caches the previous hop (the channel's transmitter), the RESPONSE is unicast to it and forwarded hop by hop, the 300 s window is honoured, a miss floods with the default hop limit, and copies retransmitted after a lost ACK are absorbed per (query, answerer). The RESPONSE bills 8 more bytes (query_id, requester, answerer). One `NW-8` line | Needs `--reversePath` and `--identityQueryPeriodS` > 0, else FATAL. Multi-hop needs `--meshLinks=all`. G48: 137 hops, 40 forwarded, 55 delivered, 42 duplicates absorbed (G32 regime, mesh links, 900 s) |
| `--grantGatedUwb` | bool | `false` | added 2026-10-03 | An escalation into UWB commits only on a delivered grant (MAC §8; Transport §4; NW-7). `false` is every result so far: the tier changes inside `Step()` and nothing at the node reads ESC_GRANT. `true`: the escalation is a pending request (the ESC_REQUEST still goes out); the node stays on 2.4 GHz, frozen, until a GRANT(granted) answering its own request arrives, and moves to UWB at the next cycle; a denial drops the request and its retries; exhausted retries time it out; coverage loss still demotes and cancels. Every measured subject. Writes `grants_<arm>_seed<N>.csv` (request/confirm/deny/timeout/late/coverage per subject), digested, and NW-7 lines | Needs `--handshakeFixes`, else FATAL. Under it the trace's `escalated` on 2.4 GHz marks the request, and the escalation count the grant. Static arms are exempt. G49 regime: tag 0 61 requests, 29 confirmed, 24 denied, 8 timed out |
| `--queryV2`, `--identityQueryManifests` | bool; string: comma-separated ids 1..255 | `false`; empty (= `--identityQueryTarget` alone) | added 2026-10-03 | QUERY and RESPONSE carry what the spec says (Network §4; NW-9). `false` is every result so far: one manifest per QUERY, an empty RESPONSE, no handler at the requester, and an answering target still rebroadcasts. `true`: a QUERY carries query id, manifest list, requester and priority; a node answers every listed manifest it can (target or proxy), each RESPONSE carrying the entry with the producer's status, the manifest and the query id (12 bytes billed); it relays with the answered manifests removed, and not at all if nothing is left. Writes `responses_<arm>_seed<N>.csv` and `queryv2_<arm>_seed<N>.csv`, digested, and one `NW-9` line | `--queryV2` needs `--identityQueryPeriodS` > 0; a list needs `--queryV2`; an id outside 1..255 is FATAL. A node can transmit two frames at once (TECH_DEBT N.6), so an answer and the relay of the rest can collide |
| `--queryTriggeredEscalation`, `--identityQueryUrgent`, `--arbitrationV2` | bool | `false` | added 2026-10-03 | NW-4: an URGENT query for a node's own manifest forces its escalation toward UWB, one tier a cycle, whatever the PCS (the suppress-only preconditions still apply, counted), and its ESC_REQUEST carries is_query_triggered; `--identityQueryUrgent` sends the identity queries URGENT (priority 1, OURS) until the session requester does. NW-5: U(r) with T_batt from the battery remaining (min(1/max(rem, 0.05), 20) x 0.05, OURS) and T_query from the bit; an arbiter considers only requests with its own UWB-reach evidence (a UWB exchange, or a 2.4 GHz range under 29.5 m, within 10 s, OURS), queues the rest with their position, re-scores the queue each cycle with staleness aging by time waited, and drops an entry after 30 s (OURS); finders arbitrate too. Writes `arb_<arm>_seed<N>.csv`, digested, and NW-4 and NW-5 lines | `--queryTriggeredEscalation` needs `--queryV2` and `--identityQueryUrgent`; `--identityQueryUrgent` needs `--queryV2`; `--arbitrationV2` needs `--uwbArbitration` and `--handshakeFixes`, else FATAL. Every out-of-range anchor answers with a DENY (TECH_DEBT N.8) |
| `--arbSlotHoldS`, `--gossipQueue` | double; bool | `10`; `false` | added 2026-10-03 | `--arbSlotHoldS`: seconds an awarded UWB slot is held (CaTblMacConfig::arbSlotHoldS); 10 is the C4 harness constant every result used, and a longer hold makes requests contend. `--gossipQueue` (NW-6): with `--arbitrationV2`, the grant gossip carries the arbiter's queue (4 bytes an item, billed); the lower-id tie-break extends to queues (give up, with a final denial, or refuse to queue a request a higher-id anchor queues or has granted; free a slot whose grant was yielded); finders gossip too over `--meshLinks=all`, unbilled. One `NW-6` line | `--arbSlotHoldS` must be > 0 and needs `--uwbArbitration` when changed; `--gossipQueue` needs `--anchorGrantGossipS` > 0 and `--arbitrationV2`, else FATAL. Two arbiters rarely hold one request in the five-subject regimes (TECH_DEBT N.9) |
| `--hopProvenance` | bool | `false` | added 2026-10-03 | The hop count of what a fix fused reaches the decision and the RESULT (Physical §2.5, Transport §3; NW-1 steps 1-2). `false` is every result so far: `Step()` is passed a literal 0 and the RESULT says 0. `true`: the measured tag's and every short relay's `Step()` get the deepest leg of the set they fuse, and a fused fix carries it as hop_count_at_fix. hopWeight stays 0, so no decision moves; the count is carried. Writes `hops_<arm>_seed<N>.csv`, digested, and one `NW-1` line | Needs `--relayedRanging`, else FATAL. Per-hop variance in σ_eff (step 3) is not built |
| `--sessionOverMesh`, `--sessionStalenessS`, `--sessionTimeoutS` | bool; double; double | `false`; `120`; `600` | added 2026-10-03 | TA-1a: the base station (requester 0) opens one session over what its identity queries ask and is fed only by RESPONSEs delivered to a gateway, once per answer, the RESULT built from the delivered entry (position, Σ, the producer's fix tier and hop count, PCS byte, integrity verdict). Staleness and timeout configure that session. Writes `session_<arm>_seed<N>.csv`, digested, and one `TA-1` line | `--sessionOverMesh` needs `--sessionLayer` and `--queryV2`; the two times are read only under it, else FATAL. Finder sessions (TA-1b) not built |
| `--sessionPreciseOnGrant` | bool | `false` | added 2026-10-03 | TA-3: under `--sessionOverMesh` the session becomes LOCATED_PRECISE on a delivered entry that names a confirmed UWB grant (NW-7), one delivery after the fact; a result from a UWB fix without a grant is coarse. The TA-1 line also reports the batch's precision spread (TA-4), UNDEFINED below two precise targets | Needs `--sessionOverMesh` and `--grantGatedUwb`, else FATAL |
| `--finderSessions`, `--finderCancelM` | bool; double | `false`; `2` | added 2026-10-03 | TA-1b: under `--sessionOverMesh` every finder runs its own session for subject (k mod N) + 1 (OURS, as `--finderTasking`), queries for it from its own tier-1 radio on the identity query period, and is fed only by answers that reach it as the requester (over the reverse path to a moving node). The session is cancelled (closed) once the finder is within `--finderCancelM` of its target, true positions (OURS). Rows tagged with the finder id in `session_*`, one `TA-1b` line per finder | `--finderSessions` needs `--sessionOverMesh` and `--nFinders` > 0; `--finderCancelM` must be > 0 and is read only under it, else FATAL. With the 8 m stand-off, 2 m rarely fires |
| `--requesterEscalation` | bool | `false` | added 2026-10-03 | TA-2: under `--sessionOverMesh` with `--arbitrationV2`, a delivered answer whose PCS is below the 2.4 GHz up-threshold, from a node not on UWB, makes the base station submit an escalation request for that target, at most one per target per `--arbSlotHoldS` (OURS). It goes to the arbiter the target's entry names as the one it last had UWB-reach evidence with (10 s, OURS), never the nearest gateway; with none the request is unroutable and not sent. It reaches the arbiter instantly and unbilled (OURS). The arbiter scores it (query_triggered set under `--identityQueryUrgent`), and its on-behalf GRANT is accepted by the target as its escalation into UWB (or as the confirmation of its own pending request), refused on 868 MHz or a static arm. Writes `reqesc_<arm>_seed<N>.csv`, digested, and a TA-2 line | Needs `--sessionOverMesh`, `--arbitrationV2` and `--grantGatedUwb`, else FATAL. Under the default PCS form every such answer qualifies, and most requests are unroutable (TECH_DEBT N.10) |
| `--hopVarianceTerms` | bool | `false` | added 2026-10-03 | NW-1 step 3: every relay leg that reaches admission, on either leg of the chain, adds the von Neumann σ of its RSSI/SNR readings (window 8, three readings, keyed by initiator and relay) converted to range by the tier's nominal path-loss slope at the range (OURS: tiers 2 and 3 range by time of flight), σ_leg = sqrt(σ² + σ_var²); a leg the tier's NLOS score (2.4 GHz: ToF/RSSI excess against a pool of legs; UWB: the CIR leading-edge test) puts at or above `--nlosScoreThreshold` is multiplied by nlosSigmaInflation, as a direct NLOS link is; C5 composes σ_eff from σ_leg. relay_chain_* gains the leg's readings and terms; one NW-1 line per arm | Needs `--relayedRanging`, `--relayChain` and `--nlosClassifier`, else FATAL. On tag-to-tag legs the variance term dwarfs the raw σ (TECH_DEBT N.11). Not in `--specProfile` since 2026-10-03 (owner decision, TECH_DEBT N.11) |
| `--fixRoutingSubjects`, `--routeAlpha`, `--routeBeta`, `--routeGamma`, `--routeSlack` | bool; double; double; double; uint | `false`; `1`; `1`; `1`; `1` | added 2026-10-03 | NW-2: `--fixRoutingSubjects` builds one router per tier, the measured tag included; each cycle a node learns gateway links, advertises, originates its ENTRY_PUSH and forwards only in the router of the tier it ranged on, so a routed frame is on the one tier its sender transmits on (sect 6); a relay that is not a subject stays on `--relayTier`; a route whose next hop has since moved tier is refused (counted). Writes `fix_routing_<arm>_seed<N>_tier<k>.csv` per tier with the confidence route's three sums, digested, and an NW-2 line per router. `--routeAlpha/--routeBeta/--routeGamma/--routeSlack` weight the confidence cost α·NLOS + β·RSSI_var + γ·jitter and its slack (OURS; the rows gain the three sums when one is changed). Since 2026-10-03 `--specProfile` sets (0, 0, 1, 1), CAMP-SPEC's registered sweep pick, by owner decision; a weight the preset applied is not refused when `--fixRouting` is turned off explicitly | `--fixRoutingSubjects` needs `--fixRouting` and `--measuredSubjects` > 1; the weights need `--fixRouting`, must be finite and ≥ 0, and slack ≤ 3, else FATAL. `--fixRouting` with subjects but without the flag routed them on `--relayTier` whatever they ranged on, and warned; since 2026-10-03 (owner decision) it is FATAL (TECH_DEBT N.12) |
| `--caseCRelative` | bool | `false` | added 2026-10-03 | DC-3, Physical §2.4 Case C: a measured subject with no anchor range this cycle (active tier and floor) ranges each peer subject its active radio has heard, on that tier, billed there; the ranges ride its next entry at 3 bytes a range (OURS, billed as the extra airtime) and reach the base station when an answer to its query carries that entry to a gateway, within 10 s (OURS). Once a cycle's window closes the base station reconstructs each connected set of 3+ by classical MDS (missing pairs by shortest path, OURS) and a pair as its distance. Never fused into a filter. Writes `case_c_ranges_*` and `case_c_relative_*`, digested, and one DC-3 line | Needs `--measuredSubjects` > 1, `--meshLinks=all` and `--sessionOverMesh`, else FATAL. Peer ranges on 868 MHz are coarse, so the shapes are too (TECH_DEBT N.13) |
| `--specProfile` | bool | `false` | added 2026-10-03 | P6, the spec preset (plan §3.1, owner decision 1): 102 settings (2026-10-04, after the spec's 3-gateway geometry) across the Physical, decision-core, MAC, Network, Transport, System and Deployment groups, each applied unless the same flag is given explicitly (which always wins). Resolved before the parse: unset settings are put ahead of the user's arguments. `spec_profile.txt` (digested) lists what was set and what was kept, and a NOTE goes to stderr. The help lists every setting and the section it serves. The radio basis is `--radioBasis=lr1121` (SY-2). The geometry is the spec's deployment since 2026-10-04, `--nGateways=3 --gwLayout=star` (GEO-01 stage 2's best N = 3 layout; owner call delegated, DESIGN_DECISIONS §9); before, `--nGateways=8` (a ring), GEO-01's registered fallback when no candidate met the SAR criterion. The PCS is `--pcsForm=calibrated` with CAL-03's weights (2.995, 0, 0.005) and thresholds, re-fitted on the finished preset on 2026-10-04 and adopted by CAL-02's registered rule (CAL-02's (0, 1.92, 1.08) were fitted at eight gateways). The routing weights are CAMP-SPEC's pick (0, 0, 1, 1), by owner decision, and `--subjectRelayChain` is on (both unused, not refused, when `--fixRouting` or `--relayChain` is turned off explicitly). `--sparseCaseA` is left out (FATAL with `--sparseInStep`, which replaces it), and so are `--hopVarianceTerms` (owner decision, N.11) and `--fuseFloorActive` (FUSE-01b failed FUSE-01's adoption rule, report/FUSE01B_RESULTS.md); `--relayVarBudget=24336` and `--identityQueryPeriodS=30` are added (OURS) Since 2026-10-08 it also takes FIXES-N26's four fixes (`--loraDualBase`, `--uwbCrossTierPoll`, `--tdoaInnovationGate`, `--mahalUseR` with `--integrityChi2=1992.9`), by owner override of that campaign's registered rule (DESIGN_DECISIONS §9). | FATAL with `--topology=1`. Two mechanisms have a zero population under it (G24, TECH_DEBT N.15) |
| `--radioBasis` | string: `sx`, `lr1121` | `sx` | added 2026-10-03 | SY-2: the radio parts the energy model bills tiers 1 and 2 on. `sx` is the SX1262 (868 MHz) and SX1280 (2.4 GHz) of every result so far; `lr1121` bills both from the Semtech LR1121 datasheet, DS.LR1121.W.APP Rev 2.0, Tables 3-4..3-6: TX 28 / 26 mA, RX 7.8 / 7.5 mA (RxBoosted), CAD at the RX current (OURS), idle 1.25 mA (STBY_RC), sleep 1.6 µA. UWB stays DW3000, and no timing moves, so at defaults only energy columns change (G61). Every field node's account and the gateways' gossip accounts; one SY-2 line. In `--specProfile` | any other value is FATAL |
| `--subjectRelayChain` | bool | `false` | added 2026-10-04 | N.3 / N.15: measured subjects take part in L.1.2's relay chain. A subject with three or more direct ranges (active tier or floor) holds a fix at depth 0; one short of three on both ranges, on its active tier (never 868 MHz, where A1 admits nothing), every node that holds a fix this cycle and is awake on that tier: plain relays and lower-id subjects, never tag 0. Admitted ranges join its active set only if they complete three (OURS). Fused, it carries the deepest holder's depth + 1, so tag 0's range through it is depth 2. `relay_chain_*` gains the subjects' rows; one L.1.2 subject-chain line (G62). In `--specProfile` since 2026-10-04 | needs `--relayChain` and `--measuredSubjects` > 1, else FATAL |
| `--fuseFloorActive` | bool | `false` | added 2026-10-04 | N.14: when the active tier has fewer than three ranges and the floor ranged, the fix set is the union (the active tier's ranges, relays included, then the floor's), and Step()'s sparse cases act on it below three; the floor alone at three or more keeps the old path. A union holding active ranges is labelled with the active tier (OURS). Tag 0 and every measured subject. Writes `fusefloor_<arm>_seed<N>.csv` (digested) and one N.14 line (G63). In `--specProfile` for part of 2026-10-04, by FUSE-01's registered rule (report/FUSE01_RESULTS.md); out again since FUSE-01b, with the subjects' finder oracle closed, failed the same rule on tag 0's endgame CEP95 (+13.7 %, report/FUSE01B_RESULTS.md; N.14 reopened for the owner) | needs `--sparseInStep`, else FATAL |
| `--finderRequesterEscalation` | bool | `false` | added 2026-10-04 | TA-1b: a finder whose own session receives a clean result below the 2.4 GHz up-threshold for a target not on UWB asks for UWB, at most once per target per `--arbSlotHoldS`: in its own queue when it holds UWB-reach evidence of the target (it is an arbiter, NW-5), else at the arbiter the target's entry names (as TA-2's, instant and unbilled), else unroutable and not sent (OURS). Writes `finderreq_<arm>_seed<N>.csv` (digested) and one TA-1b line (G64). In `--specProfile` since 2026-10-04 (decision 8); a preset-applied value is unused, not refused, when `--finderSessions` or `--requesterEscalation` is turned off explicitly | needs `--finderSessions` and `--requesterEscalation`, else FATAL |
| `--subjectEdgeUplink` | bool | `false` | added 2026-10-04 | Row 55 / N.3: subjects 2..N bill the uplink of their tier-2/3 ranges as `--edgeUplinkRanges` bills tag 0's: one frame on the tier that ranged, 9 B + 6 B per answered anchor (OURS), billed to the subject. Only energy moves at defaults (G65); one SY-1 subjects line. In `--specProfile` since 2026-10-04; a preset-applied value is unused, not refused, when `--edgeUplinkRanges` is turned off explicitly | needs `--edgeUplinkRanges` and `--measuredSubjects` > 1, else FATAL |
| `--subjectTriggeredUpdates` | bool | `false` | added 2026-10-04 | Row 55 / N.3: subjects 2..N send MC-2's LARGE CORRECTION as tag 0 does: the corrected entry published and pushed as a TRIGGERED_UPDATE on the tier the subject transmitted on, three repeats, billed to it. No last gasp (subjects have no battery depletion). Writes `subjtriggered_<arm>_seed<N>.csv` (digested) and one MC-2 subjects line (G66). In `--specProfile` since 2026-10-04; a preset-applied value is unused, not refused, when `--triggeredUpdates` is turned off explicitly | needs `--triggeredUpdates` and `--measuredSubjects` > 1, else FATAL |
| `--subjectBatteryDeplete` | bool | `false` | added 2026-10-04 | Row 55 / N.3: subjects 2..N run `--batteryDepleteJ`'s fault as tag 0 does: at the crossing their radios fall silent, after one unrepeated last gasp under `--subjectTriggeredUpdates`; then no exchange, request or peer range. A silent radio still forwards floods (N.1), and listen and sleep are still billed (N.20). One FAULT line per subject (G67) | needs `--batteryDepleteJ` > 0 and `--measuredSubjects` > 1, else FATAL |
| `--subjectFinderPseudoAnchor` | bool | `false` | added 2026-10-04 | N.21: a subject's range to a finder uses the finder's REPORTED position (the freshest entry its radios hold, carried forward under `--entryPrediction`, its covariance projected into sigma), as tag 0's does under `--finderPseudoAnchor`; with no entry held the range is dropped (received and billed, not used); under `--bidirectionalDigest` the subject asks the finders' digest question on its active tier, subject k of N at (0.5 + k/(2N)) of the finder digest period (OURS; unbilled, as tag 0's). By default subjects range finders against their TRUE positions. Writes `subjpseudo_<arm>_seed<N>.csv` (digested) and one N.21 line (G68). In `--specProfile` since 2026-10-04; a preset-applied value is unused, not refused, when `--finderPseudoAnchor` is turned off explicitly | needs `--finderPseudoAnchor` and `--measuredSubjects` > 1, else FATAL |
| `--finderPseudoAnchorStrict` | bool | `false` | added 2026-10-04 | N.21, tag 0's half: under `--finderPseudoAnchor` no finder range uses the finder's true position. By default tag 0 keeps the truth when it holds no entry for the finder, or when the report's variance cannot be projected; with the flag, a range tag 0 holds no entry for is dropped (received and billed, not used), and where the variance cannot be projected the report is used uninflated (the same fallback for subjects under `--subjectFinderPseudoAnchor`; defensive, no population in G69). Writes `pseudo_<arm>_seed<N>.csv` (tag 0's finder ranges, digested) and one N.21 strict line (G69). In `--specProfile` since 2026-10-04; a preset-applied value is unused, not refused, when `--finderPseudoAnchor` is turned off explicitly | needs `--finderPseudoAnchor`, else FATAL |
| `--silenceStopsRelay` | bool | `false` | added 2026-10-04 | N.1: while a radio is silenced (a spent battery under `--batteryDepleteJ` / `--subjectBatteryDeplete`, or `--injectSilentId`), the MAC refuses at CAD every frame another node originated, and counts it; new relays, queued relays, their CAD retries, repeats and retransmissions alike. The node's own frames are untouched, so a frame queued before the silence (MC-2's last gasp) still leaves. One N.1 line (G70) | needs `--batteryDepleteJ` > 0 or `--injectSilentId`, else FATAL |
| `--deadBatteryNoDraw` | bool | `false` | added 2026-10-04 | N.20: from the cycle its budget runs out, a dead node (tag 0, and each subject under `--subjectBatteryDeplete`) is billed for no reception, no neighbour-search window and no sleep, and tag 0 for no escalation handshake (its frame was already suppressed). A frame it transmitted (the last gasp, a relay past CAD) is still billed. One N.20 line (G71) | needs `--batteryDepleteJ` > 0 and `--silenceStopsRelay`, else FATAL |
| `--digestVizOutputs` | bool | `false` | added 2026-10-04 | M.4: `events_*`, `nodes_*`, `nbrtable_*`, `loctable_*` and `deployment.json` get a SHA-256 line in `run_id.txt`; without it they are written under `--vizEvents` and not digested (house rule 4.5). Nothing else changes (G73). In `--specProfile` since 2026-10-04: the preset sets `--vizEvents`, and its row said digested before this was true | needs `--vizEvents`, else FATAL |
| `--halfDuplex` | uint: 0, 1, 2 | `0` | added 2026-10-04 | N.6: every radio half-duplex. 1 counts the frames that came due while the radio's own frame was on air and the receptions made while transmitting, and changes nothing else (outputs byte-identical to 0); 2 sends such a frame back to CAD when the own frame ends, and drops such a reception (an `events_` row with outcome `halfduplex`). Per radio. One N.6 line (G74). `--specProfile` sets 2 since 2026-10-04 | any other value is FATAL |
| `--reportRecedingPushes` | bool | `false` | added 2026-10-04 | M.9: one line counting L.1.5's RECEDING eager push, summed over every radio: the handshakes that took the RECEDING branch and the push frames transmitted (the redundant class: up to 3 frames a push). A report only (G75) | needs `--mobilityFromRange`, else FATAL |
| `--stampBinaryHash` | bool | `false` | added 2026-10-04 | M.3: `run_id.txt` gains `binary sha256 <hash> <path>` for `/proc/self/exe`, after the `source` line, so the binary that ran is named whatever the working directory. Nothing else changes (G76) | — |
| `--subjectTier1Tdoa` | bool | `false` | added 2026-10-04 | Row 55 / MS-1 step 2: each measured subject runs its own instance of tag 0's tier-1 TDoA: gateways stamp the subject's 868 MHz broadcast, a tier-1 fix set of ≥ 3 ranges is solved and fused by `Step()` beside the ranges, on tag 0's stream constants + 97 × the subject index. Writes `subjtdoa_<arm>_seed<N>_subj<id>.csv` (digested) and two TDoA lines per subject after tag 0's (G77). In `--specProfile` since 2026-10-04; a preset-applied value is unused, not refused, when `--tier1Tdoa` is turned off explicitly | needs `--tier1Tdoa` and `--measuredSubjects` > 1, else FATAL |
| `--subjectNlosClassifier` | bool | `false` | added 2026-10-04 | Row 55 / MS-1 step 2: each measured subject runs its own instance of tag 0's NLOS classifier (its own 868 MHz windows, 2.4 GHz pool and UWB CIR stream, + 97 × the subject index), scoring the links its ranging decodes; its policy reads the scores for PCS term 3 and integrity eligibility. Writes `subjnlos_links_`/`subjnlos_summary_<arm>_seed<N>_subj<id>.csv` (digested) and the classifier lines per subject after tag 0's (G78). In `--specProfile` since 2026-10-04; a preset-applied value is unused, not refused, when `--nlosClassifier` is turned off explicitly | needs `--nlosClassifier` and `--measuredSubjects` > 1, else FATAL |
| `--subjectIntegrityWitness` | bool | `false` | added 2026-10-04 | Row 55 / Physical 2.6: before its `Step()` each measured subject computes tag 0's two switching-point witnesses for itself (DC-9's model witness from its own IMU track, and the neighbours' corroboration from its last exchange) and hands them to its own policy; by default no subject's escalation is vetted by either. Writes `subjwitness_<arm>_seed<N>_subj<id>.csv` (digested) and one line per subject (G79). In `--specProfile` since 2026-10-04; a preset-applied value is unused, not refused, when its prerequisites are turned off explicitly | needs `--measuredSubjects` > 1 and `--corroborationCheck` or `--imuCrossCheck`, the latter with `--imuWitnessSource=model`, else FATAL |
| `--toaTable` | bool | `false` | added 2026-10-04 | MAC §2: writes `toa_table.csv` (digested), one row per message type × SF 7–12 × 125/250/500 kHz, with the frame size (9-byte header + `CaTblPayloadBytes` for one entry) and the ToA from the MAC's own function (Semtech formula, CR 4/5, 8-symbol preamble, explicit header, CRC, low-data-rate above 16 ms symbols). Flag-dependent extras are not included. Nothing else changes (G80) | — |
| `--requestOverMesh` | bool | `false` | added 2026-10-04 | N.10 (register row 52): a requester's escalation request reaches its arbiter as a frame. When the base station's arbiter is a finder, the gateway that heard it most recently on 2.4 GHz sends it an on-behalf ESC_REQUEST (a gateway arbiter stays on the backhaul); a finder's named request leaves its own 2.4 GHz radio. Billed to the sender, lossy, ACKed and retried, scored only on reception; with no carrier that hears the arbiter it is unheard and not sent (one hop, OURS). Writes `reqcarry_<arm>_seed<N>.csv` (digested) and one N.10 line (G81). In `--specProfile` since 2026-10-04 (row 52); a preset-applied value is unused, not refused, when `--requesterEscalation` is turned off explicitly | needs `--requesterEscalation`, else FATAL |
| `--stampBuildCommit` | bool | `false` | added 2026-10-04 | M.3: `run_id.txt` gains `build source <commit> dirty <n>`, stamped into the binary at build time by `scratch/catbl_build_stamp.cmake` with the `source` line's own rule, so a sidecar names the tree that compiled the binary whatever directory it ran from (`unknown dirty -1` if the build tree was not a git checkout). Nothing else changes (G82) | — |
| `--collisionBothLost` | bool | `false` | added 2026-10-04 | N.22: the channel keeps a completed frame while a frame on air overlaps it and judges the later-ending frame against it by the capture rule, so both colliding frames can be lost; each such loss is an events_ row `collision_late` with `;by=<sender>:<packet id>`. Changes every channel: no magnitude compares across it (rule 8). One N.22 line (G83). In `--specProfile` since 2026-10-04 | — |
| `--halfDuplexChip` | bool | `false` | added 2026-10-04 | N.6 per chip: under `--halfDuplex` 1 or 2 and `--radioBasis=lr1121`, each node's 868 MHz and 2.4 GHz MACs are one LR1121, so the half-duplex rules read both bands (a frame due while the other band transmits is counted or waits; a reception overlapping the other band's transmission is counted or lost). UWB stays a separate chip. Not modelled: the LR1121 listens on one band at a time. One N.6 chip line (G84). In `--specProfile` since 2026-10-04; a preset-applied value is unused, not refused, when `--halfDuplex=0` or `--radioBasis=sx` is given | needs `--halfDuplex` 1 or 2 and `--radioBasis=lr1121`, else FATAL |
| `--fuseFloorShort` | bool | `false` | added 2026-10-04 | N.14 / FUSE-02: narrows `--fuseFloorActive` to cycles whose floor is short too, so a union is formed only when the floor has fewer than three ranges, the case that carried FUSE-01's availability gain; a floor of three or more is the old floor-first path. A registered campaign (FUSE-02) decides it (G85) | needs `--fuseFloorActive`, else FATAL |
| `--reportAirtime` | bool | `false` | added 2026-10-04 | MAC §2 / Transport §7: writes `airtime_<arm>_seed<N>.csv` (digested), every MAC's own frames sent, relays and airtime per tier and message type, and a FULL_PUSH_CF row per tier: for each DIGEST transmitted, the airtime of pushing the sender's whole valid table instead (a counterfactual). One AIRTIME line with the channel's own frame count beside the MACs'. Nothing else changes (G86). Flooding against reverse path is a paired run with `--reversePath=false`; under `--specProfile` a preset-applied `--reversePathHop` is unused there, not refused | — |
| `--pcsTerm1Inverse` | bool | `false` | added 2026-10-08 | Register row 17 (Phys 2.5): PCS term 1 is the spec's literal 1/trace(Σ), unbounded, in place of the bounded monotone stand-in `traceRef/(traceRef+trace)`. Off by default and off in the preset: every threshold and calibration so far was made with the bounded form, so switching is a re-calibration | FATAL with `--pcsForm=v2` (no term 1) |
| `--tofFieldSigmaM` | double | `0` | added 2026-10-08 | Tier-2 (2.4 GHz ToF) ranging sigma floor raised to a MEASURED field level, applied as max(floor, value) to the 3.0 m LOS and 3.6 m NLOS floors; the EKF sees it through each range's sigma. Mueller et al., ICL-GNSS 2021: SD 83.6 m rural (to 1400 m), 45 m suburban (to 500 m). For `report/PREREG_TOF_FIELD.md` (G120) | — |
| `--tofFieldBiasM` | double | `0` | added 2026-10-08 | Added to every tier-2 range, LOS or NLOS, with no random draw (G120 shows no draw moves); the EKF is not told. Mueller et al. 2021: mean +46.4 m rural, +39 m suburban | — |
| `--traceCovariance` | bool | `false` | added 2026-10-08 | Appends tag 0's posterior `pxx,pxy,pyy` (m²) to every trace row, for NEES; every other cell and file is unchanged (G120 COV), and pxx + pyy = traceSigma (G120 TRACE) | — |
| `--imuQuScale` | double | `1` | added 2026-10-09 | Multiplies the IMU increment covariance handed to every filter (tag 0 and subjects, every arm). A stand-in for the correlation the filter ignores: the dead-reckoner's bias and scale error persist for a run, the EKF adds increments as independent; measured 3-54x too small in variance (S2). 1 reproduces every earlier result (G100) | — |
| `--unknownStart` | bool | `false` | added 2026-10-09 | Seeds every field-node filter (tag 0, subjects, relay peers) at the search box's centre with sigma boxHalf/sqrt(3), never from its truth. Off: tag 0 at its base camp, subjects at their truth, as every earlier result (G130) | — |
| `--traceSubjectConsistency` | bool | `false` | added 2026-10-09 | Appends pxx,pxy,pyy, the raw fix's 2-DOF NIS (ca_tbl_full only) and meanSigmaM to every subject trace row. Writes only; off keeps the old columns (G130) | — |
| `--measVarScale` | double | `1` | added 2026-10-09 | Multiplies every range variance a filter's Step() fuses, every node and arm. An empirical probe of over-trusted ranges, not a physical constant. 1 multiplies exactly (G130) | — |
| `--nlosRSource` | string | `operative` | added 2026-10-09 | Which NLOS tag keys the filter's R inflation and LOS count in Step(): operative (default; under the preset the propagation truth), detector (the classifier's verdict only, so no truth reaches R) or none. Off reproduces every earlier result (G131) | — |
| `--acquireMinAnchors` | uint32 | `0` | added 2026-10-09 | Acquisition rule for --unknownStart: no two-range or one-range update until the filter first fuses a cycle of at least this many ranges; fewer coast. 0 is off and reproduces every earlier result (G132) | — |
| `--imuFilter` | string: `cv`, `ekf5` | `cv` | added 2026-10-17 | With `--imuPredict`, the policy's filter. `cv`: the constant-velocity filter with DC-1's displacement as a control input and no heading, bias or scale state. `ekf5`: the 5-state [x, y, theta, b, s] `CaTblImuEkf` on the same IMU output, learning heading offset, bias and stride scale from fixes; assumed model = DC-1's parameters; variance-only moment-matched coasting; `--acquireParticles`, `--acquireReinit` and `--relayNisInflate` act on it. Every node with an IMU. Refused without `--imuPredict`, with `--rangeBiasStates`, and for any other value | Null control against a89aed8d: 13/14 byte-identical, NC6 4.1-class rounding in estXhp/estYhp only. G95 |
| `--ekf5Robust` | int bit set, 0-15 | `0` | added 2026-10-18 | With `--imuFilter=ekf5`, the 5-state filter's update mode (TECH_DEBT N.33). 0: the update every ekf5 result used. 8 CONSIDER: a fix of fewer than 3 ranges is a Schmidt update -- REJECTED by EKF5-CONSIDER (`report/EKF5_CONSIDER_RESULTS.md`). 1 GATE, 2 PROTECT, 4 RECOVER: diagnostic modes the validator's robustness study rejected. Kept for those studies, not for use. Refused without ekf5 or outside 1-15 | Null control 14/14 on its own base (`61d2f89a`). G96 |
| `--imuStrideSigmaPct` | double, percent | `0` | added 2026-10-18 | With `--imuPredict`: per-step random stride-length error, relative, independent between steps (steps of 0.7 m, ours); 0 keeps DC-1's exact stride. Jahn et al. 2010; Harle 2013. DC-1's Q_u and ekf5's stride noise follow it. Refused without `--imuPredict` or negative | Null control 13/14 + NC6 4.1-class against a89aed8d. G97 |
| `--acquireHandoverCurvature` | bool | `false` | added 2026-10-18 | With `--acquireParticles`: the cloud hands over only once l1 <= 2 r sigma for every range in the set (linearising each range at the cloud's spread costs no more than its own sigma), as well as the median-range guard. Refused without `--acquireParticles` | Null control as `--imuStrideSigmaPct`. G98 |
| `--imuMagSigmaDeg` (+ `--imuMagGmSigmaDeg`, `--imuMagGmTauS`) | double, deg (deg, s) | `0` (`3`, `120`) | added 2026-10-18 | With `--imuPredict` and `--imuFilter=ekf5`: a magnetometer heading reference, white noise per reading plus a Gauss-Markov disturbance (sigma, tau ours), fused by ekf5 as a theta reading at the independent-equivalent variance. 0 keeps gyro-only heading. Afzal, Renaudin, Lachapelle 2011. Refused without ekf5 or negative | Null control 13/14 + NC6 4.1-class against a89aed8d. G99 |
| `--ekf5FinderConsider` | bool | `false` | added 2026-10-18 | With `--imuFilter=ekf5`: each finder's GNSS offset (variance `--finderPosSigma`^2, or 4 m^2) is a Schmidt consider parameter -- covariance and cross-covariance with the state carried, not estimated; the range's R leaves the GNSS part out. Refused without ekf5 or with `--ekf5Robust` | Null control 13/14 + NC6 4.1-class against a89aed8d. G100 |
| `--ekf5RelayGate` | bool | `false` | added 2026-10-18 | With `--imuFilter=ekf5`: a relayed range whose NIS against the prior exceeds chi^2_1(0.999) = 10.83 is not fused; the filter's own ranges are never gated. Refused without ekf5 or with `--ekf5Robust` | As `--ekf5FinderConsider`. G101 |

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
- 2026-09-30, DC-7: `--suspectRecovery` said `none` is "RevertToPrior with no recovery";
  RevertToPrior is a no-op there, so a rejected cycle is a plain coast, and it now says
  so. `--suspectInflateFactor` now says what it does under `downweight`.

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
| 12 | `--boxHalf` | `340` |
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
| 31 | `--reversePathHop` | `false` |
| 32 | `--grantGatedUwb` | `false` |
| 33 | `--queryV2` | `false` |
| 34 | `--queryTriggeredEscalation` | `false` |
| 35 | `--identityQueryUrgent` | `false` |
| 36 | `--arbitrationV2` | `false` |
| 37 | `--sessionOverMesh` | `false` |
| 38 | `--finderSessions` | `false` |
| 39 | `--requesterEscalation` | `false` |
| 40 | `--finderCancelM` | `2` |
| 41 | `--sessionPreciseOnGrant` | `false` |
| 42 | `--sessionStalenessS` | `120` |
| 43 | `--sessionTimeoutS` | `600` |
| 44 | `--finderRequesterEscalation` | `false` |
| 45 | `--requestOverMesh` | `false` |
| 46 | `--fuseFloorActive` | `false` |
| 47 | `--fuseFloorShort` | `false` |
| 48 | `--dualBase868OnNeed` | `false` |
| 49 | `--rssiCorrInflate` | `false` |
| 50 | `--relayNisInflate` | `false` |
| 51 | `--acquireNegInfo` | `false` |
| 52 | `--acquireHandoverCurvature` | `false` |
| 53 | `--tofRecal` | `false` |
| 54 | `--hold24AfterK` | `0` |
| 55 | `--onDemandCalibrate` | `false` |
| 56 | `--onDemandExplore` | `0.1` |
| 57 | `--dualBaseOnDemand` | `false` |
| 58 | `--onDemandR50M` | `180` |
| 59 | `--onDemandSigmaDb` | `7` |
| 60 | `--onDemandN` | `2.3` |
| 61 | `--onDemandPSkip` | `0.05` |
| 62 | `--loraDualBase` | `false` |
| 63 | `--uwbCrossTierPoll` | `false` |
| 64 | `--uwbCrossTierRadius` | `40` |
| 65 | `--integrityChi2` | `9.21` |
| 66 | `--pcsTerm1Inverse` | `false` |
| 67 | `--rssiSigmaLogNormal` | `false` |
| 68 | `--rangeErrorNoLinkLeak` | `false` |
| 69 | `--agcPerReceiver` | `false` |
| 70 | `--tofCalibrate` | `false` |
| 71 | `--finderEnergyReport` | `false` |
| 72 | `--foliageP833` | `false` |
| 73 | `--probeMonotoneTime` | `false` |
| 74 | `--fadingPerBand` | `false` |
| 75 | `--traceOnDemand` | `false` |
| 76 | `--probeTerms` | `false` |
| 77 | `--nlos3gppRMa` | `false` |
| 78 | `--nlosRangeBias` | `false` |
| 79 | `--rssiRangeFromShadowing` | `false` |
| 80 | `--tofErrorModel` | `engine` |
| 81 | `--tofFieldSigmaM` | `0` |
| 82 | `--tofFieldBiasM` | `0` |
| 83 | `--traceRanges` | `false` |
| 84 | `--traceCovariance` | `false` |
| 85 | `--imuQuScale` | `1` |
| 86 | `--unknownStart` | `false` |
| 87 | `--traceSubjectConsistency` | `false` |
| 88 | `--measVarScale` | `1` |
| 89 | `--nlosRSource` | `operative` |
| 90 | `--acquireReinit` | `false` |
| 91 | `--c2bEscalation` | `false` |
| 92 | `--rangeReplyPosition` | `false` |
| 93 | `--acquireParticles` | `false` |
| 94 | `--rangeBiasStates` | `false` |
| 95 | `--acquireMinAnchors` | `0` |
| 96 | `--dualBaseRelayTier` | `false` |
| 97 | `--subjectPreciseOnly` | `false` |
| 98 | `--tdoaInnovationGate` | `false` |
| 99 | `--subjectRelayChain` | `false` |
| 100 | `--hopProvenance` | `false` |
| 101 | `--hopVarianceTerms` | `false` |
| 102 | `--fixRoutingSubjects` | `false` |
| 103 | `--specProfile` | `false` |
| 104 | `--caseCRelative` | `false` |
| 105 | `--routeAlpha` | `1` |
| 106 | `--routeBeta` | `1` |
| 107 | `--routeGamma` | `1` |
| 108 | `--routeSlack` | `1` |
| 109 | `--arbSlotHoldS` | `10` |
| 110 | `--gossipQueue` | `false` |
| 111 | `--identityQueryManifests` | (empty) |
| 112 | `--imuIdeal` | `false` |
| 113 | `--imuFusion` | `false` |
| 114 | `--shsTrustedFixK` | `0` |
| 115 | `--maxTurnRate` | `0` |
| 116 | `--subjectKeepOutM` | `0` |
| 117 | `--uwbTriad` | `10` |
| 118 | `--t2Triad` | `150` |
| 119 | `--uwbAnchors` | `5` |
| 120 | `--t2Anchors` | `3` |
| 121 | `--speed` | `1.2` |
| 122 | `--dwell` | `90` |
| 123 | `--vegWide` | `3` |
| 124 | `--vegFacility` | `255` |
| 125 | `--beacon868` | `10` |
| 126 | `--beacon2p4` | `2` |
| 127 | `--beaconUwb` | `2` |
| 128 | `--nbrTtlMult` | `3` |
| 129 | `--fastDemote` | `off` |
| 130 | `--suspectRecovery` | `none` |
| 131 | `--suspectInflateFactor` | `4` |
| 132 | `--suspectRecoveryN` | `5` |
| 133 | `--suspectPcsFactor` | `nan` |
| 134 | `--escalationRequiresEvidence` | `true` |
| 135 | `--escalationRequiresEvidenceT2` | `false` |
| 136 | `--t2PredictRadius` | `156` |
| 137 | `--t2PredictMinAnchors` | `3` |
| 138 | `--uwbEvidenceFromNeighbours` | `false` |
| 139 | `--mahalUseR` | `false` |
| 140 | `--gwGrid` | `3` |
| 141 | `--multiStepDemote` | `off` |
| 142 | `--demoteStalenessS` | `60` |
| 143 | `--searchDuty` | `1` |
| 144 | `--arms` | `all` |
| 145 | `--injectRangeBias` | `0` |
| 146 | `--injectFaultAnchors` | `0` |
| 147 | `--injectBiasFraction` | `0` |
| 148 | `--floorOnDemand` | `false` |
| 149 | `--floorSkipK` | `1` |
| 150 | `--floorHeartbeatN` | `0` |
| 151 | `--fixedVarThreshold` | `-1` |
| 152 | `--pcsSigmaTargetM` | `-1` |
| 153 | `--deferEscalationOnDutyLimit` | `true` |
| 154 | `--arbitrateDutyBudget` | `true` |
| 155 | `--pcsForm` | `legacy` |
| 156 | `--pcsW1` | `-1` |
| 157 | `--pcsW2` | `-1` |
| 158 | `--pcsW3` | `-1` |
| 159 | `--pcsGateFloor` | `-1` |
| 160 | `--pcsHysteresisBand` | `-1` |
| 161 | `--pcsThrUp868` | `nan` |
| 162 | `--pcsThrUp2p4` | `nan` |
| 163 | `--pcsThrDown2p4` | `nan` |
| 164 | `--pcsThrUpUwb` | `nan` |
| 165 | `--pcsThrDownUwb` | `nan` |
| 166 | `--nDeescalate` | `0` |
| 167 | `--sendHandshakeFrames` | `false` |
| 168 | `--foliageDepthClamp` | `false` |
| 169 | `--rngKeyed` | `false` |
| 170 | `--rngKeyedProbeExtraTx` | `false` |
| 171 | `--terrainPerTierFreq` | `false` |
| 172 | `--intraCycleHandshakeDefer` | `false` |
| 173 | `--gwRidge` | `false` |
| 174 | `--gwArcDeg` | `360` |
| 175 | `--gwLayout` | `legacy` |
| 176 | `--gwLayoutFrac` | `0.9` |
| 177 | `--gwClusterRadiusM` | `300` |
| 178 | `--gwStarArms` | `3` |
| 179 | `--gwStarSpacingM` | `1500` |
| 180 | `--gwLayoutFile` | (empty) |
| 181 | `--gwArcCentreDeg` | `90` |
| 182 | `--trailDeviateSide` | `-1` |
| 183 | `--gwRidgeY` | `0.55` |
| 184 | `--trailDeviateAtS` | `0` |
| 185 | `--trailStopAfterS` | `0` |
| 186 | `--vizTablePeriodS` | `0` |
| 187 | `--stampBinaryHash` | `false` |
| 188 | `--stampBuildCommit` | `false` |
| 189 | `--toaTable` | `false` |
| 190 | `--digestVizOutputs` | `false` |
| 191 | `--reportRecedingPushes` | `false` |
| 192 | `--vizEvents` | `false` |
| 193 | `--uwbCoverageIncludesFinders` | `false` |
| 194 | `--finderDispersal` | `0` |
| 195 | `--processNoiseStdPos` | `1` |
| 196 | `--processNoiseStdVel` | `0.5` |
| 197 | `--gatewayEdgeServer` | `true` |
| 198 | `--edgeDownlinkTier` | `fix` |
| 199 | `--tier1ChargeReplies` | `true` |
| 200 | `--bidirectionalDigest` | `false` |
| 201 | `--finderPosSigma` | `0` |
| 202 | `--finderPseudoAnchor` | `false` |
| 203 | `--hopProvenanceForm` | `0` |
| 204 | `--hopPenalty` | `0` |
| 205 | `--imuCrossCheck` | `false` |
| 206 | `--imuDriftMps` | `0.15` |
| 207 | `--imuConsistencyK` | `3` |
| 208 | `--imuSigmaFloorM` | `5` |
| 209 | `--corroborationCheck` | `false` |
| 210 | `--corroborationK` | `3` |
| 211 | `--corroborationRangeSigmaM` | `30` |
| 212 | `--finderDigestPeriodS` | `30` |
| 213 | `--finderSpeed` | `-1` |
| 214 | `--searchLaunchOnLoss` | `0` |
| 215 | `--searchLaunchAtS` | `0` |
| 216 | `--searchLaunchSigmaM` | `0` |
| 217 | `--finderStaleSteerBound` | `0` |
| 218 | `--finderMode` | `1` |
| 219 | `--nFinders` | `0` |
| 220 | `--nGateways` | `0` |
| 221 | `--gwRingFrac` | `0.6` |
| 222 | `--topology` | `0` |
| 223 | `--sitingMode` | `0` |
| 224 | `--relayedRanging` | `false` |
| 225 | `--maxRelayDepth` | `1` |
| 226 | `--relayVarBudget` | `0` |
| 227 | `--relayChain` | `false` |
| 228 | `--relaySlots` | `auto` |
| 229 | `--fixRouting` | `false` |
| 230 | `--fixRoutingConfidence` | `false` |
| 231 | `--tier1Tdoa` | `false` |
| 232 | `--tdoaPpsSigmaNs` | `20` |
| 233 | `--nlosClassifier` | `false` |
| 234 | `--pcsNlosClassifier` | `false` |
| 235 | `--nlosScoreThreshold` | `0.5` |
| 236 | `--entryVelocity` | `false` |
| 237 | `--integrityNlosFromScore` | `false` |
| 238 | `--uwbNlosModel` | `legacy` |
| 239 | `--radioBasis` | `sx` |
| 240 | `--sf868` | `0` |
| 241 | `--bw868` | `0` |
| 242 | `--sf2p4` | `0` |
| 243 | `--bw2p4` | `0` |
| 244 | `--t2SensitivityRowKHz` | `0` |
| 245 | `--relayTier` | `1` |
| 246 | `--nTags` | `1` |
| 247 | `--identityQueryPeriodS` | `0` |
| 248 | `--identityQueryTarget` | `2` |
| 249 | `--injectTxPowerDropDb` | `0` |
| 250 | `--injectAntennaLossDb` | `0` |
| 251 | `--measuredSubjects` | `1` |
| 252 | `--finderTasking` | (empty) |
| 253 | `--meshLinks` | (empty) |
| 254 | `--imuWitnessSource` | (empty) |
| 255 | `--entryPrediction` | `false` |
| 256 | `--caseD` | `false` |
| 257 | `--imuPredict` | `false` |
| 258 | `--imuFilter` | `cv` |
| 259 | `--ekf5Robust` | `0` |
| 260 | `--imuGyroBiasSigmaDps` | `0` |
| 261 | `--imuStrideSigmaPct` | `0` |
| 262 | `--imuMagSigmaDeg` | `0` |
| 263 | `--ekf5RelayGate` | `false` |
| 264 | `--ekf5FinderConsider` | `false` |
| 265 | `--imuMagGmSigmaDeg` | `3` |
| 266 | `--imuMagGmTauS` | `120` |
| 267 | `--sparseInStep` | `false` |
| 268 | `--subjectEdgeUplink` | `false` |
| 269 | `--edgeUplinkRanges` | `false` |
| 270 | `--deadBatteryNoDraw` | `false` |
| 271 | `--subjectIntegrityWitness` | `false` |
| 272 | `--subjectNlosClassifier` | `false` |
| 273 | `--subjectTier1Tdoa` | `false` |
| 274 | `--halfDuplex` | `0` |
| 275 | `--reportAirtime` | `false` |
| 276 | `--collisionBothLost` | `false` |
| 277 | `--halfDuplexChip` | `false` |
| 278 | `--silenceStopsRelay` | `false` |
| 279 | `--finderPseudoAnchorStrict` | `false` |
| 280 | `--subjectFinderPseudoAnchor` | `false` |
| 281 | `--subjectBatteryDeplete` | `false` |
| 282 | `--subjectTriggeredUpdates` | `false` |
| 283 | `--triggeredUpdates` | `false` |
| 284 | `--triggeredUpdateK` | `1` |
| 285 | `--handshakeFixes` | `false` |
| 286 | `--queryTierBinding` | (empty) |
| 287 | `--uwbArbitration` | `false` |
| 288 | `--arbBatteryCapJ` | `1000` |
| 289 | `--anchorGrantGossipS` | `0` |
| 290 | `--suspectForcesEscalation` | `true` |
| 291 | `--nlosSource` | `legacy` |
