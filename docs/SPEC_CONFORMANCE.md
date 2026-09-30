# Spec conformance register — `finalyearprojectcontext.docx`

**Started 2026-09-30.** One row per checkable mechanism in the spec
(`~/catbl_work/finalyearprojectcontext.docx`, an exported design conversation: physical layer
§1–§2, MAC, network, transport/application). Hardware items (nRF52840, Zephyr, the campus
testbed phases) are out of scope for a simulator and are not rows. Evaluation metrics are not
rows either; they belong to the campaigns.

This is the register TECH_DEBT §L's **Bar 2** is judged against: *the running simulator does
what each section describes, or the divergence is recorded with a reason.* It is a dated
snapshot like every other status document here (CLAUDE.md §8). Check the cited code and the
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
| 1 | Tier-1 868 MHz LoRa CSS link (Phys 1.1) | CONFORMANT | `LoRaPhysicsEngine`; r80 1515 m. SF/BW is fixed at SF7/125 kHz, whereas the spec says "configurable" | SY-3 (flag) |
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
| 9 | Table entry `{pos, Σ, t, seq, hop, source_type, velocity}` (Phys 2.1) | DIVERGENT | Velocity is billed on the wire but published as 0,0 (`scratch:6435-6437`) | DC-10 |
| 10 | EKF prediction with IMU control input u_k (Phys 2.2) | DIVERGENT | Constant velocity only (`ca-tbl-policy.cc:207-259`); the SHS engine diverges (K.9) | DC-1 |
| 11 | EKF correction with tier-specific R (Phys 2.3) | CONFORMANT | Per-measurement σ from each engine | — |
| 12 | Sparse cases solved against the propagated prior (Phys 2.4) | DIVERGENT | Solved before `Step()` against the previous posterior, and fused at t−dt. `Step()` then counts the cycle as a no-fix | DC-11 |
| 13 | Case A, two-circle + Mahalanobis choice (Phys 2.4) | OPT-IN | `--sparseCaseA`, `CaTblSolveCaseA` | DC-11 |
| 14 | Case B, orientation from IMU heading (Phys 2.4) | DIVERGENT | Prior-based choice; deliberate while the SHS diverges (`ca-tbl-sparse-anchor.h:78-83`) | DC-2 |
| 15 | Case C, relative shape only (Phys 2.4) | PARTIAL | Classified as unlocalisable; no relative tracking | DC-3 |
| 16 | Case D, one range + IMU (Phys 2.4D) | ABSENT | `Step()` discards fewer than three ranges | DC-4 |
| 17 | PCS term 1 = 1/trace(Σ) (Phys 2.5) | DIVERGENT (kept) | The bounded transform `traceRef/(traceRef+trace)` (`ca-tbl-policy.cc:857`); monotone equivalent | — |
| 18 | PCS term 3 from the tier-specific NLOS score (Phys 2.5) | OPT-IN | `--pcsNlosClassifier`; the default reads the boolean tag | preset |
| 19 | PCS weights tuned empirically, by LOOCV (Phys 2.5, audit Gap B) | DIVERGENT | Unweighted sum by default. CAL-01 ran and gave (3,0,0), not adopted. Since DC-6 (2026-09-30) the calibrated form is expressible: `--pcsForm=calibrated` with the weights (0 included) and per-tier thresholds from the CLI, gate G28. What is still missing is the calibration itself, CAL-02 | DC-5, ~~DC-6~~ done, CAL-02 |
| 20 | Switching-point integrity: SUSPECT downweights PCS (Phys 2.6) | OPT-IN | `--suspectRecovery=downweight` (DC-7, `d31dd0a`, gate G29) accepts the fix at R x 4 and scales its PCS by 0.5 (OURS), and the fix stays SUSPECT so it cannot count toward a de-escalation. The default still rejects it and forces an escalation (`ca-tbl-policy.cc`, the suspect branch of `Step()`). The spec's form also needs `--suspectForcesEscalation=false`, which the preset will set | ~~DC-7~~ done; preset |
| 21 | Integrity eligibility "NLOS_score LOW" (Phys 2.6) | OPT-IN | `--integrityNlosFromScore` (DC-8, gate G30): the mean NLOS_score of the fix set's scored links is below `--nlosScoreThreshold`, and with none scored the tag decides. The default is still "all links tagged LOS", which has no population under the default tag: 0 of 305 evaluated fixes at seed 7 (I.1). Read from the score, 296 of 297 are eligible and 152 are called suspect -- see I.1 for why that is its own problem | ~~DC-8~~ done; preset |
| 22 | Cross-validation against IMU dead reckoning and neighbours (novelty item 2) | OPT-IN | `--imuCrossCheck`, `--corroborationCheck`. The IMU witness uses the diverging SHS; withdrawn in the default because the evidence gate absorbs its refusals (B3 C1) | DC-9 |
| 23 | Sequence-number supersession (Phys 2.7) | OPT-IN | Runs only when the digest exchange does (`--sendHandshakeFrames`, finders) | MC-3 |
| 24 | Covariance Intersection on equal sequence numbers (Phys 2.7) | OPT-IN | `--tableFuseCI`; near-zero population (L.2) | MC-3 |
| 25 | Escalation/de-escalation hysteresis with N-cycle dwell (Phys 2.8) | CONFORMANT | `pcsThresholdUp/Down`, `--nDeescalate` | — |
| 26 | Per-entry prediction with source-type Q (Phys 2.2) | DIVERGENT | Isotropic walk-speed × age inflation (`scratch:7352-7382`) | DC-13 |

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
| 35 | Selective ACK; escalation handshake on the air (MAC §8) | PARTIAL | Frames are sent only with `--sendHandshakeFrames`. An ACK clears the oldest frame, not the matching one; a denied request is ACKed before arbitration; ESC_REQUEST is also sent on de-escalations | MC-1 |
| 36 | Triggered update / last gasp (MAC §3, §8) | PARTIAL | Only the RECEDING branch sends; there is no large-correction or last-gasp trigger, and no counter (M.9) | MC-2 |
| 37 | Mobility-state message selection (MAC §9) | OPT-IN | `--mobilityFromRange` | MC-4 |

### Network layer

| # | mechanism (docx §) | status | evidence | plan |
|---|---|---|---|---|
| 38 | Mesh: any two radios can exchange frames (throughout) | DIVERGENT | Links exist only between a field node and an anchor (`scratch:608-760`) | NW-0 |
| 39 | Addressing: node_id, identity class, target manifest id (Net §1) | CONFORMANT | `CaTblHeader`, `CaTblMac` | — |
| 40 | Routing table, ETX baseline (Net §2) | OPT-IN | `--fixRouting`, relays only | NW-2 |
| 41 | Confidence-weighted path selection for position-fix traffic (Net §3) | PARTIAL | `--fixRoutingConfidence`; the measured tag's traffic is never routed; α, β, γ are uncalibrated | NW-2 |
| 42 | Hop provenance reaches confidence and the RESULT (Phys 2.5, Net, Transp §3) | PARTIAL | Σ inflation exists (C5). The hop count reaches `Step` and RESULT as a literal 0; per-hop variance is missing | NW-1 |
| 43 | Identity-scoped QUERY, tier binding (Net §4) | PARTIAL | `--identityQueryPeriodS`; every driver is bound to the 868 MAC (L.2) | NW-3 |
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
| 53 | Gateways as edge servers (final plan) | PARTIAL | On by default since `db2ac2e`; the delivery of a tier-2/3 range is not billed (L.2.3) | SY-1 |
| 54 | GPS-surveyed, PPS-synchronised anchors (audit Q1) | OPT-IN | Under `--tier1Tdoa`, a per-stamp PPS residual on surveyed positions. The audit says GPS lock needs no separate sync protocol, so nothing further is missing | PH-1 |
| 55 | Five localised field nodes (audit Q2) | PARTIAL | One measured subject (C2-full structure merged at `abbe3de`, behaviour not looped) | MS-1 |
| 56 | Three gateways, 5 km radius (audit Q2) | PARTIAL | Reachable with `--nGateways`. The shape is to be chosen by study | MS-2, GEO-01 |
| 57 | Environmental fault classes for evaluation (exec Phase 5) | PARTIAL | Range bias, silence and depletion exist; no TX-power or antenna-loss fault | SY-5 |

## 2. Count

57 rows. It is more than the 52 of the first audit (2026-09-30), because rows 12, 26, 38, 44
and 50 were split out by the code explorations behind the plan.

| status | n |
|---|---|
| CONFORMANT | 16 |
| OPT-IN | 12 |
| PARTIAL | 14 |
| DIVERGENT | 13 (one of them, row 17, kept) |
| ABSENT | 2 |

These are parsed from the tables. A first hand count gave 17/11/14/13/2 and was wrong.
They have moved since the first parsed count (16/10/14/15/2): row 20, DIVERGENT → OPT-IN, with DC-7, and row 21 likewise with DC-8.

**Correction, 2026-10-01.** The DC-7 commit (`d31dd0a`) changed this block to 16/11/14/14/2 but not row 20 itself: the edit to the row was never written to disk. So for one commit the numbers above disagreed with the table they claim to be parsed from. Row 20 is corrected with DC-8, and the block is parsed again, as it says to be.

Recount it from the tables. Do not edit the numbers by hand.
