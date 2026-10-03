# Scenario flags — `ca_tbl_causal_sim`

**Snapshot of 2026-09-30.** It lists 194 flags. It was generated from the binary built from
`ff4f8bb`'s `src/` and `scratch/`, which later commits up to this file's own leave
unchanged. **Rows have changed since, on 2026-09-30, 2026-10-01 and 2026-10-02, and §3 was regenerated
for each change.** Row numbers below are as of the latest regeneration:

- `--relaySlots` (row 156 of §3, and its §1 row): its default moved from `false` to
  `auto`.
- `--edgeDownlinkTier` (row 126 of §3, and its §1 row): added with its default, `fix`
  (M.10). Every row after it moved down one.
- `--uwbNlosModel` (row 166 of §3, and its §1 row): added with its default, `legacy`
  (M.2, `report/PREREG_UWB_NLOS.md`). Every row after it moved down one.
- `--pcsThrUp868`, `--pcsThrUp2p4`, `--pcsThrDown2p4`, `--pcsThrUpUwb`, `--pcsThrDownUwb`
  (rows 97–101 of §3, and their §1 row): added with the default `nan`, "not given"
  (`docs/SPEC_CONFORMANCE_PLAN.md` DC-6). Every row after them moved down five.
- `--suspectPcsFactor` (row 69 of §3, and its §1 row): added with the default `nan`,
  "not given" (DC-7). Every row after it moved down one.
- `--integrityNlosFromScore` (row 165 of §3, and its §1 row): added with the default
  `false` (DC-8). Every row after it moved down one.
- `--entryVelocity` (row 164 of §3, and its §1 row): added with the default `false`
  (DC-10). Every row after it moved down one.
- `--queryTierBinding` (row 189 of §3, and its §1 row): added with an empty default,
  which ns-3 prints with no bracket and which means `first` (NW-3). Every row after it
  moved down one.
- `--handshakeFixes` (row 188 of §3, and its §1 row): added with the default `false`
  (MC-1). Every row after it moved down one.
- `--triggeredUpdates` (row 186 of §3, and its §1 row): added with the default `false`
  (MC-2). Every row after it moved down one.
- `--triggeredUpdateK` (row 187 of §3, and its §1 row): added with the default `1`
  (MC-2). Every row after it moved down one.
- `--edgeUplinkRanges` (row 185 of §3, and its §1 row): added with the default `false`
  (SY-1). Every row after it moved down one.
- `--sf868`, `--bw868`, `--sf2p4`, `--bw2p4` (rows 167–170 of §3, and their §1 row): added with the
  default 0, "not given" (SY-3). Every row after them moved down four.
- `--injectTxPowerDropDb`, `--injectAntennaLossDb` (rows 175–176 of §3, and their §1 row): added
  with the default `0` (SY-5). Every row after them moved down two.
- `--gwLayout`, `--gwLayoutFrac`, `--gwClusterRadiusM`, `--gwStarArms`, `--gwStarSpacingM`,
  `--gwLayoutFile` (rows 108–113 of §3, and their §1 row): added with the defaults
  `legacy`, `0.9`, `300`, `3`, `1500` and empty (MS-2). Every row after them moved down six.
- `--sparseInStep` (row 184 of §3, and its §1 row): added with the default `false`
  (DC-11). Every row after it moved down one.
- `--imuPredict` (row 183 of §3, and its §1 row): added with the default `false`
  (DC-1). Every row after it moved down one.
- `--caseD` (row 182 of §3, and its §1 row): added with the default `false` (DC-4).
  Every row after it moved down one.
- `--entryPrediction` (row 181 of §3, and its §1 row): added with the default `false`
  (DC-13). Every row after it moved down one.
- `--imuWitnessSource` (row 180 of §3, and its §1 row): added with an empty default,
  which means `shs` (DC-9). Every row after it moved down one.
- `--meshLinks` (row 179 of §3, and its §1 row): added with an empty default, which
  means `star` (NW-0). Every row after it moved down one.
- `--measuredSubjects` (row 177 of §3, and its §1 row): added with the default `1`
  (MS-1). Every row after it moved down one.
- `--finderTasking` (row 178 of §3, and its §1 row): added with an empty default,
  which means `tag0` (MS-1). Every row after it moved down one.
- `--reversePathHop` (row 31 of §3, and its §1 row): added with the default `false`
  (NW-8). Every row after it moved down one.
- `--grantGatedUwb` (row 32 of §3, and its §1 row): added with the default `false`
  (NW-7). Every row after it moved down one.
- `--queryV2` and `--identityQueryManifests` (rows 33 and 47 of §3, and their §1 row): added with
  the defaults `false` and empty (NW-9). Every row after them moved down two.
- `--queryTriggeredEscalation`, `--identityQueryUrgent`, `--arbitrationV2` (rows 34–36 of §3, and their §1 row):
  added with the default `false` (NW-4, NW-5). Every row after them moved down three.
- `--arbSlotHoldS` and `--gossipQueue` (rows 45–46 of §3, and their §1 row): added with the
  defaults `10` and `false` (NW-6). Every row after them moved down two.
- `--hopProvenance` (row 44 of §3, and its §1 row): added with the default `false` (NW-1).
  Every row after it moved down one.
- `--sessionOverMesh`, `--sessionStalenessS`, `--sessionTimeoutS` (rows 37, 42 and 43 of §3, and their §1 row):
  added with the defaults `false`, `120` and `600` (TA-1a). Every row after them moved down three.
- `--sessionPreciseOnGrant` (row 41 of §3, and its §1 row): added with the default `false`
  (TA-3). Every row after it moved down one.
- `--finderSessions` and `--finderCancelM` (rows 38 and 40 of §3, and their §1 row): added with the
  defaults `false` and `2` (TA-1b). Every row after them moved down two.
- `--requesterEscalation` (row 39 of §3, and its §1 row): added with the default `false`
  (TA-2). Every row after it moved down one.

A fresh `--PrintHelp` differs from the `ff4f8bb` one in those rows only, and adds the
fifty-two names. Three flags changed meaning without changing their printed default, and §1
says how: `--pcsForm` gained the value `calibrated`, `--pcsW1..3` take 0, and
`--suspectRecovery` gained the value `downweight`.

**Why this file exists.** No tracked document listed the scenario's flags.
`HANDOFF_2026_09_09.md` §8 asked for `--gwArcDeg`, `--gwArcCentreDeg`, `--trailDeviateSide`
and `--searchLaunchAtS` to get "a line in whichever document lists scenario flags", and
there was no such document. Those four are rows 107, 114, 115 and 143 of §3.

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
| 44 | `--hopProvenance` | `false` |
| 45 | `--arbSlotHoldS` | `10` |
| 46 | `--gossipQueue` | `false` |
| 47 | `--identityQueryManifests` | (empty) |
| 48 | `--imuIdeal` | `false` |
| 49 | `--imuFusion` | `false` |
| 50 | `--shsTrustedFixK` | `0` |
| 51 | `--maxTurnRate` | `0` |
| 52 | `--subjectKeepOutM` | `0` |
| 53 | `--uwbTriad` | `10` |
| 54 | `--t2Triad` | `150` |
| 55 | `--uwbAnchors` | `5` |
| 56 | `--t2Anchors` | `3` |
| 57 | `--speed` | `1.2` |
| 58 | `--dwell` | `90` |
| 59 | `--vegWide` | `3` |
| 60 | `--vegFacility` | `255` |
| 61 | `--beacon868` | `10` |
| 62 | `--beacon2p4` | `2` |
| 63 | `--beaconUwb` | `2` |
| 64 | `--nbrTtlMult` | `3` |
| 65 | `--fastDemote` | `off` |
| 66 | `--suspectRecovery` | `none` |
| 67 | `--suspectInflateFactor` | `4` |
| 68 | `--suspectRecoveryN` | `5` |
| 69 | `--suspectPcsFactor` | `nan` |
| 70 | `--escalationRequiresEvidence` | `true` |
| 71 | `--escalationRequiresEvidenceT2` | `false` |
| 72 | `--t2PredictRadius` | `156` |
| 73 | `--t2PredictMinAnchors` | `3` |
| 74 | `--uwbEvidenceFromNeighbours` | `false` |
| 75 | `--mahalUseR` | `false` |
| 76 | `--gwGrid` | `3` |
| 77 | `--multiStepDemote` | `off` |
| 78 | `--demoteStalenessS` | `60` |
| 79 | `--searchDuty` | `1` |
| 80 | `--arms` | `all` |
| 81 | `--injectRangeBias` | `0` |
| 82 | `--injectFaultAnchors` | `0` |
| 83 | `--injectBiasFraction` | `0` |
| 84 | `--floorOnDemand` | `false` |
| 85 | `--floorSkipK` | `1` |
| 86 | `--floorHeartbeatN` | `0` |
| 87 | `--fixedVarThreshold` | `-1` |
| 88 | `--pcsSigmaTargetM` | `-1` |
| 89 | `--deferEscalationOnDutyLimit` | `true` |
| 90 | `--arbitrateDutyBudget` | `true` |
| 91 | `--pcsForm` | `legacy` |
| 92 | `--pcsW1` | `-1` |
| 93 | `--pcsW2` | `-1` |
| 94 | `--pcsW3` | `-1` |
| 95 | `--pcsGateFloor` | `-1` |
| 96 | `--pcsHysteresisBand` | `-1` |
| 97 | `--pcsThrUp868` | `nan` |
| 98 | `--pcsThrUp2p4` | `nan` |
| 99 | `--pcsThrDown2p4` | `nan` |
| 100 | `--pcsThrUpUwb` | `nan` |
| 101 | `--pcsThrDownUwb` | `nan` |
| 102 | `--nDeescalate` | `0` |
| 103 | `--sendHandshakeFrames` | `false` |
| 104 | `--terrainPerTierFreq` | `false` |
| 105 | `--intraCycleHandshakeDefer` | `false` |
| 106 | `--gwRidge` | `false` |
| 107 | `--gwArcDeg` | `360` |
| 108 | `--gwLayout` | `legacy` |
| 109 | `--gwLayoutFrac` | `0.9` |
| 110 | `--gwClusterRadiusM` | `300` |
| 111 | `--gwStarArms` | `3` |
| 112 | `--gwStarSpacingM` | `1500` |
| 113 | `--gwLayoutFile` | (empty) |
| 114 | `--gwArcCentreDeg` | `90` |
| 115 | `--trailDeviateSide` | `-1` |
| 116 | `--gwRidgeY` | `0.55` |
| 117 | `--trailDeviateAtS` | `0` |
| 118 | `--trailStopAfterS` | `0` |
| 119 | `--vizTablePeriodS` | `0` |
| 120 | `--vizEvents` | `false` |
| 121 | `--uwbCoverageIncludesFinders` | `false` |
| 122 | `--finderDispersal` | `0` |
| 123 | `--processNoiseStdPos` | `1` |
| 124 | `--processNoiseStdVel` | `0.5` |
| 125 | `--gatewayEdgeServer` | `true` |
| 126 | `--edgeDownlinkTier` | `fix` |
| 127 | `--tier1ChargeReplies` | `true` |
| 128 | `--bidirectionalDigest` | `false` |
| 129 | `--finderPosSigma` | `0` |
| 130 | `--finderPseudoAnchor` | `false` |
| 131 | `--hopProvenanceForm` | `0` |
| 132 | `--hopPenalty` | `0` |
| 133 | `--imuCrossCheck` | `false` |
| 134 | `--imuDriftMps` | `0.15` |
| 135 | `--imuConsistencyK` | `3` |
| 136 | `--imuSigmaFloorM` | `5` |
| 137 | `--corroborationCheck` | `false` |
| 138 | `--corroborationK` | `3` |
| 139 | `--corroborationRangeSigmaM` | `30` |
| 140 | `--finderDigestPeriodS` | `30` |
| 141 | `--finderSpeed` | `-1` |
| 142 | `--searchLaunchOnLoss` | `0` |
| 143 | `--searchLaunchAtS` | `0` |
| 144 | `--searchLaunchSigmaM` | `0` |
| 145 | `--finderStaleSteerBound` | `0` |
| 146 | `--finderMode` | `1` |
| 147 | `--nFinders` | `0` |
| 148 | `--nGateways` | `0` |
| 149 | `--gwRingFrac` | `0.6` |
| 150 | `--topology` | `0` |
| 151 | `--sitingMode` | `0` |
| 152 | `--relayedRanging` | `false` |
| 153 | `--maxRelayDepth` | `1` |
| 154 | `--relayVarBudget` | `0` |
| 155 | `--relayChain` | `false` |
| 156 | `--relaySlots` | `auto` |
| 157 | `--fixRouting` | `false` |
| 158 | `--fixRoutingConfidence` | `false` |
| 159 | `--tier1Tdoa` | `false` |
| 160 | `--tdoaPpsSigmaNs` | `20` |
| 161 | `--nlosClassifier` | `false` |
| 162 | `--pcsNlosClassifier` | `false` |
| 163 | `--nlosScoreThreshold` | `0.5` |
| 164 | `--entryVelocity` | `false` |
| 165 | `--integrityNlosFromScore` | `false` |
| 166 | `--uwbNlosModel` | `legacy` |
| 167 | `--sf868` | `0` |
| 168 | `--bw868` | `0` |
| 169 | `--sf2p4` | `0` |
| 170 | `--bw2p4` | `0` |
| 171 | `--relayTier` | `1` |
| 172 | `--nTags` | `1` |
| 173 | `--identityQueryPeriodS` | `0` |
| 174 | `--identityQueryTarget` | `2` |
| 175 | `--injectTxPowerDropDb` | `0` |
| 176 | `--injectAntennaLossDb` | `0` |
| 177 | `--measuredSubjects` | `1` |
| 178 | `--finderTasking` | (empty) |
| 179 | `--meshLinks` | (empty) |
| 180 | `--imuWitnessSource` | (empty) |
| 181 | `--entryPrediction` | `false` |
| 182 | `--caseD` | `false` |
| 183 | `--imuPredict` | `false` |
| 184 | `--sparseInStep` | `false` |
| 185 | `--edgeUplinkRanges` | `false` |
| 186 | `--triggeredUpdates` | `false` |
| 187 | `--triggeredUpdateK` | `1` |
| 188 | `--handshakeFixes` | `false` |
| 189 | `--queryTierBinding` | (empty) |
| 190 | `--uwbArbitration` | `false` |
| 191 | `--arbBatteryCapJ` | `1000` |
| 192 | `--anchorGrantGossipS` | `0` |
| 193 | `--suspectForcesEscalation` | `true` |
| 194 | `--nlosSource` | `legacy` |
