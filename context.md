# CA-TBL — context for the accuracy and ML work

Research record, 2026-10-08. The S2 cloud branch, base `a0829213`.
Companion: `plan.md` (what to do about it). Exploratory scripts and the logging patch:
`analysis/accuracy_replay/`.

## 0. How to read this

**Labels.** *fact* = in a repo file or a tool output cited beside it. *measured* = produced in this
session; method in §9. *estimate*, *hypothesis*, *opinion* = stated as such.

**Two kinds of numbers.**
- **[repo]** — quoted from a tracked file. Provenance is the repo's (`run_id.txt`, `dirty 0`).
- **[M]** — measured in this session on a freshly built binary. **Exploratory: not citable under
  RULES.md §1 rule 13** (the tree carried a logging patch, so sidecars would read `dirty`).
  §6.1 shows the build reproduces the repo's results.

**Configurations used throughout.** Both are `--specProfile=true --duration=1800` (720 cycles of
2.5 s), seeds 1–20, as in CAMP-FINAL.
- **A** — the preset: 5 km box (`boxHalf` 5000), 3 gateways, 4 finders, 5 field nodes.
- **B** — the endgame: `A` plus `--boxHalf=750 --finderMode=1` (finders converging).

**Tag 0** is the measured lost node. Subjects 2–5 are the other four field nodes.

---

## 1. The project in one page

*fact* — RULES.md, `Architecture.md` §1–§3, §9.

A three-tier radio localisation simulation (ns-3.47 fork) for GPS-denied search and rescue.
- **Tiers.** 868 MHz LoRa (RSSI-inversion ranging, plus gateway TDoA), 2.4 GHz LoRa (two-way ToF,
  3.0 m floor), UWB (two-way ranging, 0.10 m floor).
- **Topology.** One population of tri-band nodes; a tier is a choice of radio. Gateways are the
  only immobile class (3 in the preset), finders (4) are mobile, field nodes (5) are the subjects.
  Facility sites are mobility targets and host no anchors.
- **Estimator.** 4-state constant-velocity EKF (`ca-tbl-policy.cc:173-445`). In the preset the
  prediction takes a synthetic IMU displacement (`--imuPredict`), ranges update sequentially, and
  a tier-1 TDoA position fix is fused after them.
- **Decision layer.** A confidence score (PCS) and an integrity check drive escalation between
  tiers; an evidence gate suppresses escalation to UWB; at most one tier transmits per cycle
  (RULES.md §6).
- **Preset.** `--specProfile` sets 97 flags (`TECH_DEBT` N.15). Gateways at (−3750, ±1299) and
  (−3000, 0) — **all three on the west edge of the 10 km box** [M: `deployment.json`] — and
  facilities at (±2350, ±2350).
- **What the project is.** A measurement apparatus with pre-registered campaigns, 58 gates with
  negative fixtures, and a negative-result record. The contribution is "the assembly and its
  quantification" (RULES.md §9), not a better localiser.

## 2. Requirements and how they are measured

| item | definition | source |
|---|---|---|
| Requirement | Availability A ≥ 0.95 at accuracy R ≤ 25 m, fixed in advance | `Architecture.md` §1.3, `Simulation.md` §1.2 |
| Availability, as written | fraction of cycles **with a fix accurate to ≤ 25 m**; a cycle without a fix counts against | `Simulation.md` §1.1 |
| Availability, as reported since GEO-01 | **mean over the five nodes of `fixCycles / steps`**: a radio fix, whatever its error | `camp_spec_analyse.py:matrix`, `GEO01_RESULTS.md` §1 |
| Accuracy | CEP50 / CEP95 of the position error over **all** cycles, tag 0 only | `CAMP_FINAL_RESULTS.md` §1 |
| Energy | joules per node over the run, per radio action, datasheet currents | `Architecture.md` §8 |
| Regulatory | 2.5 % duty (field), 10 % (gateways); cycle 2.5 s derived from it | `Architecture.md` §7.5–7.6 |
| Time bound | median time to first 100 m fix ≤ 600 s (GEO-01) | `GEO01_RESULTS.md` §1 |

**The two availability definitions differ and the reported one is not the requirement as written.**
The reported one ignores accuracy; a dead-reckoned estimate that is within 25 m on every cycle
scores 0 if no radio fix arrives. §6.2 measures both.

## 3. What is built

*fact* — `Architecture.md` §2.4, `TECH_DEBT` §L, `docs/SPEC_CONFORMANCE.md`.

| layer | state |
|---|---|
| Physics: 3 engines, ~35 submodules each, ~150 telemetry fields per transmission | complete; sensitivity unverified in absolute terms; two residuals (UWB ~3.7 dB, 868 MHz ~3.5 dB) open on purpose |
| MAC: CSMA/CA, duty governor, emergent collisions and capture, 9 message types, half-duplex chip, mesh, relays | complete (B1 met 2026-09-29) |
| Energy model | complete; reconciles to datasheet to 10 s.f. |
| Estimator: EKF, IMU-driven prediction, TDoA (Chan/Taylor, closed-form for 3 stamps), Cases A/B/D, relay chain, DC-7 downweight | built; several pieces default-off and enabled by the preset |
| Decision layer: PCS (calibrated form in preset), integrity check, evidence gate, grant-gated UWB, sessions, arbitration | built |
| Verification: 58 gates, verifier (134 checks / 153 gates on a fresh run), pre-registrations, sidecars | complete; 5 gates red by design or by finding (RULES.md §3) |
| ML | `ml_pipeline/`: one RSSI regressor, **not on any decision path** (§7.1) |

**Not built or not usable.** Hop-provenance (`hopWeight` stays 0, RULES.md §2); the SHS gait
engine diverges where fixes are sparse (`TECH_DEBT` K.9, left as debt by owner decision); G24
lists mechanisms with zero population under the preset (N.15).

## 4. What the repository has measured

### 4.1 Headline, the finished preset [repo: `CAMP_FINAL_RESULTS.md` §1]

| arm | A availability | A tag-0 CEP50 / CEP95 (m) | A subject J | B availability | B CEP50 / CEP95 (m) | B subject J |
|---|---|---|---|---|---|---|
| `ca_tbl_full` | 0.174 | 70.9 / 209.6 | 12.37 | 0.423 | 8.4 / 18.4 | 24.91 |
| `fixed_threshold` | 0.186 | 69.3 / 179.4 | 11.04 | 0.439 | 7.7 / 16.6 | 15.44 |
| `static_868` | 0.165 | 289.2 / 559.2 | 6.10 | 0.371 | 9.1 / 20.6 | 6.43 |
| `static_2p4` | 0.031 | 24.5 / 54.9 | 14.21 | 0.271 | 4.9 / 10.8 | 5.81 |
| `poucet_fusion` | 0.238 | 161.8 / 219.4 | 462.46 | 0.506 | 6.4 / 13.5 | 416.24 |
| dual-band LoRa fusion (`lora_fusion`) | 0.226 | 193.1 / 410.9 | 16.75 | 0.501 | 6.6 / 15.6 | 9.45 |

(The CEP50 column is a mean of per-seed values; §6.2 gives pooled values.)

**The requirement (A ≥ 0.95) has never been met.** Best availability anywhere: 0.329 at 5 km
(GEO-01, 10 candidate layouts), 0.562 in the endgame (FIXES-N26 `ALL`).

### 4.2 What worked [repo: technical report §0.1, §0.2]

| result | evidence |
|---|---|
| The filter beats snapshot estimators | cep95 2 016 m vs MLAT 20 831 m (20+/0−, p = 1.9e-6); K10. A filter beats a memoryless solver; tiering is not exercised there |
| The mesh carries availability | 0.041 → 0.319 → 0.647 (WAN only, sparse, WAN + mesh), 20/20 seeds |
| Estimation at the gateway saves node energy | −42.5 % (`static_868`), −32.9 % (hybrids), accuracy byte-identical; default since `db2ac2e` |
| An ideal dead reckoner collapses the tail | `static_uwb` cep95 8 413 → 17 m; **a ceiling, never a system result** |
| The measurement method | pre-registration, negative fixtures, provenance; found a 6 dB sensitivity error no gate caught |
| FIXES-N26, all four fixes | availability +0.111 / +0.139 / +0.133 over the preset (5 km / endgame / N8_ring), 5 km CEP95 −151.8 m, UWB fixes on 20/20 endgame seeds. **Not adopted**: 5 km subject energy 16.69 vs 12.37 J (`FIXES_N26_RESULTS.md` §5) |
| TDoA innovation gate alone | 5 km tag-0 CEP95 **−139.4 m [17.2, 284.7]**, availability −0.003, energy −0.06 J (`TECH_DEBT` N.25). Not in the preset |

### 4.3 What did not work

| result | evidence |
|---|---|
| PCS carries information beyond the covariance trace | **no**: all forms lose or tie against the monotone control; CAL-01 fitted weights are (3, 0, 0) |
| Confidence model beats a flat threshold | same controller: byte-identical on 19/20 seeds; `fixed_threshold` matches or beats `ca_tbl_full` in A and B |
| Tiering beats single-band | `static_2p4` dominates `ca_tbl_full` in 11/11 K1 cells; the 3 exceptions are a billing effect |
| UWB escalation | 0 UWB-delivered fixes in 20/20 seeds of A and B; UWB is 64.6 % of endgame subject energy (`TECH_DEBT` N.26) |
| Integrity check | flags 77.5 % / 96.9 % of **fault-free** fixes in A / B; precision 0.12–0.31. Cause found: distance mis-specified (S omits R). With `--mahalUseR` and a fitted threshold of 1 992.9 the false alarms go and so does detection (recall ≤ 0.031). Open (N.15) |
| Witness cross-check (IMU, corroboration) | no population in the shipped default (8/8 seeds byte-identical) |
| Hop-aware confidence | no population (`hopWeight` 0, hop count a literal 0) |
| Poucet (LoRa + UWB) vs CA-TBL | Poucet leads on availability; the lead is LoRa band fusion, not UWB (+0.004 [−0.009, 0.017] in the endgame) |
| SHS gait engine | diverges where fixes are sparse (up to 4×10⁶ m); the two guards bring it to 301–459 m, still net-negative on high-availability arms (`TECH_DEBT` K.9) |
| Geometry search | no candidate layout (N = 3…8) reaches A ≥ 0.95; best 0.329 |

## 5. Physical limits and the deployment

### 5.1 Per-link limits [repo: `results/geo01_stage1_2026-10-02/probe/probe_range_sweep.csv`]

| tier | distance | PDR | mean abs range error |
|---|---|---|---|
| 868 MHz | 100 m / 1 000 m / 3 000 m / 6 000 m | 1.00 / 0.90 / 0.53 / 0.26 | 51 / 497 / 1 367 / 2 794 m |
| 2.4 GHz | 50 m / 200 m / 600 m / 1 200 m | 0.97 / 0.71 / 0.26 / 0.07 | 2.5 / 3.3 / 4.6 / 7.3 m |
| UWB | 2 m / 5 m / 10 m / 20 m / 50 m | 0.78 / 0.51 / 0.30 / 0.075 / 0.007 | 0.08 / 0.08 / 0.08 / 0.11 / 0.17 m |

- 868 MHz range error is **about 0.5 × the range** at every distance (*fact*: σ = d · ln10/(10n) ·
  shadowing σ, `lora-ranging-submodule.cc:169`). A single RSSI range at 3 km is worth ±1.4 km.
- UWB is precise and almost never decodes beyond 10 m. Three simultaneous UWB ranges need three
  nodes within a few metres.

### 5.2 Geometry [M]

Facilities at (±2350, ±2350), gateways clustered at x ≤ −3000. Computed from those coordinates:
a node dwelling at an eastern facility is 5.8–7.1 km from the gateways (868 MHz PDR 0.26 at
6 km and falling), and the three gateways lie within a 21° bearing sector as seen from it. The
western facilities are 1.8–3.9 km away. Cross-range information from 868 MHz is therefore poor
whenever the node is in the east; the consequence for TDoA is in §6.4.

## 6. Where the error comes from

All of §6 is **[M]**: configurations A and B, seeds 1–20, `ca_tbl_full` unless stated.

### 6.1 Instruments validated

- **Reproduction.** Fresh build (gcc 13, `-march=native`, patch applied) against the tracked
  `camp_final_2026-10-04` summaries: configuration A **640 of 640** compared values identical
  (`cep50M cep95M rmseM fixCycles totalJ escalations suspects mac_tx`, 4 arms × 20 seeds);
  configuration B **630 of 640**. The 10 differing values are `static_2p4` seeds 15 and 16
  (endgame) only; `ca_tbl_full` is identical on all 40 cells. This is the known machine
  dependence of `-march=native` (`TECH_DEBT` 4.1/4.2), not a defect of the patch.
- **Independent recomputation of repo numbers.** Fix-fraction availability 0.174 / 0.423
  (`ca_tbl_full`), 0.031 / 0.271 (`static_2p4`), 0.165 / 0.371 (`static_868`) — equal to the
  CAMP-FINAL table. UWB-active cycles 900 of 14 400 in B, nearest finder ≤ 30 m on 91 % of
  them, no UWB neighbour on 78 % — equal to N.26's 900 / 823 / 704.
- **Replay fidelity.** A Python mirror of the EKF, run on the logged range stream, against the
  simulator's own error (pooled CEP50 / CEP95 over 20 seeds):

  | arm | cfg | simulator | replay |
  |---|---|---|---|
  | `static_2p4` | B | 4.3 / 13.4 | 4.4 / 13.2 |
  | `static_868` | A | 52.3 / 1 807 | 59.9 / 1 880 |
  | `ca_tbl_full` | B | 6.4 / 24.7 | 4.9 / 18.8 |
  | `ca_tbl_full` | A | 24.0 / 626 | 34.9 / 1 461 |

  The replay omits DC-7 inflation, the Case A/B solves and the Case D update. **Read differences
  between variants, not absolute levels, and trust `static_2p4`-B and `static_868`-A most.**

### 6.2 Availability: radio fixes versus accurate estimates

Mean over the five nodes; "within 25 m" counts cycles whose estimate is within 25 m **whether or
not a radio fix arrived**.

| arm | cfg | fix availability | within 25 m |
|---|---|---|---|
| `ca_tbl_full` | A | 0.174 | 0.615 |
| `static_2p4` | A | 0.031 | 0.683 |
| `static_868` | A | 0.165 | 0.319 |
| `ca_tbl_full` | B | 0.423 | 0.946 |
| `static_2p4` | B | 0.271 | 0.968 |
| `static_868` | B | 0.371 | 0.613 |

Per node, `ca_tbl_full`, configuration A (fix / within 25 m): tag 0 0.460 / 0.510; subject 2
0.240 / 0.651; subject 3 0.105 / 0.725; subject 4 0.063 / 0.619; **subject 5 0.002 / 0.569**.

- Subjects 4 and 5 almost never get a radio fix at 5 km, yet are within 25 m on 57–62 % of
  cycles. What holds them there is dead reckoning (§6.3).
- Tag 0 pooled, A: `ca_tbl_full` CEP50 24.0 / CEP95 **626.5** m, within 25 m on 0.510 of cycles,
  CEP95 > 100 m on 7 of 20 seeds and > 500 m on 3; `static_2p4` 12.5 / 79.4, 0.722, 1 of 20.
  B: `ca_tbl_full` 6.4 / 24.7, 0.951; `static_2p4` 4.3 / 13.4, 0.997.
- **The two definitions rank the arms differently.** On fix availability `ca_tbl_full` beats
  `static_2p4` 5.6× at 5 km; on accurate estimates `static_2p4` is ahead in both configurations.

### 6.3 Dead reckoning carries the result; radio fusion adds the tail

The preset's prediction is a synthetic IMU: `u = (1+s) R(ψ) Δ` with a gyro bias drawn once per
run (σ = 8.816 °/h), angle random walk and a 2.66 % scale error, **applied to the true
displacement** (`ca-tbl-imu-model.h`). It is "a displacement-level error model, not a sensor
simulation". The filter starts at the truth with a 100 m prior.

Replay with no radio at all (dead reckoning from the true start), pooled, tag 0:

| cfg | DR only CEP50 / CEP95 (m) | `ca_tbl_full` in the simulator |
|---|---|---|
| A | 25.7 / 112.8 | 24.0 / **626.5** |
| B | 10.6 / 44.9 | 6.4 / 24.7 |

- At 5 km the radio contributes almost nothing to the median (25.7 → 24.0 m) and **adds** a tail
  (112.8 → 626.5 m). In B the radio helps.
- *Caveat (fact, then opinion).* The IMU model is generous in two ways: the filter begins at the
  true position, and 8.816 °/h is a bias-instability figure (an Allan-deviation floor), not a
  turn-on bias. Whether a trunk-mounted MEMS gyro holds that after calibration is not in the
  repo and I have not checked a datasheet. *Opinion:* every 5 km accuracy number is conditional
  on this model. No CLI flag scales it (`CaTblImuParams` has no `AddValue`).

### 6.4 One confident wrong fix ruins a run

`static_868`, A, seed 1 [M]: for 40 s the covariance stays at its initial 2.0e4 m² because 868 MHz
ranges (σ ≈ 600 m) carry no information; the error is 2.4–10.5 m. At t = 42.5 s a 3-stamp TDoA
solve arrives 2 630 m from the truth with a claimed covariance of 604 / 252 m². The filter jumps
to it (error 2 453 m, covariance 790 m²). Every later range (σ ≥ 600 m) is too weak to pull it
back quickly: median error 2 456 m for 42–100 s, 2 230 m for 100–400 s, 816 m for 400–900 s,
697 m for 900–1 500 s and 588 m for the last 300 s. Dead reckoning alone: CEP95 36 m on that seed.

Across all runs the TDoA attempt log, `ca_tbl_full`, 20 seeds:

| cfg | attempts | OK | TOO_FEW | AMBIGUOUS / NO_ROOT | stamps = 0 / 1 / 2 / 3 |
|---|---|---|---|---|---|
| A | 4 948 | 147 | 4 758 | 29 / 14 | 1 291 / 2 343 / 1 124 / 190 |
| B | 9 034 (1 DIVERGED) | 4 053 | 4 979 | 0 / 1 | 149 / 1 102 / 3 728 / 4 055 |

Accepted solves, `ca_tbl_full`, 20 seeds: **A** — 147 solves, median error 723 m against a median
claimed σ of 543 m, 17 % beyond 3 σ, 59 of them (40 %) more than 1 km off; **B** — 4 053 solves,
median error 26 m against 28 m, 2.5 % beyond 3 σ, 13 beyond 1 km. At 5 km the typical solve is
consistent with its claim but nearly useless; the danger is the minority whose *claimed*
covariance is small and whose root is wrong. Replay, A, per-seed paired difference in CEP95, seed
bootstrap 95 %:

| change (all on the same logged ranges) | arm | Δ CEP95 (m) | seeds better / worse |
|---|---|---|---|
| TDoA innovation gate (NIS ≤ 9.21) | `static_868` | **−483.8 [−862.7, −143.7]** | 10 / 0 |
| same | `ca_tbl_full` | −364.6 [−723.0, −72.1] | 12 / 3 |
| prior std 100 m → 1 m, no gate | `static_868` | −495.4 [−888.4, −177.6] | 18 / 2 |

The gate is what `--tdoaInnovationGate` already implements; FIXES-N26 measured it in the simulator
(−139.4 m). The prior-std row says the 5 km tail is mostly "loose prior + early confident outlier".

### 6.5 The covariance is inconsistent; gating on it backfires

Median of `error² / trace(P)` over cycles (a calibrated 2-D Gaussian gives 0.69) and the share of
errors inside the isotropic 95 % ellipse (replay, prior std 100 m for the baseline):

| arm, cfg | baseline EKF | augmented-state EKF (§6.8) |
|---|---|---|
| `ca_tbl_full`, B | 37.5, coverage 0.19 | 6.3, 0.50 |
| `static_2p4`, B | 24.7, 0.15 | 5.5, 0.51 |
| `static_2p4`, A | 2.2, 0.71 | 1.0, 0.93 |

*Mechanism (fact from the code).* `CaTblImuDeadReckoner` hands the filter the **increment** of
accumulated displacement-error covariance as if independent each cycle, "correlation with earlier
states is still ignored -- stated" (`ca-tbl-imu-model.h`). But the gyro bias and the scale error are
constant for the run. The filter therefore believes its position far better than it is.

Consequence, replay B, `ca_tbl_full`, both filters starting at 1 m: adding range and TDoA
innovation gates to the **baseline** makes it worse — CEP50 +1.65 m [0.73, 2.80], CEP95 +6.89 m
[1.23, 13.10], worse on 16–18 of 20 seeds — because an overconfident P rejects good data.
`static_2p4`: +1.11 m [0.32, 2.43] and +4.88 m [0.53, 10.69].
`static_868`-B is the exception: gates help the baseline there (CEP95 49.1 → 38.3 m), where TDoA
outliers dominate. Whether a gate helps depends on whether P is honest.

### 6.6 Ranging errors are calibrated and independent

`ca_tbl_full` range logs, all links, 20 seeds, z = (range − true) / reported σ:

| link class | cfg | n | mean z | std z | lag-1 autocorr | share \|z\| > 3 |
|---|---|---|---|---|---|---|
| 868, finders | A / B | 13 168 / 27 419 | −0.10 / −0.23 | 0.97 / 0.96 | — | 1.1 % / 0.1 % |
| 868, gateways | A / B | 5 613 / 20 769 | −0.05 / +0.12 | 0.82 / 0.86 | 0.019 / −0.001 | 0.1 % / 0.1 % |
| 2.4 GHz, gateways | B | 3 692 | +0.04 | 0.99 | −0.020 | 0.2 % |
| 2.4 GHz, peers | B | 9 295 | +0.03 | **1.50** | — | 4.9 % |
| 2.4 GHz, finders | B | 12 738 | −0.38 | 0.92 | — | 0.7 % |

- In the rows shown, reported σ matches the realised error to within 20 % except for peers; gateway errors
  are serially uncorrelated. **There is no temporal or spatial structure in a gateway range error
  for a model to learn** (*fact*: the error is drawn `N(0, σ)` per call, `lora-ranging-submodule.cc:173`).
- Tier-1 relative error std is 0.51–0.64 of range in every distance bin (A, all 868 MHz ranges).
  There is a mild low bias beyond 1 km (mean z −0.10 … −0.40, relative −8 % … −15 %). *Hypothesis:*
  only links that decode are ranged, and a long reading is likelier to be lost.
- Peers' 2.4 GHz ranges are 1.5× over-confident: the other subject's reported position is itself
  an estimate.

### 6.7 TDoA's covariance and the NLOS oracle

- *fact* — TDoA stamps add an NLOS excess delay only where the **engine's truth** says NLOS, and
  the stamp σ and the EKF's R inflation read `meas.nlos`, which under `--nlosSource=propagation`
  (in the preset) is that truth (`ca_tbl_causal_sim.cc:5351-5360`; `ca-tbl-policy.cc:1404`). NLOS
  is an **oracle** in R and in the TDoA weights.
- [M] Pooled over 20 seeds, `ca_tbl_full`, the hand-built classifier (`--nlosClassifier`) against
  that truth:

  | tier | cfg | NLOS among decoded links | precision | recall |
  |---|---|---|---|---|
  | 868 | A / B | 7.3 % / 3.8 % | 7.8 % / 5.2 % | 7.2 % / 9.4 % |
  | 2.4 GHz | A / B | 0.31 % / 0.29 % | 5.4 % / 2.7 % | 42.9 % / 28.2 % |
  | UWB | A / B | empty | — | — |

  NLOS links mostly fail to decode, so the population the classifier could act on is small, and
  where it exists it is read as truth anyway.
- TDoA error rises with the number of NLOS stamps in the cycle (B, `ca_tbl_full`, 4 053 accepted
  solves, 20 seeds: median 20.9 m with none, 57.5 m with one, 83.0 m with two) while the median
  normalised error z falls (1.04, 0.92, 0.73). The claimed covariance already accounts for it —
  because it is told.

### 6.8 What an error-state IMU filter does [M, replay]

An augmented-state EKF carries the gyro bias `b` and scale `s` as states
(`z = [x, y, ψ, b, s]`, `Δ ≈ (1−s) R(−ψ) u`), with the generator's own priors. It uses the same
logged ranges and TDoA fixes as the baseline.

Pooled CEP50 / CEP95 (m), 20 seeds; "+gates" = range NIS ≤ 6.63 and TDoA NIS ≤ 9.21; both filters
start with a 1 m prior unless marked:

| arm, cfg | baseline EKF | + gates | augmented | augmented + gates |
|---|---|---|---|---|
| `ca_tbl_full`, B | 4.9 / 17.8 | 5.2 / 32.1 | 2.1 / 9.2 | **1.7 / 5.4** |
| `static_2p4`, B | 4.3 / 12.9 | 4.7 / 20.4 | 1.9 / 6.7 | **1.6 / 4.6** |
| `static_868`, B | 8.4 / 49.1 | 7.3 / 38.3 | 2.7 / 89.6 | **2.0 / 6.1** |
| `static_2p4`, A | 21.1 / 104.0 | 22.0 / 104.2 | 12.5 / 73.7 | 12.8 / 75.7 |
| `ca_tbl_full`, A | 22.2 / 120.1 | 23.1 / 105.1 | 23.9 / 196.7 | 13.5 / 91.9 |

Paired per seed, augmented + gates against the baseline EKF (both 1 m prior):

| arm, cfg | Δ CEP50 (m) | Δ CEP95 (m) | seeds better / worse (CEP95) |
|---|---|---|---|
| `ca_tbl_full`, B | −4.10 [−5.91, −2.50] | −9.04 [−12.91, −5.68] | 19 / 1 |
| `static_2p4`, B | −3.00 [−4.22, −1.89] | −6.86 [−9.27, −4.86] | 19 / 1 |
| `static_868`, B | −8.56 [−12.19, −5.69] | −41.05 [−65.65, −20.95] | 20 / 0 |
| `static_2p4`, A | −7.05 [−11.92, −3.04] | −20.48 [−37.74, −7.95] | 13 / 3 |
| `ca_tbl_full`, A | −7.19 [−14.81, 0.68] | −64.27 [−141.62, −9.64] | 14 / 6 |

**Controls.**
- *Is it the initial covariance?* No, in B: 100 m vs 1 m prior moves `ca_tbl_full`-B CEP95 from
  18.8 to 17.8 m. At 5 km yes: the baseline's CEP95 falls 1 461 → 120 m (`ca_tbl_full`) and
  1 880 → 136 m (`static_868`) with a 1 m prior (§6.4).
- *Is it just process noise?* Inflating the IMU process noise recovers part of it. B,
  `ca_tbl_full`, ×10: 3.3 / 10.5; ×30: 3.0 / 10.4 (with gates 2.7 / 7.9); `static_2p4`, B, ×10:
  2.7 / 9.6. The augmented filter is still ahead (2.1 / 9.2, and 1.7 / 5.4 with gates).
- *Is it matched to the generator?* **Yes, by construction** (constant bias, constant scale,
  angle random walk). The IMU quality test below keeps the generator's structure and changes
  magnitude only.

**IMU quality** (the generator re-implemented and re-drawn, gyro bias ×1 / ×10 / ×30; DR-only
CEP50 is 10.3 / 48.4 / 140.4 m at B):

| B, `ca_tbl_full`, CEP50 / CEP95 (m) | ×1 | ×10 | ×30 |
|---|---|---|---|
| baseline EKF | 5.0 / 12.4 | 6.9 / 40.7 | 12.3 / 125.7 |
| baseline + gates | 5.4 / 19.1 | 8.4 / 225.3 | 26.8 / 729.0 |
| augmented | 2.1 / 9.2 | 2.3 / 9.5 | 2.4 / 10.0 |
| augmented + gates | 1.7 / 5.5 | 1.8 / 5.8 | 1.8 / 5.9 |

At 5 km (`static_2p4`): baseline CEP50 18.0 / 56.1 / 183.0 m, augmented 11.5 / 15.6 / 18.4 m.
The gain grows with IMU error; gates on the baseline turn harmful.

### 6.9 UWB [M]

Endgame, `ca_tbl_full`, 20 seeds: 900 UWB-active cycles (6.2 %); **0 fixes** (the trace's
`fixTier = UWB` appears on non-fix cycles only). Nearest finder ≤ 30 m on 91 % of them (median
14.9 m, ≤ 10 m on 25 %); exactly one finder within 30 m on 88 %, **never three**; no UWB neighbour
discovered on 78 %. With 4 finders for 5 subjects and PDR 0.30 at 10 m (§5.1), three simultaneous
UWB ranges to tag 0 are structurally unavailable. This restates N.26 with an independent count.

## 7. ML in the repository today, and what the data permit

### 7.1 What exists

`ml_pipeline/train.py` — an MLP (9-environment embedding + log-distance + TX power → received RSSI,
MSE) trained on `ml_training_dataset.csv` (5 M rows, 47 columns from `scratch/ml_dataset_generator.cc`,
**868 MHz engine alone, random links, not the CA-TBL scenario**). `verify_ml.py` reports RMSE. The
dataset and a trained model are not in the tree. Nothing imports it; `MASTER_PLAN.md` §4.5 records
the role of the dataset as open (DEC-06). *Opinion:* it is a surrogate of the simulator's own path
loss. It cannot add information the simulator does not already compute, and no decision path reads RSSI
through it.

### 7.2 Properties of this simulator that bound any ML gain

1. **Circularity.** Any model trained here learns the simulator's generative model. The repo says
   so for its own classifier (`ca-tbl-nlos-classifier.h`: "can only REDISCOVER the NLOS the channel
   model created"). A gain is a gain on the simulator.
2. **Ranging errors are i.i.d. and calibrated** (§6.6): nothing for a ranging corrector to learn
   beyond a small distance-dependent bias.
3. **NLOS is an oracle** in R and TDoA weights (§6.7); a learned detector can only replace truth
   with an imperfect guess unless the oracle is removed first.
4. **The IMU error is a two-parameter generator** (§6.3): a learned inertial model would learn two
   scalars and a random walk. Real gait and mounting effects are absent.
5. **Rule 8 (RULES.md §1).** The sampler draws from the default stream, so any change in the
   number of transmissions displaces every link; only paired seeds within one binary are
   comparable, and cross-binary magnitudes are not.

### 7.3 Feasibility tests run [M]

Seed-grouped 5-fold cross-validation, gradient-boosted trees, `ca_tbl_full`, 20 seeds.

**T1 — can a learned model flag unreliable TDoA solves better than the innovation gate?**
Target: solve error > 3 × claimed σ.

| cfg | OK solves | positives | AUC learned | AUC NIS against the filter prior |
|---|---|---|---|---|
| A | 389 (3 arms) | 69 | 0.963 | **0.960** |
| B | 15 386 (3 arms) | 385 | 0.918 | **0.905** |

*Result:* the learned model adds 0.003–0.013 AUC over a one-line gate that already exists.
For "error > 100 m": 0.827 vs 0.596 (A), 0.958 vs 0.621 (B), but there the claimed σ alone gives
0.92 in B. **No case for ML here.**

**T2 — can a learned model predict a fix's error from 18 logged features better than the
covariance trace?** (features: trace, GDOP, anchors, NLOS count, Mahalanobis, mean σ, neighbour
counts, ranges used, PCS, tier, age, previous trace; target log(1+error)).

| cfg | fix cycles | ρ learned | ρ trace | ρ PCS | AUC for top-decile error: learned / trace |
|---|---|---|---|---|---|
| A | 6 624 | 0.263 | 0.356 | 0.210 | 0.634 / **0.731** |
| B | 14 106 | 0.353 | 0.230 | 0.256 | 0.692 / **0.720** |

Within a seed in B the learned ranking beats the trace (+0.203 [0.117, 0.300], 19 of 20 seeds),
but pooled it is worse at flagging the large errors. *Result:* **no evidence** that this feature set
supports a better quality score than the trace. Consistent with CAL-01 and N.16.

## 8. Open defects that bear on accuracy

| id | defect | status |
|---|---|---|
| N.15 | integrity distance heavy-tailed even with R in S; threshold fit removes false alarms and detection | open |
| N.14 / N.26 | one tier per cycle discards 1–2 range sets; UWB buys no fix | N.26 fixes built, not adopted (5 km energy) |
| N.25 | 3-gateway TDoA worsens 5 km tier-1 accuracy | gate built (`--tdoaInnovationGate`), not in preset |
| N.16 | trace ranks error backwards at 8 gateways | recorded; not at the 3-gateway preset |
| K.9 | SHS gait engine diverges on sparse fixes | debt by owner decision |
| K.8 / A8 | PCS uncalibrated, recorded not fixed | do not fix casually (RULES.md §2) |
| §6.5 | EKF covariance inconsistent: IMU errors treated as independent increments | **new here**, measured in replay only |
| §6.3 | IMU error model unscaled by any flag; filter starts at truth | **new here** |
| §6.2 | reported availability is not the requirement as written | **new here**; owner decision |
| 4.2 | `-march=native` makes byte-identity machine-dependent | known |

## 9. Method and reproducibility of this research

1. Read: RULES.md, `Architecture.md`, `Simulation.md`, `MASTER_PLAN.md`, the technical report,
   `docs/WORK_PACKAGES.md`, `TECH_DEBT.md` (N.14–N.26, K.8–K.9, L), the 2026-10-05 handoff,
   CAMP-FINAL / LORA-CMP-2 / FIXES-N26 / TDoA-01 / GEO-01 / CAL-03 / UWB-NLOS results, and the
   estimator, ranging and IMU sources cited above.
2. Built `scratch/ca_tbl_causal_sim` (`--build-profile=optimized`, gcc 13.3, `-march=native`,
   about 15 min on 4 cores) with `analysis/accuracy_replay/rangelog_patch.txt` applied. The patch adds
   one env-var-gated CSV writer before tag 0's `Step()` and after `SetAuxPositionFix`. It draws no
   random number. Its inertness is supported by §6.1 (identical results on 1 270 of 1 280
   compared values; the 10 exceptions are two `static_2p4` cells, outside the patched path's
   effect on other arms). The patch is reverted in the tree.
3. Ran configurations A and B, seeds 1–20, arms `ca_tbl_full,static_868,static_2p4,fixed_threshold`,
   ≈ 85 s per cell per three arms.
4. Analysed with the scripts in `analysis/accuracy_replay/` (README there). Seed bootstrap: 4 000
   resamples, 95 %.

**Not done.**
- No closed-loop test of any proposed change; §6.4, §6.5, §6.8 are open-loop replays.
- The Python mirror omits DC-7, Case A/B/D (fidelity in §6.1).
- No subject-level replay; subjects' ranges are not logged.
- No run of configuration C (N8_ring) or of the fault matrix.
- No datasheet or literature check of gyro bias figures; no external IMU or ranging dataset
  (the repo's validation datasets are absent: `analysis/overall_agreement_report.txt` reads "Data not found").
- The sandbox's network was not used for literature.

## 10. Unknowns that change the plan

1. Whether the owner wants the requirement read as "accurate fix" (as written) or "radio fix"
   (as reported). §6.2 shows this decides which arm leads.
2. Whether the IMU model's optimism (§6.3) is acceptable for the thesis claim or should be made a
   swept parameter. It changes every 5 km number.
3. Whether the start-at-truth, 100 m prior is a scenario choice or an artefact.
4. Closed-loop effect of a consistent covariance on escalation, finder dispatch and PCS: unmeasured.

---

## 11. Addendum, 2026-10-08 (later): the new preset scoreboard

**Baseline moved.** `e308c14f` (branch `work/2026-09-27-spec-b1`, owner override) put FIXES-N26's four fixes into
`--specProfile` (102 settings). My `--specProfile` + those four flags cells are identical to the tracked
`fixes_n26_2026-10-05/eval/ALL_main` and `ALL_end` cells (320 of 320 compared values). The sections above
measured the 97-setting preset except where marked `ALL`.

Closed loop, 20 seeds, [M], exploratory. Five-node fix availability / five-node within 25 m / tag-0 CEP50 /
CEP95 (m) / J per node:

| arm | 5 km | endgame |
|---|---|---|
| `ca_tbl_full` (new preset) | 0.285 / 0.613 / 16.2 / 85.0 / 16.7 | 0.562 / 0.968 / 4.8 / 16.1 / 23.0 |
| `fixed_threshold` + the four fixes | 0.274 / 0.625 / 16.7 / 72.9 / 16.9 | 0.545 / 0.964 / 4.6 / 17.6 / 22.0 |
| `static_2p4` | 0.031 / 0.683 / 12.5 / 79.4 / 14.2 | 0.271 / 0.968 / 4.3 / 13.4 / 5.8 |
| `poucet_fusion` | 0.238 / 0.499 / 29.7 / 290.0 / 462.5 | 0.506 / 0.862 / 5.4 / 18.9 / 416.2 |
| `lora_fusion` | 0.226 / 0.513 / 36.1 / 2 308.6 / 16.8 | 0.501 / 0.885 / 5.9 / 18.5 / 9.5 |

Findings that change the picture (full tables in `plan_part_a_filter.md` §2 and `plan_part_b_hybrid.md` §1–2):
- Fix availability is already first among all arms on the new preset.
- Multiplying the IMU increment covariance by 10, for every arm, gives `ca_tbl_full` 11.7 / 61.7 m (5 km) and
  2.7 / 8.0 m (endgame), and a significant win over `poucet_fusion` and `lora_fusion` on every accuracy metric.
- Remaining significant loss: 5 km five-node within 25 m against `static_2p4` (−0.067 [−0.129, −0.016]).
  It comes from subjects 2–5, not tag 0.
- Against `fixed_threshold`, `ca_tbl_full` ties on all accuracy metrics in both configurations.
- Endgame range supply: tag 0 gets 1.80 precise (σ < 40 m) ranges per cycle against `static_2p4`'s 2.67, because
  peer ranges fall from 1.52 to 0.23 per cycle. The dual base moved the hybrid's active tier off 2.4 GHz
  (86 % → 36 % of cycles).
- Replay with the new preset's range streams reproduces `ca_tbl_full` well (15.2 / 87.8 vs 16.2 / 85.0 m at 5 km;
  4.8 / 15.8 vs 4.8 / 16.1 m endgame). The error-state filter in replay: 7.2 / 60.4 and 1.5 / 4.7 m with the
  scenario's 100 m prior.
- Dropping or inflating the 868 MHz ranges to absorb their long-range bias hurts or changes nothing.
