# Spec conformance register — `finalyearprojectcontext.docx`

**Started 2026-09-30.** One row per checkable mechanism in the spec
(`~/catbl_work/finalyearprojectcontext.docx`, an exported design conversation: physical layer
§1–§2, MAC, network, transport/application). Hardware items (nRF52840, Zephyr, the campus
testbed phases) are out of scope for a simulator and are not rows. Evaluation metrics are not
rows either; they belong to the campaigns.

This is the register TECH_DEBT §L's **Bar 2** is judged against: *the running simulator does
what each section describes, or the divergence is recorded with a reason.* It is a dated
snapshot like every other status document here (RULES.md §8). Check the cited code and the
`run_id.txt` sidecars before trusting a row.

**Status words**, used strictly:

| status | meaning |
|---|---|
| **CONFORMANT** | built as specified and **on by default** |
| **OPT-IN** | built as specified, behind a default-off flag. It becomes part of the spec configuration through `--specProfile`, once that exists (plan §3.1) |
| **PARTIAL** | built, but part of the specified behaviour is missing, or the mechanism has no population where it should act |
| **DIVERGENT** | built differently from the spec. The reason is recorded, and whether it is to be closed or kept is stated |
| **ABSENT** | not built |

The **plan** column names the item of `docs/SPEC_CONFORMANCE_PLAN.md` that closes the row;
"—" means nothing is planned, and the row says why.

## 1. The register

### Physical layer

| # | mechanism (docx §) | status | evidence | plan |
|---|---|---|---|---|
| 1 | Tier-1 868 MHz LoRa CSS link (Phys 1.1) | CONFORMANT | `LoRaPhysicsEngine`; r80 1515 m at the default SF7/125 kHz. SF and BW are configurable since 2026-10-02 (SY-3, G36): `--sf868` SF7-SF12, `--bw868` 125/250/500 kHz, and the tier-2 pair `--sf2p4`/`--bw2p4`. A non-default bandwidth shifts the engine's sensitivity table by 10 log10(BW/BW0), read back per arm | ~~SY-3~~ done |
| 2 | Tier-1 RSSI log-distance ranging (Phys 1.1) | CONFORMANT | `lora-ranging-submodule.cc` | — |
| 3 | Tier-1 TDoA, Chan/Taylor, combined with RSSI (Phys 1.1) | OPT-IN | `--tier1Tdoa`, `7377ed0`; J.2 item 4 (σ) is not done | PH-1 |
| 4 | Tier-2 symmetric two-way ToF (Phys 1.2) | CONFORMANT | `LoRa2p4PhysicsEngine`, floor 3.0 m (Zhou et al.) | — |
| 5 | Tier-3 UWB three-message TWR (Phys 1.3) | CONFORMANT | `LoRaUWBPhysicsEngine`, floor 0.10 m (Benouakta et al.) | — |
| 6 | UWB CIR leading-edge NLOS score (Phys 1.3) | PARTIAL | `--nlosClassifier` (`86e43f2`); the UWB matrix is empty under legacy physics (M.2). The registered 802.15.4a model (`--uwbNlosModel=ieee802154a`, `57225b8`) **does not** fill it: P1 FALSIFIED (`report/UWB_NLOS_RESULTS.md`) | PH-2 |
| 7 | Radio basis LR1121 (868 + 2.4 GHz) + DW3000 (hardware) | DIVERGENT | The energy model uses SX1262/SX1280 currents (PROV-03) | SY-2 |
| 8 | Energy metered per action (hardware: fuel gauge) | CONFORMANT | `CaTblEnergyModel`, reconciled to 10 s.f. | — |

### Filter, sparse cases, confidence, integrity

| # | mechanism (docx §) | status | evidence | plan |
|---|---|---|---|---|
| 9 | Table entry `{pos, Σ, t, seq, hop, source_type, velocity}` (Phys 2.1) | OPT-IN | `--entryVelocity` (DC-10, gate G31): a field node publishes its EKF velocity, a finder its GNSS velocity (displacement over time), a gateway 0; held entries carry it intact. The default still publishes 0,0. Nothing reads it yet: NW-9 (RESPONSE), DC-13 (entry prediction) and TA-1 (sessions) will | ~~DC-10~~ done; preset |
| 10 | EKF prediction with IMU control input u_k (Phys 2.2) | OPT-IN | `--imuPredict` (DC-1, 2026-10-02, G40): a synthetic-IMU dead-reckoning error model (MPU-6050 gyro, Rudyk 2020; 2.66% scale, OURS) drives `PredictWithControl`, its covariance the accumulated error since the last fused fix; NEES across 20 seeds 2.15, inside the chi^2 band. Recorded deviation: velocity is overwritten with u/dt (a PDR prediction, not a literal F x + B u). By default the prediction is constant velocity; the SHS engine is untouched (K.9) | ~~DC-1~~ done; preset |
| 11 | EKF correction with tier-specific R (Phys 2.3) | CONFORMANT | Per-measurement σ from each engine | — |
| 12 | Sparse cases solved against the propagated prior (Phys 2.4) | OPT-IN | `--sparseInStep` (DC-11, 2026-10-02, G39): Cases A/B solved inside `Step()` after the prediction, against the propagated prior; fused once as a position (never also as ranges, which closes the Case A + relays double count); a solved cycle is a fix and does not count toward demotion. By default `--sparseCaseA` solves before `Step()` against the previous posterior and `Step()` counts the cycle a no-fix | ~~DC-11~~ done; preset |
| 13 | Case A, two-circle + Mahalanobis choice (Phys 2.4) | OPT-IN | `--sparseCaseA` (before `Step()`) or `--sparseInStep` (inside it, DC-11), both `CaTblSolveCaseA`; Case A when both ranges are to surveyed gateways, read since DC-11 from the measurement's `sourceKind` rather than its position | preset |
| 14 | Case B, orientation from IMU heading (Phys 2.4) | OPT-IN | Under `--imuPredict --sparseInStep` (DC-2, 2026-10-02, G42) Step() resolves Case B against the IMU-propagated prior (last posterior + u_k, P + Q_u): the IMU heading carried into position, labelled `sparsePriorSource = 1`; 42 Case B solves over 4 seeds, every prior the previous posterior + u. Without `--imuPredict` the choice uses the constant-velocity prior and the deviation stays open (`ca-tbl-sparse-anchor.h`). The SHS heading is still not used (K.9) | ~~DC-2~~ done; preset |
| 15 | Case C, relative shape only (Phys 2.4) | PARTIAL | Classified as unlocalisable; no relative tracking | DC-3 |
| 16 | Case D, one range + IMU (Phys 2.4D) | OPT-IN | `--caseD` (DC-4, 2026-10-02, G41; needs `--sparseInStep`, best with `--imuPredict`): a single range is applied as a scalar update on the (IMU-)propagated state when NIS <= 6.63; the cycle is not a position fix but its covariance is live, it reaches the escalation decision (evidence gate still applies) and does not count toward demotion (OURS). NC3 regime, seed 7: 65 Case D cycles, all applied, 30 escalated, 0 demoted. The scenario's old census label "CASE D" means the ordinary 3+ solve | ~~DC-4~~ done; preset |
| 17 | PCS term 1 = 1/trace(Σ) (Phys 2.5) | DIVERGENT (kept) | The bounded transform `traceRef/(traceRef+trace)` (`ca-tbl-policy.cc:857`); monotone equivalent | — |
| 18 | PCS term 3 from the tier-specific NLOS score (Phys 2.5) | OPT-IN | `--pcsNlosClassifier`; the default reads the boolean tag | preset |
| 19 | PCS weights tuned empirically, by LOOCV (Phys 2.5, audit Gap B) | DIVERGENT | Unweighted sum by default. CAL-01 ran and gave (3,0,0), not adopted. Since DC-6 (2026-09-30) the calibrated form is expressible: `--pcsForm=calibrated` with the weights (0 included) and per-tier thresholds from the CLI, gate G28. What is still missing is the calibration itself, CAL-02 | DC-5, ~~DC-6~~ done, CAL-02 |
| 20 | Switching-point integrity: SUSPECT downweights PCS (Phys 2.6) | OPT-IN | `--suspectRecovery=downweight` (DC-7, `d31dd0a`, gate G29) accepts the fix at R x 4 and scales its PCS by 0.5 (OURS), and the fix stays SUSPECT so it cannot count toward a de-escalation. The default still rejects it and forces an escalation (`ca-tbl-policy.cc`, the suspect branch of `Step()`). The spec's form also needs `--suspectForcesEscalation=false`, which the preset will set | ~~DC-7~~ done; preset |
| 21 | Integrity eligibility "NLOS_score LOW" (Phys 2.6) | OPT-IN | `--integrityNlosFromScore` (DC-8, gate G30): the mean NLOS_score of the fix set's scored links is below `--nlosScoreThreshold`, and with none scored the tag decides. The default is still "all links tagged LOS", which has no population under the default tag: 0 of 305 evaluated fixes at seed 7 (I.1). Read from the score, 296 of 297 are eligible and 152 are called suspect -- see I.1 for why that is its own problem | ~~DC-8~~ done; preset |
| 22 | Cross-validation against IMU dead reckoning and neighbours (novelty item 2) | OPT-IN | `--imuCrossCheck`, `--corroborationCheck`. By default the IMU witness uses the diverging SHS against the EKF estimate. `--imuWitnessSource=model` (DC-9, 2026-10-02, G44) compares the synthetic IMU track, anchored at the last fused fix, with the cycle's raw radio fix (independent of the IMU-fed filter), tolerance 3 sqrt(trace R_fix + trace Sigma_acc), and splits its counters: evaluations, refusals, and refusals the evidence gate would also have made (absorbed, never reported as a pass). Withdrawn in the default because the evidence gate absorbs its refusals (B3 C1; G44 shows 1 of 1 absorbed with the gate on) | ~~DC-9~~ done; preset |
| 23 | Sequence-number supersession (Phys 2.7) | OPT-IN | Runs only when the digest exchange does (`--sendHandshakeFrames`, finders) | MC-3 |
| 24 | Covariance Intersection on equal sequence numbers (Phys 2.7) | OPT-IN | `--tableFuseCI`; near-zero population (L.2) | MC-3 |
| 25 | Escalation/de-escalation hysteresis with N-cycle dwell (Phys 2.8) | CONFORMANT | `pcsThresholdUp/Down`, `--nDeescalate` | — |
| 26 | Per-entry prediction with source-type Q (Phys 2.2) | OPT-IN | `--entryPrediction` with `--entryVelocity` (DC-13, 2026-10-02, G43): a held finder entry is used at x + v·age with added variance (0.5 m/s · age)² (OURS, a GNSS-derived velocity); gateways are surveyed and never substituted (Q = 0); 504 substitutions in the converging-finder regime, all carried forward. By default the entry is used where reported with an isotropic walk-speed × age inflation. Field-node (peer) entries arrive with MS-1 | ~~DC-13~~ done; preset |

### MAC / link layer

| # | mechanism (docx §) | status | evidence | plan |
|---|---|---|---|---|
| 27 | WPC G.S.R. 853(E) Table-II duty cycle 10 % / 2.5 % (MAC §1) | CONFORMANT | `duty_868` 0.0226 | — |
| 28 | Adaptive Power Control (MAC, audit Gap A) | CONFORMANT | `CaTblMac::UpdateApc` | — |
| 29 | ToA from the Semtech formula (MAC §2) | CONFORMANT | Matches measured airtime to 5 s.f. | — |
| 30 | CAD CSMA/CA with priority contention windows (MAC §3) | CONFORMANT | Captures emerge (14 564) | — |
| 31 | 2.4 GHz ranging as a directed exchange (MAC §4) | CONFORMANT | Direct engine exchange with a CAD charge | — |
| 32 | UWB randomised-wait initiation (MAC §5) | CONFORMANT | `ca-tbl-mac.cc:909-916` | — |
| 33 | Flood dedup, hop limit, jitter (MAC §6) | CONFORMANT | `CaTblMac` | — |
| 34 | 9-byte header, message taxonomy (MAC §7) | CONFORMANT | Validator 38/38 | — |
| 35 | Selective ACK; escalation handshake on the air (MAC §8) | OPT-IN | With `--sendHandshakeFrames`, `--handshakeFixes` (MC-1, 2026-10-02, G33) gives the specified handshake: an ACK names the frame it acknowledges, an anchor arbitrates first and answers with a GRANT saying granted or denied, a denial is retried on the ACK timeout, the terms are the cycle's own, and only escalations into UWB send a request. By default: frames only with `--sendHandshakeFrames`, the oldest frame is cleared by any ACK, a denied request is ACKed before arbitration, and de-escalations are signalled. The arbiter still holds no queue (NW-5) | ~~MC-1~~ done; preset |
| 36 | Triggered update / last gasp (MAC §3, §8) | OPT-IN | `--triggeredUpdates` (MC-2, 2026-10-02, G34): a LARGE CORRECTION -- the posterior moving more than k sqrt(trace P) from the prediction, k = `--triggeredUpdateK` = 1 (OURS; k = 3 never fires, a Kalman update moves the mean by about one prior sigma) -- pushes the corrected entry with the redundant class's 3 jittered repeats; a LAST GASP goes once, unrepeated, before a flat battery silences the radio. Both billed on the tier that transmitted; counted (M.9). By default only the RECEDING branch sends. A silenced radio still forwards floods (TECH_DEBT N.1) | ~~MC-2~~ done; preset |
| 37 | Mobility-state message selection (MAC §9) | OPT-IN | `--mobilityFromRange` | MC-4 |

### Network layer

| # | mechanism (docx §) | status | evidence | plan |
|---|---|---|---|---|
| 38 | Mesh: any two radios can exchange frames (throughout) | OPT-IN | `--meshLinks=all` (NW-0, 2026-10-03, G45): every registered pair on a tier both carry is a link (tag-tag, gateway-gateway, finder-gateway); DISCOVERY one-hop (it leaves with hop limit 0); the ranging poll addresses anchors only; the node's sleeping radios neither receive nor relay, and its relays are billed. By default links exist only between a field node and an anchor. Multi-hop RESPONSE needs NW-8 | ~~NW-0~~ done; preset |
| 39 | Addressing: node_id, identity class, target manifest id (Net §1) | CONFORMANT | `CaTblHeader`, `CaTblMac` | — |
| 40 | Routing table, ETX baseline (Net §2) | OPT-IN | `--fixRouting`, relays only | NW-2 |
| 41 | Confidence-weighted path selection for position-fix traffic (Net §3) | PARTIAL | `--fixRoutingConfidence`; the measured tag's traffic is never routed; α, β, γ are uncalibrated | NW-2 |
| 42 | Hop provenance reaches confidence and the RESULT (Phys 2.5, Net, Transp §3) | PARTIAL | Σ inflation exists (C5). The hop count reaches `Step` and RESULT as a literal 0; per-hop variance is missing | NW-1 |
| 43 | Identity-scoped QUERY, tier binding (Net §4) | PARTIAL | `--identityQueryPeriodS`. By default every driver is bound to gateway 101's 868 MHz MAC (L.2): G32 shows that the "2.4 GHz" queries were billed at 868 MHz airtime. `--queryTierBinding=per-tier` (NW-3, 2026-10-01, G32) binds each tier's driver to that tier's radio. Still PARTIAL: QUERY carries one manifest and nothing handles the RESPONSE at the requester (NW-9, TA-1) | ~~NW-3~~ done; NW-9 |
| 44 | QUERY/RESPONSE contents; the answering node stops the flood (Net §4) | DIVERGENT | Single manifest, no priority carried, empty RESPONSE, no requester handler; answering nodes still rebroadcast | NW-9 |
| 45 | Proxy response with a staleness bound (Net §4) | OPT-IN | `--queryProxy`; the bound was measured at L.2.5 | preset |
| 46 | Query-triggered UWB escalation (Net §4) | ABSENT | No path from QUERY to escalation | NW-4 |
| 47 | UWB arbitration U(r) with a queue at anchors (Net §5) | DIVERGENT | Battery term is the fraction spent; T_query is constant; no queue; finders never arbitrate | NW-5 |
| 48 | Grant/deny on the ACK'd message; escalation only when granted (MAC §8, Transp §4) | DIVERGENT | Nothing at the tag consumes ESC_GRANT; the tier change commits without a grant | NW-7 |
| 49 | Anchors gossip queue and grants; the lower id defers (Net §6) | PARTIAL | `--anchorGrantGossipS`; grants only, gateways only | NW-6 |

### Transport / application layer

| # | mechanism (docx §) | status | evidence | plan |
|---|---|---|---|---|
| 50 | Reverse-path unicast of RESPONSE, flood fallback (Transp §1) | DIVERGENT | `--reversePath` caches the originator, so it is a no-op | NW-8 |
| 51 | Session state machine fed by RESPONSEs to a requester (Transp §2) | PARTIAL | `--sessionLayer`; fed the tag's own filter; the requester is the tag itself | TA-1, TA-3 |
| 52 | SEARCH_AND_LOCALIZE and BATCH_SEARCH over identified targets (Transp §4–§5) | PARTIAL | API exists; only manifest 1 is fed; spread always UNDEFINED | TA-2, TA-4 |

### System and deployment

| # | mechanism (docx) | status | evidence | plan |
|---|---|---|---|---|
| 53 | Gateways as edge servers (final plan) | OPT-IN | The edge estimator is on by default since `db2ac2e`. `--edgeUplinkRanges` (SY-1, 2026-10-02, G35) bills the frame that carries a tier-2/3 range from the node to the gateways: 9 B + 6 B per answered anchor (OURS) on the tier that ranged. Only energy moves, on tiers 2 and 3. The UWB branch is unexercised: no regime returns a UWB range (DC-6) | ~~SY-1~~ done; preset |
| 54 | GPS-surveyed, PPS-synchronised anchors (audit Q1) | OPT-IN | Under `--tier1Tdoa`, a per-stamp PPS residual on surveyed positions. The audit says GPS lock needs no separate sync protocol, so nothing further is missing | PH-1 |
| 55 | Five localised field nodes (audit Q2) | PARTIAL | `--measuredSubjects=N` (MS-1, 2026-10-03, G46): tags 2..N run the core of tag 0's cycle on their own accounts -- ranging on the active tier, the tier-1 floor, `Step()` with their own escalation state machine, handshakes with their own degradation and battery terms (PeerRequestDriver's constant 128/128 requests retire), the downlink, listen/sleep -- each in its own slot of a round before tag 0's instant; a relay that is a subject is stepped once; `subject_summary.csv` (subject 1 = tag 0's arm_summary row) and `subjtrace_*`. Still tag 0 only: SHS/IMU witness, TDoA, NLOS scorer, sessions, battery depletion, triggered updates, edge uplink, entry prediction (warned at start-up). `--finderTasking=roundrobin` (2026-10-03, G47) tasks finder k to subject (k mod N) + 1 (OURS); a finder that never hears its subject stays blind (TECH_DEBT N.4) | per-subject SHS/TDoA/NLOS/sessions (no item yet); TA-2 |
| 56 | Three gateways, 5 km radius (audit Q2) | PARTIAL | Reachable with `--nGateways`. The shapes GEO-01 compares exist since 2026-10-02 (MS-2, G38): `--gwLayout=perimeter|cluster|star|lshape|file` beside the ring, arc, ridge and grid. GEO-01 stage 1 (the screen, `report/GEO01_STAGE1_RESULTS.md`) pruned 10 candidates for stage 2; tier 1 alone covers mean P(≥3) ≤ 0.71 of the 10 km square at N ≤ 8. Shape and count are chosen in stage 2 (P5) | GEO-01 stages 2-3 |
| 57 | Environmental fault classes for evaluation (exec Phase 5) | OPT-IN | Range bias (`--injectRangeBias`), silence (`--injectSilentId`) and depletion (`--batteryDepleteJ`) existed. Since 2026-10-02 (SY-5, G37) `--injectTxPowerDropDb` (cold-sagged battery) and `--injectAntennaLossDb` (detuned or covered antenna, reciprocal) act over the same seeded window: tier-1 RSSI ranges read long by 10^(dB/(10 n)), and tiers 2/3 lose SNR and links. Magnitudes OURS. A silenced radio still forwards floods (TECH_DEBT N.1) | ~~SY-5~~ done; campaigns |

## 2. Count

57 rows. It is more than the 52 of the first audit (2026-09-30), because rows 12, 26, 38, 44
and 50 were split out by the code explorations behind the plan.

| status | n |
|---|---|
| CONFORMANT | 16 |
| OPT-IN | 23 |
| PARTIAL | 10 |
| DIVERGENT | 7 (one of them, row 17, kept) |
| ABSENT | 1 |

These are parsed from the tables. A first hand count gave 17/11/14/13/2 and was wrong.
They have moved since the first parsed count (16/10/14/15/2), each row DIVERGENT → OPT-IN: row 20 with DC-7, row 21 with DC-8, row 9 with DC-10; row 35 with MC-1 (PARTIAL → OPT-IN); row 36 with MC-2 (PARTIAL → OPT-IN); row 53 with SY-1 (PARTIAL → OPT-IN); row 57 with SY-5 (PARTIAL → OPT-IN); row 12 with DC-11 (DIVERGENT → OPT-IN); row 10 with DC-1 (DIVERGENT → OPT-IN); row 16 with DC-4 (ABSENT → OPT-IN); row 14 with DC-2 (DIVERGENT → OPT-IN); row 26 with DC-13 (DIVERGENT → OPT-IN); row 38 with NW-0 (DIVERGENT → OPT-IN).

**Correction, 2026-10-01.** The DC-7 commit (`d31dd0a`) changed this block to 16/11/14/14/2 but not row 20 itself: the edit to the row was never written to disk. So for one commit the numbers above disagreed with the table they claim to be parsed from. Row 20 is corrected with DC-8, and the block is parsed again, as it says to be.

Recount it from the tables. Do not edit the numbers by hand.
