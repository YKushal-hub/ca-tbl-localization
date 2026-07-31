# CA-TBL Simulation Audit — What Exists, What's Broken, What's Missing

Audit date: 2026-07-30
Scope: `src/lora-physics-engine/`, `scratch/`, `scratch/python_handoff/`, `results/`
Target: prove 6 claims in simulation — (1) architectural advantage, (2) procedure advantage,
(3) handoff triggers correctly, (4) custom handshake, (5) specialized packets, (6) real physics.

---

## 1. What is actually implemented

### 1.1 PHY / propagation — extensive, and genuinely good

`src/lora-physics-engine/` is a large custom ns-3 module with **three parallel engines**:

| Engine | Class | Config |
|---|---|---|
| Tier 1 — 868 MHz | `LoRaPhysicsEngine` | `LoRaPhysicsConfig` |
| Tier 2 — 2.4 GHz | `LoRa2p4PhysicsEngine` | `LoRa2p4PhysicsConfig` |
| Tier 3 — UWB | `LoRaUWBPhysicsEngine` | `LoRaUWBPhysicsConfig` |

~35 submodules feed each engine:

- **Propagation / geometry**: log-distance PL, two-ray ground, terrain (Fresnel clearance, earth
  bulge, multi-edge diffraction, reflection count), tunnel, soil attenuation.
- **Stochastic channel**: Gudmundson spatially-correlated shadowing, Rician/Rayleigh fading with
  sampled K-factor, NLOS submodule with hidden-state Markov channel, delay spread + coherence
  bandwidth, correlated obstruction.
- **Environment**: foliage (static + wind-dynamic), atmospheric (rain rate sampling, snow, humidity
  refractivity, temperature-dependent CFO, effective earth radius k).
- **Indoor/body**: building entry, wall material, floor attenuation, body shadowing, temporal blockage.
- **Radio realism**: hardware TX variation, oscillator/thermal drift, phase noise, PLL lock + cycle
  slips, symbol sync + timing jitter, IQ impairment + image rejection, ADC clipping, AGC convergence,
  RF non-linearity, spur response, front-end desensitisation, CSS PHY, baseband, micro-DSP FFT
  confidence, spectrum occupancy, impulsive/coloured noise.
- **Link outcome**: sensitivity thresholds per SF/CR, interference + capture effect, collision overlap
  fraction, symbol error → CRC probability, SIC recovery, cooperative/distributed decode gain.
- **Ranging**: `lora-ranging-submodule` producing `estimatedRangeM`, `rangeErrorM`,
  `TimestampObservation`, with `RANGING_PHYSICS_DERIVED` mode.
- **Region/regulatory**: ADR, region scheduler state.

`PhysicsResult` exposes ~150 telemetry fields per reception.

**Verdict: claim (6) — real physics — is done and over-delivered *at the link level*.**

### 1.2 Empirical validation — real work already banked

`results/` holds 246 artefacts, including:

- 5M-row ML dataset + validation reports (`dataset_5M_validation_report*.txt`).
- Per-metric validation: sensitivity, PDR-vs-distance, capture effect, fade margin, RSSI CDF,
  inter-SF orthogonality, airtime/collision, gateway density, timestamp jitter.
- Datasets A–E across seeds 7/11/23 (reproducibility discipline is present).
- Scale sweeps: city-scale, 20 km, 35 km, 40 km rural, 80 km open field, 100 km regime sweep.
- Stage-1/Stage-2 trained models, 3-state sensitivity grid, CRLB check, TDoA jitter/sigma sweeps.

### 1.3 Tri-tier data generation

`scratch/ca_tbl_trajectory_generator{,_2p4,_uwb,_multi}.cc` — instantiate all three engines,
register 3 anchors (0,0)/(100,0)/(50,86.6), walk one node, and emit per-timestep, per-anchor,
per-tier CSV rows.

### 1.4 Decision layer — Python, offline

`scratch/python_handoff/`:

- `ekf_tracker.py` — `RangeEKF`, constant-velocity, range-measurement update.
- `ca_tbl_fusion.py` — PCS (3 terms: `trace_ref/(trace_ref+trace Σ)`, `1/GDOP`, LOS fraction),
  Mahalanobis integrity check → SUSPECT, escalate/de-escalate with hysteresis
  (`PCS_THRESHOLD_UP/DOWN`, `N_DEESCALATE=5`), tier-borrowing fallback when <3 anchors,
  5 ablation modes (`baseline`, `ca_tbl_escalate_only`, `ca_tbl_reject_only`,
  `ca_tbl_downweight_only`, `ca_tbl_full`).
- `arbitration_harness.py` — `U(r) = λ₁T_deg + λ₂T_batt + λ₃T_stale + λ₄T_query`, with calibrated `k`.
- `test_hop_variance.py`, `test_reject_vs_downweight.py` — unit-ish tests.

---

## 2. The structural problem

**There is no network, and no packets.**

The entire simulation is single-hop, single-node, tag↔anchor link sampling. Specifically:

1. **`relayHops` is a scalar penalty knob, not a routed path.** In `hybrid_lorawan_mesh.cc` it is
   swept 0/1/2 and handed to `ProcessTransmission()` as an argument. No packet is relayed by any
   node. There is no forwarding, no dedup cache, no hop limit, no route table.
2. **No packets exist as objects.** Nothing constructs a MAC header, sets a `msg_type`, computes
   time-on-air, or contends for the channel. `ProcessTransmission()` takes `(SF, BW, txPower)` and
   returns a statistical outcome.
3. **No ns-3 discrete-event loop.** `ca_tbl_trajectory_generator_multi.cc` is a `for (t += dt)` loop.
   `Simulator::Run()` is never used for the CA-TBL pipeline. Collisions are modelled *inside* the
   interference submodule as a probability, not as an emergent consequence of two nodes transmitting
   at overlapping times.
4. **One node.** Multi-target arbitration — one of the three locked novelty claims — has no scenario
   that exercises it.

### 2.1 The correctness bug this creates

`ca_tbl_trajectory_generator_multi.cc` writes **868 + 2.4 GHz + UWB rows for every node, every
anchor, every timestep**, unconditionally. `ca_tbl_fusion.py` then *selects* which tier's rows to
consume.

That means **the tier decision has zero causal effect on the physical layer.** UWB reception is
pre-computed even for timesteps where the policy never escalated. Consequences:

- Energy saved by *not* activating UWB cannot be measured → claim (1) unprovable as written.
- "Handoff is worth it" reduces to a post-hoc data-selection experiment → claim (2) weakened.
- Escalation latency, grant/deny, and UWB dwell contention are invisible → claim (3) partial.

This is the single highest-priority fix: **the decision loop must live inside the simulation and
gate whether a tier's transmission happens at all.**

---

## 3. Claim-by-claim status

| # | Claim | Status | What's missing |
|---|---|---|---|
| 1 | Architectural advantage (hybrid > single-band) | **~40%** | No single-band baseline arms (868-only, always-on-UWB, 2.4-only). **No energy model at all** — no per-tier mA, no TX/RX/sleep accounting, no joules-per-fix. Coverage×accuracy Pareto not computed. |
| 2 | Procedure advantage (handoff worth it) | **~45%** | Ablations compare CA-TBL against *itself*. Missing the literature baselines the paper promises: fixed-threshold switching, Poucet-style flat fusion, LoRaWAPS-style single-hop confidence. One 60 s trajectory, one node, one seed. No CEP50/CEP95, no confidence intervals. |
| 3 | Handoff triggers properly | **~40%** | Thresholds `1.55/1.70/2.30` are hardcoded magic numbers — the promised LOOCV calibration is not implemented. No ground-truth "should-have-escalated" label ⇒ trigger precision/recall/F1 uncomputable. **Hop count never enters PCS** — hop-provenance confidence degradation, pillar 1 of the novelty, is absent from the implemented formula. Integrity check requires `n_los_frac == 1.0` (all links LOS), which is very restrictive and likely under-fires. No fault-injection matrix (cold battery, antenna detune, iced tag). |
| 4 | Custom handshake | **0%** | Nothing exists. Missing: mobility-state classifier (stationary/approaching/receding), Beacon→Digest→Entry-Push exchange, eager one-shot push on receding, CAD-based CSMA/CA, priority contention windows (CW_urgent/high/normal/low), selective-ACK policy, flood dedup + random jitter rebroadcast. |
| 5 | Specialized packets | **0%** | Nothing exists. Missing: 9-byte MAC header (`packet_id(4) origin_id(2) hop_limit(1) msg_type(1) tier(1)`), the 7-type taxonomy (Discovery, Digest, Entry-Push, Triggered-update/last-gasp, Escalation req/grant, QUERY, RESPONSE), per-type payload structs, LoRa ToA formula per type, WPC duty-cycle enforcement (10% anchors / 2.5% field nodes), APC transmit-power loop. |
| 6 | Real physics on packets | **~70%** | Physics itself: done. But it is applied to *link samples*, not to *packets in a protocol*. Not wired to `Simulator::Schedule`. Interference is statistical rather than emergent from concurrent real transmissions. |

### 3.1 Also missing (novelty claims with no substrate)

- **Hop-provenance uncertainty propagation** — needs multi-hop relay; not implemented anywhere.
- **Confidence-weighted routing** (`path_cost = Σ α·NLOS + β·RSSI_var + γ·jitter`) — needs a routing
  layer; not implemented.
- **Identity-scoped QUERY / proxy response / session state machine** — not implemented.
- **Multi-anchor arbitration under contention** — scorer exists, scenario does not.
- **Covariance Intersection table fusion + sequence-number supersession** — not implemented.
- **Case A/B/C geometry resolution** (2 anchors / 1 anchor+1 peer / 0 anchors) — not implemented;
  fusion currently requires ≥3 anchors and otherwise coasts on prediction.

---

## 4. Recommended build order

Each step is gated on the previous one producing a number.

**S1 — Make the decision causal (unblocks 1, 2, 3).** — DONE
Move the tier policy into the C++ simulation. At each step, run only the active tier's
`ProcessTransmission()`. Escalation costs a grant round-trip and a UWB dwell. Emit a decision trace.

**S2 — Energy model (unblocks 1).** — DONE
Per-tier state machine (SLEEP/RX/CAD/TX/RANGE) with datasheet currents (SX1262, SX1280, DW3000).
Integrate joules over the run. Report accuracy-per-joule, not just RMSE.

**S2.5 — Tiered deployment geometry (unblocks 1, 2, 3).** — DONE, results pending a run
S1 and S2 were structurally right but ran in a scenario where two of three tiers could not
physically operate: one 100 m anchor triangle shared by all tiers, node random-walking a 1.2 km
box. `static_uwb` had `noFixCycles = 600/600`, was therefore never charged for a ranging exchange,
and came out as the *cheapest* arm — inverting the energy axis of claim 1. Five of six arms were
above 80% no-fix, so every RMSE was EKF coasting, and `static_2p4` beat `ca_tbl_full` 3.8× on
accuracy-per-joule. Those results are void.

Now: per-tier anchor sets (5 wide-area 868 gateways over a 3 km box; co-located 2.4 GHz and UWB
triads at 4 facility sites), facility-tour mobility so the node repeatedly enters and leaves
short-range coverage, geometric ground-truth labels (`vis868/vis2p4/visUwb`, `shouldUwb`) giving
the trigger a confusion matrix, and a `--probe` range sweep to check the siting against the physics
before any arm is run. Verifier now hard-fails below 50% fix coverage instead of warning at 20%.

See `CA_TBL_S1_S2_README.md`.

**S3 — Baseline arms (unblocks 1, 2).**
`868-only`, `2.4-only`, `always-on-UWB`, `fixed-threshold-switch`, `CA-TBL`. Same seeds, same
trajectories. 20+ seeds → CEP50/CEP95 with CIs.

**S4 — Packet + MAC layer (unblocks 4, 5, 6).**
`CaTblHeader` (ns-3 `Header` subclass), 7 msg types, ToA computation, CAD-CSMA/CA with priority CWs,
selective ACK, flood dedup + jitter, duty-cycle enforcer, APC loop. Drive it with `Simulator::Schedule`
so interference becomes emergent.

**S5 — Mesh + multi-hop (unblocks hop-provenance + routing novelty).**
Relay forwarding, route table, `path_confidence_penalty`, confidence-weighted path selection for
position-fix traffic only. Now `hop_count` can legitimately enter PCS.

**S6 — Multi-target + identity (unblocks arbitration novelty).**
5–10 tags, QUERY flood + proxy response, session state machine, anchor-side `U(r)` arbitration under
simultaneous contention. Report time-to-precision delta between highest/lowest priority targets.

**S7 — Trigger validation + fault injection (unblocks 3).**
Ground-truth escalation labels from true error; ROC/PR for the trigger; LOOCV threshold calibration;
fault-injection matrix for the integrity check (spoof-clean-but-wrong fixes, cold-bias RSSI,
antenna detune).

---

## 5. Environment note

The sandboxed shell cannot mount `\\wsl.localhost\...` (UNC paths unsupported), so builds and runs
must be executed by the user inside WSL. File read/write/edit into the repo works normally.
