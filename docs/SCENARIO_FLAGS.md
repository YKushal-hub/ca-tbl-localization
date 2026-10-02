# Scenario flags — `ca_tbl_causal_sim`

**Snapshot of 2026-09-30.** It lists 170 flags. It was generated from the binary built from
`ff4f8bb`'s `src/` and `scratch/`, which later commits up to this file's own leave
unchanged. **Rows have changed since, on 2026-09-30, 2026-10-01 and 2026-10-02, and §3 was regenerated
for each change.** Row numbers below are as of the latest regeneration:

- `--relaySlots` (row 139 of §3, and its §1 row): its default moved from `false` to
  `auto`.
- `--edgeDownlinkTier` (row 109 of §3, and its §1 row): added with its default, `fix`
  (M.10). Every row after it moved down one.
- `--uwbNlosModel` (row 149 of §3, and its §1 row): added with its default, `legacy`
  (M.2, `report/PREREG_UWB_NLOS.md`). Every row after it moved down one.
- `--pcsThrUp868`, `--pcsThrUp2p4`, `--pcsThrDown2p4`, `--pcsThrUpUwb`, `--pcsThrDownUwb`
  (rows 80–84 of §3, and their §1 row): added with the default `nan`, "not given"
  (`docs/SPEC_CONFORMANCE_PLAN.md` DC-6). Every row after them moved down five.
- `--suspectPcsFactor` (row 52 of §3, and its §1 row): added with the default `nan`,
  "not given" (DC-7). Every row after it moved down one.
- `--integrityNlosFromScore` (row 148 of §3, and its §1 row): added with the default
  `false` (DC-8). Every row after it moved down one.
- `--entryVelocity` (row 147 of §3, and its §1 row): added with the default `false`
  (DC-10). Every row after it moved down one.
- `--queryTierBinding` (row 165 of §3, and its §1 row): added with an empty default,
  which ns-3 prints with no bracket and which means `first` (NW-3). Every row after it
  moved down one.
- `--handshakeFixes` (row 164 of §3, and its §1 row): added with the default `false`
  (MC-1). Every row after it moved down one.
- `--triggeredUpdates` (row 162 of §3, and its §1 row): added with the default `false`
  (MC-2). Every row after it moved down one.
- `--triggeredUpdateK` (row 163 of §3, and its §1 row): added with the default `1`
  (MC-2). Every row after it moved down one.
- `--edgeUplinkRanges` (row 161 of §3, and its §1 row): added with the default `false`
  (SY-1). Every row after it moved down one.
- `--sf868`, `--bw868`, `--sf2p4`, `--bw2p4` (rows 144–147 of §3, and their §1 row): added with the
  default 0, "not given" (SY-3). Every row after them moved down four.
- `--injectTxPowerDropDb`, `--injectAntennaLossDb` (rows 152–153 of §3, and their §1 row): added
  with the default `0` (SY-5). Every row after them moved down two.
- `--gwLayout`, `--gwLayoutFrac`, `--gwClusterRadiusM`, `--gwStarArms`, `--gwStarSpacingM`,
  `--gwLayoutFile` (rows 91–96 of §3, and their §1 row): added with the defaults
  `legacy`, `0.9`, `300`, `3`, `1500` and empty (MS-2). Every row after them moved down six.
- `--sparseInStep` (row 160 of §3, and its §1 row): added with the default `false`
  (DC-11). Every row after it moved down one.

A fresh `--PrintHelp` differs from the `ff4f8bb` one in those rows only, and adds the
twenty-eight names. Three flags changed meaning without changing their printed default, and §1
says how: `--pcsForm` gained the value `calibrated`, `--pcsW1..3` take 0, and
`--suspectRecovery` gained the value `downweight`.

**Why this file exists.** No tracked document listed the scenario's flags.
`HANDOFF_2026_09_09.md` §8 asked for `--gwArcDeg`, `--gwArcCentreDeg`, `--trailDeviateSide`
and `--searchLaunchAtS` to get "a line in whichever document lists scenario flags", and
there was no such document. Those four are rows 90, 97, 98 and 126 of §3.

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
| 52 | `--suspectPcsFactor` | `nan` |
| 53 | `--escalationRequiresEvidence` | `true` |
| 54 | `--escalationRequiresEvidenceT2` | `false` |
| 55 | `--t2PredictRadius` | `156` |
| 56 | `--t2PredictMinAnchors` | `3` |
| 57 | `--uwbEvidenceFromNeighbours` | `false` |
| 58 | `--mahalUseR` | `false` |
| 59 | `--gwGrid` | `3` |
| 60 | `--multiStepDemote` | `off` |
| 61 | `--demoteStalenessS` | `60` |
| 62 | `--searchDuty` | `1` |
| 63 | `--arms` | `all` |
| 64 | `--injectRangeBias` | `0` |
| 65 | `--injectFaultAnchors` | `0` |
| 66 | `--injectBiasFraction` | `0` |
| 67 | `--floorOnDemand` | `false` |
| 68 | `--floorSkipK` | `1` |
| 69 | `--floorHeartbeatN` | `0` |
| 70 | `--fixedVarThreshold` | `-1` |
| 71 | `--pcsSigmaTargetM` | `-1` |
| 72 | `--deferEscalationOnDutyLimit` | `true` |
| 73 | `--arbitrateDutyBudget` | `true` |
| 74 | `--pcsForm` | `legacy` |
| 75 | `--pcsW1` | `-1` |
| 76 | `--pcsW2` | `-1` |
| 77 | `--pcsW3` | `-1` |
| 78 | `--pcsGateFloor` | `-1` |
| 79 | `--pcsHysteresisBand` | `-1` |
| 80 | `--pcsThrUp868` | `nan` |
| 81 | `--pcsThrUp2p4` | `nan` |
| 82 | `--pcsThrDown2p4` | `nan` |
| 83 | `--pcsThrUpUwb` | `nan` |
| 84 | `--pcsThrDownUwb` | `nan` |
| 85 | `--nDeescalate` | `0` |
| 86 | `--sendHandshakeFrames` | `false` |
| 87 | `--terrainPerTierFreq` | `false` |
| 88 | `--intraCycleHandshakeDefer` | `false` |
| 89 | `--gwRidge` | `false` |
| 90 | `--gwArcDeg` | `360` |
| 91 | `--gwLayout` | `legacy` |
| 92 | `--gwLayoutFrac` | `0.9` |
| 93 | `--gwClusterRadiusM` | `300` |
| 94 | `--gwStarArms` | `3` |
| 95 | `--gwStarSpacingM` | `1500` |
| 96 | `--gwLayoutFile` | (empty) |
| 97 | `--gwArcCentreDeg` | `90` |
| 98 | `--trailDeviateSide` | `-1` |
| 99 | `--gwRidgeY` | `0.55` |
| 100 | `--trailDeviateAtS` | `0` |
| 101 | `--trailStopAfterS` | `0` |
| 102 | `--vizTablePeriodS` | `0` |
| 103 | `--vizEvents` | `false` |
| 104 | `--uwbCoverageIncludesFinders` | `false` |
| 105 | `--finderDispersal` | `0` |
| 106 | `--processNoiseStdPos` | `1` |
| 107 | `--processNoiseStdVel` | `0.5` |
| 108 | `--gatewayEdgeServer` | `true` |
| 109 | `--edgeDownlinkTier` | `fix` |
| 110 | `--tier1ChargeReplies` | `true` |
| 111 | `--bidirectionalDigest` | `false` |
| 112 | `--finderPosSigma` | `0` |
| 113 | `--finderPseudoAnchor` | `false` |
| 114 | `--hopProvenanceForm` | `0` |
| 115 | `--hopPenalty` | `0` |
| 116 | `--imuCrossCheck` | `false` |
| 117 | `--imuDriftMps` | `0.15` |
| 118 | `--imuConsistencyK` | `3` |
| 119 | `--imuSigmaFloorM` | `5` |
| 120 | `--corroborationCheck` | `false` |
| 121 | `--corroborationK` | `3` |
| 122 | `--corroborationRangeSigmaM` | `30` |
| 123 | `--finderDigestPeriodS` | `30` |
| 124 | `--finderSpeed` | `-1` |
| 125 | `--searchLaunchOnLoss` | `0` |
| 126 | `--searchLaunchAtS` | `0` |
| 127 | `--searchLaunchSigmaM` | `0` |
| 128 | `--finderStaleSteerBound` | `0` |
| 129 | `--finderMode` | `1` |
| 130 | `--nFinders` | `0` |
| 131 | `--nGateways` | `0` |
| 132 | `--gwRingFrac` | `0.6` |
| 133 | `--topology` | `0` |
| 134 | `--sitingMode` | `0` |
| 135 | `--relayedRanging` | `false` |
| 136 | `--maxRelayDepth` | `1` |
| 137 | `--relayVarBudget` | `0` |
| 138 | `--relayChain` | `false` |
| 139 | `--relaySlots` | `auto` |
| 140 | `--fixRouting` | `false` |
| 141 | `--fixRoutingConfidence` | `false` |
| 142 | `--tier1Tdoa` | `false` |
| 143 | `--tdoaPpsSigmaNs` | `20` |
| 144 | `--nlosClassifier` | `false` |
| 145 | `--pcsNlosClassifier` | `false` |
| 146 | `--nlosScoreThreshold` | `0.5` |
| 147 | `--entryVelocity` | `false` |
| 148 | `--integrityNlosFromScore` | `false` |
| 149 | `--uwbNlosModel` | `legacy` |
| 150 | `--sf868` | `0` |
| 151 | `--bw868` | `0` |
| 152 | `--sf2p4` | `0` |
| 153 | `--bw2p4` | `0` |
| 154 | `--relayTier` | `1` |
| 155 | `--nTags` | `1` |
| 156 | `--identityQueryPeriodS` | `0` |
| 157 | `--identityQueryTarget` | `2` |
| 158 | `--injectTxPowerDropDb` | `0` |
| 159 | `--injectAntennaLossDb` | `0` |
| 160 | `--sparseInStep` | `false` |
| 161 | `--edgeUplinkRanges` | `false` |
| 162 | `--triggeredUpdates` | `false` |
| 163 | `--triggeredUpdateK` | `1` |
| 164 | `--handshakeFixes` | `false` |
| 165 | `--queryTierBinding` | (empty) |
| 166 | `--uwbArbitration` | `false` |
| 167 | `--arbBatteryCapJ` | `1000` |
| 168 | `--anchorGrantGossipS` | `0` |
| 169 | `--suspectForcesEscalation` | `true` |
| 170 | `--nlosSource` | `legacy` |
