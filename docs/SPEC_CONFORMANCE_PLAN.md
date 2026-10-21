# Plan: make CA-TBL true to spec (`finalyearprojectcontext.docx`)

> **Status, 2026-10-21: executed.** `docs/SPEC_CONFORMANCE.md` is the register. Its 2026-10-21 note records the
> owner's decisions that changed the preset since (DD 19, 22, 23, 24).

## Context

`~/catbl_work/finalyearprojectcontext.docx` is the spec that Bar 2 of `docs/TECH_DEBT.md` §L is measured against. An audit of it against the code on 2026-09-30 counted 52 checkable mechanisms. Hardware items and evaluation metrics were not counted.

| status | n |
|---|---|
| true to spec, on by default | 17 |
| built to spec but switched off by default | 11 |
| built, but deviates from spec or has no population | 22 |
| absent (Case D; query-triggered escalation) | 2 |

The user wants the simulator brought to spec. This plan lists **every lacking part**. For each, it gives what the spec says, what the code does, why the gap exists, and the concrete steps to close it.

### Owner decisions taken for this plan (2026-09-30)
1. **Defaults.** One `--specProfile=true` preset turns on the whole spec configuration. Existing defaults stay put, and every `results/` sidecar stays reproducible. Making the preset the default later is a separate, registered campaign, out of scope here.
2. **IMU.** A synthetic IMU error model is built: the walker's true motion plus sensor errors taken from a cited datasheet, integrated as dead reckoning. The SHS gait engine is left as it is (K.9, owner decision 2026-09-08).
3. **PCS.** The preset adopts LOOCV-calibrated weights **and** thresholds. Because RULES.md §2 forbids a casual PCS fix, this is a registered PCS campaign (CAL-02). The report states the outcome plainly: the calibrated score is trace-only, which is effectively the flat-threshold baseline.
4. **Deployment.** **All 5 field nodes are measured subjects** (C2-full).
5. **Gateway geometry.** The shape is **found by a registered study** (GEO-01, §3.4), not assumed. It compares box, crescent/arc, circle/ring, line, cluster, star, L and an optimised free placement at 3–8 gateways over the spec's 5 km-radius search area. It picks **the least infrastructure that meets a registered SAR criterion**, subject to a realism constraint: gateways only where a vehicle can reach. The constraint comes from the owner's 2026-09-09 rejection of the interior grid (HANDOFF_2026_09_09 §4).
6. **Rescuers.** The preset includes 4 finders on the two-phase search: `--nFinders=4 --finderMode=2 --finderDispersal=1.0`, i.e. the NC2 searcher regime.
7. **Evidence gate.** It stays **on** in the preset. It is recorded as a project addition outside the spec, and its consequence is accepted: the witness cross-check's refusals are absorbed (B3 C1).
8. **Requester.** **Both.**
   - The base station behind the gateway backhaul runs the batch session.
   - Each finder (a rescuer's handheld) can open its own session over the mesh.

---

## 0. Rules every work package follows

These rules come from RULES.md. They are the reason each step below is shaped the way it is.

1. **Default-off flag for every change.** The default must reproduce the reference byte-for-byte.
   - Before the first code commit, freeze the binary and libs at `~/catbl_ref_spec0`. Include `PROVENANCE.txt`, `RUN.txt` and an `nc.sh` copied from `~/catbl_ref_edge/nc.sh`, pointing at a new `nc/` directory.
   - Run it after the M.2 commit lands.
   - Every commit then runs `bash ~/catbl_ref_spec0/nc.sh '<build>' <tag>`. NC1 must be 7/7 and NC2 2/2 byte-identical.
2. **The "old value reproduces the previous commit" check.** It applies wherever a flag *replaces* a behaviour rather than adding one. Freeze the pre-change binary at `~/catbl_prev_<item>` and compare in at least 5 configurations, following the pattern of `fab313c` and `53e5c38`.
3. **A gate with negative fixtures.** Each mechanism gets one in `scripts/catbl_gates.sh`. Each fixture must be observed to fail for its own reason (rule 6). The suite runs all gates with TMPDIR outside `/tmp`, and must leave `git status` unchanged (rule 7).
4. **Help text argues the default.** Every new constant is either cited from a real document or labelled OURS (`docs/CONTEXT_PACK.md` §1).
   - Datasheet numbers are read from the vendor PDF. There is no PDF text tool on this machine, so this needs `sudo apt install poppler-utils` (owner approval), or the owner pastes the table values.
   - Nothing is typed from memory or from search snippets.
5. **Random draws come from dedicated streams.** Use `CreateObjectWithAttributes<...>("Stream", IntegerValue(seed*7919 + K))`, the pattern of `Tier1Tdoa` and `NlosScoring` (`scratch/ca_tbl_causal_sim.cc:4201`, `:4487`). No automatic stream index moves, and no engine draw is displaced.
6. **New outputs are digested.** Each new file is added at the `AppendOutputDigest` call site (`scratch/ca_tbl_causal_sim.cc:9221`) and written only under its flag. This does not close M.4, which covers the viz outputs.
7. **New `RunArm` inputs go through one struct.** `RunArm` already has 126 positional parameters, and the handoff warns about misordering and comma expressions.
   - Add a single `SpecOptions` struct parameter (last position) and put every new flag in it.
   - Its first commit is null-controlled on its own, because a new local once moved codegen by 1 ulp (TECH_DEBT 4.1).
8. **Commit discipline.** Atomic commits, with the evidence in the message and **no attribution trailer** (RULES.md §2, gate G12). Commit from WSL. Pre-register before any measurement run (rule 13).
9. **The §6 invariant is checked first in every energy-touching change.** Only the tier that transmitted this cycle is charged. Each relevant gate asserts that an unused tier's `j_` column does not rise.
10. **Each item lands with its own gate and negative fixture in the same commit** (RULES.md §4.4). G24 later only *aggregates* them. No item waits for G24 or G27 to be tested.
11. **Null controls cover the paths being edited, not just the defaults.** NC1 and NC2 never turn on `sparseCaseA`, `relayedRanging`, `reversePath`, `queryProxy`, `tier1Tdoa`, `sessionLayer` or `suspectRecovery`. So `~/catbl_ref_spec0/nc/` adds one config per opt-in path an item touches:
    - NC3 `--sparseCaseA`
    - NC4 `--relayedRanging --relayChain --nTags=4`
    - NC5 `--identityQueryPeriodS --queryProxy --reversePath` (NC2 flags)
    - NC6 `--tier1Tdoa`
    - NC7 `--sessionLayer --batchSearch`
    - NC8 `--suspectRecovery=inflate`

    Each is run with the new flag **off**.
12. **Behaviour replaced under an existing flag gets a new flag value.** For example, `--hopProvenanceForm=2`, `--reversePath=v2`, and `--pcsW*` ≥ 0 via a new sentinel. The old value is never edited in place, and the "previous binary" check applies.
13. **Output-file hygiene.**
    - Every new file and every new stdout line exists only under its flag. `nc.sh` requires identical file sets.
    - Per-subject traces are named `subjtrace_*`, **not** `trace_*`, so `verify_causal_sim.py`'s `trace_*` glob (and G2/G4) never sees them.
    - The preset sets `--vizEvents=true`, and `events_*` is added to the `run_id.txt` digest under the preset. This partly closes M.4, because several gates recount from `events_*`.
14. **`CaTblRangeMeasurement` changes once** (in DC-11). It gains `sourceKind` {SURVEYED_GATEWAY, FINDER, RELAY, PEER}, `anchorId`, `hopDepth` and `fromFloor`, all defaulting to today's values. Every later item reads these fields rather than re-extending the struct: each layout change risks the 1-ulp drift in TECH_DEBT 4.1.

---

## 1. The lacking parts, in detail

Each item follows the same layout:

- **Spec**: what the docx says, with its section.
- **Code**: what the code does, with a `file:line`.
- **Gap**
- **Steps**
- **Flag / preset**
- **Verification**
- **Effort**

Items are grouped by layer. Section 2 gives the build order.

### 1.A Physical layer

Already conformant, and on by default:
- 868 MHz RSSI log-distance ranging;
- 2.4 GHz two-way ToF with `t_silence` removed;
- UWB three-message TWR;
- ToA-derived airtime.

**PH-1: tier-1 TDoA combined with RSSI**
- **Spec.** Physical §1.1: "RSSI-based path-loss estimation, combined with TDOA where three or more anchors receive the same transmission", solved by Taylor-series or Chan.
- **Code.** Built at `7377ed0` behind `--tier1Tdoa` (default false): Chan–Ho seeding Foy for ≥4 gateways, and a closed form that keeps every root for exactly 3. The OK fix is fused beside the RSSI ranges (`Tier1Tdoa`, `scratch/ca_tbl_causal_sim.cc:4186-4425`).
- **Gap.**
  - It is off by default.
  - J.2 item 4, re-deriving tier-1 σ, is not done. The accuracy rests on a CRLB with no threshold effect.
  - Under the spec's 3-gateway geometry, every tier-1 fix is the exactly-3 closed form, whose two roots are separated by the cycle's RSSI ranges or else refused.
- **Steps.**
  1. The preset sets `--tier1Tdoa=true`.
  2. Within CAMP-SPEC, register a TDoA σ calibration: reported covariance against realised error on TDoA-fused cycles, the A4 method. Report the AMBIGUOUS/NO_ROOT refusal rate at 3 gateways.
  3. Do not change the default (owner decision #3 stays pending).
- **Effort.** Only the preset line and the campaign cell.

**PH-2: UWB CIR leading-edge NLOS score** (TECH_DEBT M.2, in flight)
- **Spec.** Physical §1.3: `NLOS_score = 1 − A_leading_edge/A_peak` on the CIR.
- **Code.** A synthetic 802.15.4a CIR is scored behind `--nlosClassifier` (`86e43f2`). The UWB confusion matrix has empty TP/FN cells, because no decoded UWB frame is NLOS in the engine.
- **Steps.**
  1. **P0.** Implement `--uwbNlosModel=legacy|ieee802154a` **exactly as `report/PREREG_UWB_NLOS.md` §2 specifies**, in `lora-uwb-physics-engine.cc` only:
     - one sampler draw u, the same one legacy makes;
     - `P_NLOS = 1 − exp(−d_2D / 44.81 m)`;
     - on an NLOS draw, `nlosDb = ΔPL(max(d_3D,1))` from the CM5/CM6 (or CM3/CM4 under `ENV_INDOOR_OFFICE`, as `CaTblCirModelFor` chooses) PL0/n/κ values in the prereg table, with no further draws;
     - `nlosProbability` = P_NLOS(d_2D);
     - no Markov +6/+18 dB (already inactive at `FIDELITY_ULTRA_FAST_STATISTICAL`).
  2. Null control: legacy is byte-identical to `~/catbl_prev_uwbnlos` (sha 964af077…). The 868/2.4 GHz engines and non-UWB static arms are byte-identical under ieee802154a.
  3. Freeze and hash the binary, then run the 40 registered cells into `~/catbl_uwbnlos/registered/` from a clean tree. Check P1–P5 and write `report/UWB_NLOS_RESULTS.md`.
  4. The preset turns on `--nlosClassifier --pcsNlosClassifier`. Which UWB channel model the preset uses is decided **after** P0's read-out (§5). The registration binds the *default* to `legacy` whatever the runs show.
- **Effort.** 1–1.5 days.

**PH-3 radio-part basis: see SY-2. PH-4 SF/BW: see SY-3.**

### 1.B Decision core: filter, sparse cases, PCS, integrity

**DC-1: the EKF prediction uses an IMU control input (synthetic IMU)** (decision 2)
- **Spec.** Physical §2.2:
  - `x_k|k-1 = F·x + B·u_k`, `P = F·P·Fᵀ + Q·Δt`, with `x = [x, y, vx, vy]`;
  - "u_k is the IMU-derived acceleration/heading correction";
  - Q is higher for field-node self-entries that rely on IMU drift.
  - Hardware: BMI270 or ICM-42688.
- **Code.**
  - `CaTblEkf::Predict(double dt)` is constant-velocity with no control input (`ca-tbl-policy.cc:207-259`); its only caller is `:950`.
  - Q is diagonal: `qPos²·dt`, `qVel²·dt` (`:236-239`).
  - An IMU exists only as the SHS gait engine (`src/lorawan/model/lora-shs-*`), which diverges (K.9). It feeds the filter only as `--imuFusion` (a position measurement on no-fix cycles, `scratch:8060-8074`) and the `--imuCrossCheck` witness.
  - A raw-sensor IMU error model does exist: `ImuSensorSubmodule`, `lora-imu-sensor-submodule.cc:32-81`, bias Gauss-Markov plus white noise, with MPU-6050 parameters cited in `lora-shs-parameters.h:37-77`.
  - The walker exposes only true per-cycle displacement (`pos − prevTruePos`, `scratch:7947`). `Tourist::m_heading` is private with no getter (`:1595`).
- **Gap.** The spec's prediction model does not exist.
- **Steps.**
  1. **New `src/lora-physics-engine/model/ca-tbl-imu-model.{h,cc}`: `CaTblImuDeadReckoner`.** It is a displacement-level PDR error model.
     - Per cycle it takes the true displacement Δ and dt.
     - It keeps a heading error, `ψ_k = ψ_{k-1} + b_g·dt + σ_ψ·√dt·N(0,1)`. The gyro bias `b_g` is drawn once per run; σ_ψ comes from the gyro noise density.
     - It draws a per-run scale error `s ~ N(0, σ_s)`.
     - It outputs `u_k = (1+s)·R(ψ_k)·Δ`.
     - It draws from **its own stream**, `seed*7919 + 1400071`, plus 97 × the subject index.
     - **Error covariance: the errors are correlated, and the covariance must say so** (review finding).
       - The per-run scale error and the bias-driven heading error make `u_k − Δ_k` strongly correlated from cycle to cycle. Treating a per-step Q as independent understates dead-reckoning drift by roughly 10× after 100 cycles.
       - The reckoner therefore tracks the **accumulated** displacement-error covariance since the last absolute fix, Σ_acc(k).
       - It uses heading variance `σ_ψ²(t) = σ_b²·t² + σ_arw²·t`, with the bias term squared, plus the scale term, both along the accumulated path.
       - It hands the EKF the increment `Q_u,k = Σ_acc(k) − Σ_acc(k−1)`, projected to be positive semi-definite. The marginal growth over a dead-reckoned stretch is then right. Cross-cycle correlation with earlier states is still ignored; that is stated.
       - Σ_acc resets when the filter fuses a full fix.
       - When Δ = 0 (the walker dwells), n̂ is undefined, so `Q_u = σ_w²·I` only.
  2. **Parameters.**
     - Gyro noise density and bias come from the **Bosch BMI270 datasheet** (BST-BMI270-DS000, read locally once poppler-utils is installed). The document number, revision and table are cited in the source.
     - If the owner prefers not to install tools, fall back to the MPU-6050 figures **already cited** in `lora-shs-parameters.h` (noise density 1.6682e-4 rad/s/√Hz; bias instability 8.816 °/h, Rudyk), and say so.
     - The step-length/scale error σ_s is an algorithm property, not a datasheet number: cite it from `docs/IMU_SHS_DESIGN.md`'s existing sources if one is there, or label it OURS.
  3. **EKF.** Add `CaTblEkf::PredictWithControl(dt, ux, uy, Qu)`:
     - Position += u, velocity := u/dt.
     - Covariance: `P_pp ← P_pp + Qu`, `P_pv ← Qu/dt`, `P_vv ← Qu/dt²`. The old velocity correlations are discarded, because velocity is now the IMU's.
     - Snapshot `m_xPrior/m_PPrior` exactly as `Predict` does (`ca-tbl-policy.cc:248-256`). Otherwise the integrity check compares against a stale prior.
     - Leave `Predict(dt)` untouched.
     - **Recorded deviation:** overwriting velocity with u/dt is a displacement-driven PDR prediction, not a literal `F·x + B·u` with the CV F. The docx's u_k is "acceleration/heading correction", which this model subsumes. Stated in the register.
  4. **Policy.**
     - Add `CaTblPolicy::SetImuControl(ux, uy, Qu)`, queued like `SetAuxPositionFix`. `Step()` then calls `PredictWithControl` when a control is queued, and `Predict` otherwise.
     - The scenario queues it each cycle from the reckoner under `--imuPredict`.
     - `Step`'s signature is unchanged.
  5. **Output.** `imu_<arm>_seed<N>.csv`: per cycle Δ_true, u, ψ, Qu. Written only under the flag and digested. Trace columns do not change.
- **Flag / preset.** `--imuPredict` (default false); the preset sets it true.
- **Verification.**
  - New unit suite `ca_tbl_imu_test`:
    - zero-error parameters give u = Δ exactly;
    - heading-error variance grows linearly with t across 10⁴ Monte-Carlo runs;
    - the dedicated stream leaves the default stream untouched (identical `RngSeedManager` draw counts);
    - `PredictWithControl` equals `Predict` **when `P_vv = P_pv = 0`**, u = v·dt and Qu = the CV Q. A general equality cannot hold, because `Predict` propagates F·P·Fᵀ.
    - The prior is snapshotted.
  - Null control at defaults.
  - Gate **G25**: residuals `u − Δ_true` in `imu_*.csv` match the modelled Σ_acc. A **NEES consistency check** over dead-reckoned stretches must fall inside the χ² 95% band, recounted in Python from the file. The EKF prior moved by u.
  - Negatives: a file with u = Δ (no error) claimed as the error model, the flag off, an inflated σ, and an independent-Q model (fails NEES).
- **Effort.** 3 days, plus the datasheet read.

**DC-11: sparse solving moves inside `Step()`, after the prediction**
- **Spec.** Physical §2.4: Cases A/B/C/D are resolved "against the node's own **propagated** prior".
- **Code.**
  - The scenario solves Case A/B **before** `Step()`, against the previous cycle's posterior (`policy.EstX/Pxx`, `scratch:7645-7646`), and fuses the result at t−dt via `FusePosition` (`:7663`).
  - `Step()` is then handed the same 2 ranges. It takes the `<3` early return (`ca-tbl-policy.cc:961-994`): `hadFix=false`, counted as a no-fix, **and counted toward demotion**.
  - The TDoA aux fix is skipped on that path.
- **Gap.**
  - The prior is the wrong one.
  - The fix is applied one cycle early.
  - A solved sparse cycle is reported as a no-fix and pushes the node down a tier.
- **Also an existing defect (review finding).** With `--sparseCaseA --relayedRanging`, Case A fuses a position from the two ranges (`scratch:7663`), and then `Step()` updates again on the same two ranges once C5 has appended relays: a double count. It goes into §1.H.
- **Steps.**
  - **What `Step()` needs and does not have today:** ids, provenance, which set it is (floor or active), and the deployment's surveyed list. The measurement struct gains these once (rule 14): `sourceKind`, `anchorId`, `fromFloor`, set in `rangeTier()` and the C5 push, with `sourceKind` from the id classes (101+ gateway, 501+ finder, 2+ peer or relay).
  - Behind `--sparseInStep` (default false; preset true), `Step()` gains the sparse branch after `Predict`/`PredictWithControl`. It acts on the **final** `meas` set, after C5:
    - 2 ranges: Case A (both `SURVEYED_GATEWAY`) or Case B (otherwise) via `CaTblSolveCaseA/B` (`ca-tbl-sparse-anchor.cc:119-142`) against `m_xPrior/m_PPrior`. The result is fused with the same conservative variance `2(s0²+s1²)`, and **the same ranges are not also used as range updates**. That closes the double count.
    - 1 range: Case D (DC-4).
  - Record `d.fixKind` ∈ {FULL, CASE_A, CASE_B, CASE_D, NONE}.
  - A sparse-solved cycle counts as delivered for demotion, matching `activeSolved`'s existing use of `caseAUsed` (`scratch:7680`).
  - The scenario's pre-`Step` block is skipped when the flag is on.
  - The census counters are kept, and read from `d.fixKind`.
- **Verification.**
  - Null controls with the flag off: NC1, NC2, NC3 (`--sparseCaseA`) and NC4 (relays).
  - With it on, the Case A/B population matches the old path's attempts when no relays are appended. With relays, the double count is gone: the number of updates per sparse cycle is 1 position fuse and 0 range updates, recounted.
  - Unit tests in the sparse-anchor suite.
  - Its own gate fixture: a run that fuses and range-updates the same pair.
- **Effort.** 1.5 days.

**DC-2: Case B orientation from the IMU heading**
- **Spec.** Physical §2.4: Case B is "SSS triangle anchored at the known GN, orientation resolved by IMU heading estimate rather than a second geometric constraint".
- **Code.** `ca-tbl-sparse-anchor.h:78-83` records "a DEVIATION FROM THE SPEC and is deliberate": the prior chooses, because the SHS heading is untrustworthy (K.9).
- **Steps.**
  - With DC-1 and DC-11 on, the Case B candidate choice uses the **IMU-propagated** prior: last posterior + u_k, with covariance P + Q_u. That is the IMU heading estimate carried into position, which is what orientation-by-heading means in this geometry.
  - Add a `solvedBy` provenance note (`CA_TBL_CASE_B` with `priorSource=IMU`).
  - Update the header comment: the deviation is closed under `--imuPredict`, and stays open without it.
- **Verification.** A sparse-anchor unit test where the CV prior picks the wrong mirror root and the IMU prior picks the right one. Case B counts appear in G24.
- **Effort.** Half a day after DC-1/DC-11.

**DC-4: Case D, single-peer/anchor IMU-assisted tracking (ABSENT)**
- **Spec.** Physical §2.4D:
  - only one peer/anchor range is available;
  - propagate with the IMU;
  - incorporate the single range as a measurement constraint on the propagated state;
  - the output is an approximate probabilistic estimate whose covariance is kept, so PCS falls naturally;
  - it transitions to Case B/A as more references appear.
- **Code.**
  - Nothing implements it. `Step()` discards 1-range sets (`ca-tbl-policy.cc:961-994`).
  - The scenario's census label "CASE D" at `scratch:7575` means something else: the ordinary ≥3-anchor solve. That is a name collision.
- **Steps.** Behind `--caseD` (default false; preset true; requires `--sparseInStep`), in `Step()`:
  1. When the final `meas` set holds exactly one range (the `fromFloor` field says which set it came from), after the (IMU-)prediction, run one scalar range update. `CaTblEkf::Update` already accepts n=1 (`ca-tbl-policy.cc:261-331`); use the tier's `varM2`, NLOS inflation included.
     - **Consistency guard.** Apply it only if the innovation passes NIS ≤ χ²₁,₀.₉₉ = 6.63, a distributional constant. A single range against a very wide P otherwise gives an inconsistent covariance. A refused update is counted.
  2. Mark `d.fixKind = CASE_D` and `hadFix=false`: it is not a position fix, but the covariance is live.
  3. **The cycle does not take the no-fix early return, nor the `!activeDelivered` demotion branch** (`ca-tbl-policy.cc:965-993`, `:1491`). Both would exit before any escalation decision, so Case D could never escalate, which is the review's finding. Instead, PCS is computed from the updated covariance and the normal escalation decision runs. The spec's "naturally reduce PCS" can then escalate, subject to the evidence gate's coverage prediction (decision 7). A Case D cycle does not count toward demotion. This is a stated design choice, OURS.
  4. It applies to a peer range once MS-1 makes peers measured subjects (the peer's estimate as a pseudo-anchor, σ inflated per C5).
  5. Census: a new bucket `caseDImu` is printed only under the flag. The existing "CASE D" label stays, to keep stdout stable. The name collision is documented in the register and in the census comment.
- **Verification.**
  - Unit tests: one range updates only the radial component of P; the NIS guard refuses a wild range; a Case D cycle reaches the escalation decision.
  - Null control.
  - Its own gate fixture now. G24 later requires `caseDImu > 0` in the preset (the 5 km geometry produces single-range cycles).
  - Recount from `d.fixKind` written to `imu_*`/`subject_summary`. There is no new trace column.
- **Effort.** 1.5 days.

**DC-3: Case C relative-shape tracking**
- **Spec.** Physical §2.4: 0 anchors and 2+ peers gives "relative-shape-only reconstruction (SSS, no absolute frame) — explicitly not used for absolute position, only relative offset/cohesion tracking".
- **Code.** `CaTblSolveCaseC()` always refuses (`ca-tbl-sparse-anchor.cc:144-154`). The scenario only counts `caseSolved[3]` (`scratch:7600-7609`).
- **Steps.** Behind `--caseCRelative` (after MS-1 and NW-0, because peers must be rangeable):
  1. On a cycle where a subject has 0 anchor ranges and ≥1 peer subject in range on the active tier, it ranges each such peer through the C5 path (direct engine call). This is billed with `AccountRangingExchange` on the active tier; §6 holds.
  2. **Pairwise ranges need sharing (review finding).** A subject measures only its own ranges, and MDS needs peer↔peer ones. Each subject's measured peer ranges ride its ENTRY_PUSH/RESPONSE as a carried `peerRanges[]` field, billed at 3 bytes per range (OURS layout).
  3. The **base station** (edge compute, where entries converge over the backhaul) assembles each cycle's pairwise set:
     - for a connected set of k ≥ 3 nodes, it reconstructs relative coordinates by classical MDS;
     - for k = 2, it reconstructs the distance only.
  4. Write `case_c_relative_<arm>_seed<N>.csv`: cycle, subject set, relative offsets up to a rigid transform, and cohesion as the max pairwise distance.
  5. **Never fused** into any EKF.
- **Verification.**
  - Unit test: MDS recovers a known shape up to a rigid transform.
  - The gate asserts **control flow only**: rows appear only on 0-anchor cycles, and no Case C output reaches `FusePosition`/`Update` (a counter stays 0). It does not assert "EKF columns unaffected", because peer ranging moves the default random stream (rule 8).
- **Effort.** 2–3 days.

**DC-5: PCS weights and thresholds calibrated by LOOCV** (decision 3; campaign CAL-02 in §3.3)
- **Spec.** Physical §2.5: `PCS = w1·(1/tr Σ) + w2·GDOP + w3·(1−NLOS)`, "w1, w2, w3 — empirically tuned weights, calibrated against the ground-truth dataset". The final audit's Gap B says to use LOOCV.
- **Code.**
  - The default `legacy` form is the unweighted sum `t1+t2+t3` (`ca-tbl-policy.cc:880-883`).
  - `weighted` is hand-set (2, 0.5, 0.5) (`:72-74`).
  - Thresholds are hard-coded at up 1.55/1.70/2.30 and down 1.75/1.90/2.55 (`:58-64`), with no CLI flag.
  - `--pcsW1..3` apply only when > 0 (`scratch:5236-5238`), so 0 is not expressible.
  - CAL-01 ran and returned (3, 0, 0) with no thresholds (`report/CAL01_LOOCV_RESULTS.md` §2).
  - The C++ `CaTblCalibrateLoocv` fits without an intercept (M.7).
- **Steps.** DC-6 first (flags), then CAL-02 as §3.3.
- **Effort.** 2 days of work, plus compute.

**DC-6: PCS flags, making the calibrated form expressible**
- **Steps.**
  - `--pcsThrUp868/--pcsThrUp2p4/--pcsThrUpUwb` and `--pcsThrDown...`. Their sentinel default is −1, meaning "keep the built-in values".
  - `--pcsW*` switch to a −1 sentinel so that 0 is allowed.
  - Add `--pcsForm=calibrated` (the weighted formula; refused unless all weights and thresholds are given explicitly).
  - Fix `--pcsForm`'s help text, which lists only legacy|v2.
  - Fix `--pcsHysteresisBand`'s help text, which claims a 0.20 gap on every tier while UWB's is 0.25.
- **Verification.** Null control; a unit check that the parsed thresholds reach `CaTblPolicyConfig`.
- **Effort.** Half a day.

**DC-7: a SUSPECT fix is downweighted in PCS, not rejected**
- **Spec.** Physical §2.6: `disagreement = D_M(fresh_fix, predicted_prior, Σ_predicted)`. If disagreement > threshold AND NLOS_score is low: "flag SUSPECT … do not let it alone justify a de-escalation; **downweight in PCS regardless of raw trace(Σ)**".
- **Code.**
  - A suspect fix is rejected: `RevertToPrior` plus `measurementRejected=true` (`ca-tbl-policy.cc:1185-1247`).
  - It forces an escalation: `suspectForcesEscalation`, default true (`:86`, `:1300-1312`).
  - It blocks the de-escalation count through `healthy = !suspect && …` (`:1560-1562`). That part is to spec.
  - Two defects found while reading:
    - `RevertToPrior` runs before any update in that cycle, so it is a **no-op**, and a rejected cycle is a plain coast. The header (`ca-tbl-policy.h:193-197`) says P is frozen, which is wrong.
    - `m_consecutiveRejections`, `m_reinitFallbackCount` and `m_recoveryFirings` are never initialised in the constructor (`:559-569`). They are zero today only because `std::vector<CaTblTagState>(n)` value-initialises them.
- **Steps.**
  1. Add a new `CaTblSuspectRecovery` value, `DOWNWEIGHT`, exposed as `--suspectRecovery=downweight`:
     - take the accept path (`Update` + `FuseAux`, as at `:1251-1255`), **with R inflated by `suspectInflateFactor`** (existing, 4.0).
       - A full-weight fuse would absorb a persistent fault into the prior within a cycle, and recall under the SY-5 fault matrix would then collapse by construction (review finding).
       - The recall consequence is pre-registered in CAMP-SPEC;
     - `measurementRejected=false`, `d.suspect=true`;
     - after `ComputePcs` (`:1269-1275`), apply a form-aware penalty: multiply PCS by `--suspectPcsFactor` (default 0.5, OURS, stated) for the sum/weighted/calibrated forms, and subtract `log10(1/factor)` for v2, whose scores can be negative. This is modelled on `ApplyProvenance`.
  2. The SHS and IMU trusted-fix feed stays gated on `!d.suspect` (`scratch:8183`), so a suspect fix never recalibrates the dead reckoner.
  3. The preset sets `--suspectRecovery=downweight --suspectForcesEscalation=false`. The spec never says a suspect fix forces escalation; the lowered PCS escalates through the threshold if warranted.
  4. A separate no-behaviour commit initialises the three counters and corrects the header comment. Null-controlled.
- **Verification.**
  - Unit test on the policy: a suspect fix is fused and PCS is scaled.
  - The integrity gate's existing fixtures are extended with one where a downweight run is offered as a reject run.
- **Effort.** 1 day.

**DC-8: integrity eligibility is "NLOS_score LOW", not "every link tagged LOS"**
- **Spec.** §2.6's condition is "NLOS_score is LOW".
- **Code.** `allLos = (d.nNlos == 0)` (`ca-tbl-policy.cc:1175-1180`), read from the `--nlosSource` tag. Under the default legacy tag, which marks about 75% of links NLOS, all-LOS cycles almost never occur. The eligible population is 0–9 per run (TECH_DEBT I.1 and L.2), so the check cannot fire.
- **Steps.** Behind `--integrityNlosFromScore` (requires `--nlosClassifier`):
  - eligibility is the mean NLOS score over the fix set's scored links being below `--nlosScoreThreshold` (0.5, OURS, already defined);
  - it falls back to the tag when no link is scored.
  - The preset sets it on.
- **Verification.**
  - Null control.
  - G24 requires eligible cycles > 0 and suspects > 0 in the preset, recounted from `nlos_links_*` joined to the trace.
  - Negative fixture: the flag on without the classifier is refused.
- **Effort.** Half a day.

**DC-9: the witness cross-check uses the synthetic IMU**
- **Spec.** The final plan item 2: before the confidence estimate may trigger escalation, cross-validate it against "IMU dead-reckoning consistency, neighboring-node corroboration".
- **Code.**
  - `--imuCrossCheck` compares the **SHS absolute position**, which diverges, with the EKF estimate (`scratch:7957-7963`). Its help text calls this a "displacement" check, which is inaccurate.
  - `--corroborationCheck` exists and has never refused in 28 runs.
  - Both are withdrawn in the default because the evidence gate absorbs every refusal (B3 C1).
- **Steps.**
  - Add `--imuWitnessSource=shs|model` (default shs). With `model`, the witness position is the DC-1 reckoner anchored at the last trusted fix and advanced by u_k.
    - **Independence (review finding).** Under `--imuPredict` the EKF prior already contains u_k, so comparing the IMU track to the *EKF estimate* would compare the IMU with itself.
    - The witness therefore compares the IMU track against the cycle's **raw radio fix**: `RawMultilateration` of the fix set, which is independent of the IMU, as the spec intends.
    - The tolerance is `K·sqrt(trace(R_fix) + trace(Σ_acc))`, replacing the `imuDriftMps` constant.
  - The preset sets `--imuCrossCheck --corroborationCheck --imuWitnessSource=model`.
  - **The evidence gate stays on in the preset** (decision 7). The witness's refusals are then mostly absorbed by it, as B3 C1 found.
  - To keep that visible, the witness's own counters are split: evaluations, refusals, and refusals already suppressed by the evidence gate. They are printed and written to `subject_summary.csv`. An absorbed refusal is reported as absorbed, never as a pass.
- **Verification.** G24 requires witness evaluations > 0 and prints the three counts with their denominator. It does **not** require un-absorbed refusals > 0, because decision 7 makes that population structurally small. The register records why.
- **Effort.** Half a day after DC-1.

**DC-10: table entries carry velocity and hop count**
- **Spec.** Physical §2.1: `E = {pos, Σ, timestamp, seq_no, hop_count, source_type, velocity_estimate}`.
- **Code.** The wire format carries velocity (`ca-tbl-header.h:178-225`), but the casualty publishes `0.0, 0.0` (`scratch:6435-6437`). Gateways do too (`:5543`); a gateway's 0 is correct. The pseudo-anchor comment confirms "the entry's velocity fields are not populated".
- **Steps.**
  - Behind `--entryVelocity`, publish the EKF velocity. Add `CaTblPolicy::EstVx/EstVy` getters over `m_x[2..3]`.
  - Finders publish their true per-cycle displacement / dt, which a GNSS receiver would give.
  - `hop_count` is already set (0 at the producer, incremented on relay).
  - The preset sets it on.
  - Nothing reads `vx/vy` today. The loctable/viz outputs and `pack_demo.py` omit them, and the 28-byte entry already bills them. So populating them moves no output file, and the null control holds by construction. It becomes observable through NW-9's RESPONSE contents and TA-1's `session_*`.
- **Verification.** Null control. G27 checks a delivered RESPONSE carries a non-zero velocity for a moving subject.
- **Effort.** 2 hours.

**DC-13: table entries are predicted forward with a source-type Q** (Physical §2.2: "Q ... set higher for FN self-entries relying purely on IMU drift, lower for GN entries (which don't move)")
- **Code.**
  - A held entry is never propagated. The pseudo-anchor substitution inflates it isotropically by `(walkSpeedMps·age)²` (`scratch:7352-7382`), because velocity was never populated.
  - Gateways are not substituted, being surveyed, so their Q is effectively 0.
- **Steps.** Behind `--entryPrediction` (preset on; requires DC-10's populated velocity):
  - A held finder or peer entry is propagated as `x + v·age`, with covariance `P + Q_src·age`.
    - `Q_src` is 0 for gateways.
    - For finders, a GNSS-velocity Q.
    - For field-node entries, the walk-speed bound as today.
  - The Q values are OURS and stated.
  - The isotropic inflation stays the `--entryPrediction=false` path.
- **Verification.** Unit test on propagation; null control; its own gate fixture: an entry with v≠0 must move.
- **Effort.** 1 day.

**DC-12: already conformant, or conformant once the preset is on**
- Tier-specific R: conformant.
- Hysteresis with an N-cycle dwell biased toward precision: conformant.
- GDOP term: conformant.
- Sequence-number supersession and Covariance Intersection (§2.7): built. The preset turns on the digest exchange and `--tableFuseCI` (§1.C MC-3). Their population is reported by G24; CI equal-sequence arrivals have been near 0 (L.2), and with 5 subjects and multiple relays that may change. If it stays 0, that is recorded as a finding, not faked.

### 1.C MAC / link layer

Already conformant, and on by default:
- WPC G.S.R. 853(E) Table-II duty 10% / 2.5%, enforced (`duty_868` 0.0226);
- the APC loop (`CaTblMac::UpdateApc`, `ca-tbl-mac.cc:399`, fed on delivery `:1154`);
- the Semtech ToA formula;
- CAD-based CSMA/CA with priority contention windows;
- flood dedup, hop limit and rebroadcast jitter;
- the 9-byte header and message taxonomy (`ca-tbl-header.h:70-82`);
- UWB randomised-wait initiation (`ca-tbl-mac.cc:909-916`, 0–10 ms);
- the 2.4 GHz ranging exchange modelled as a directed exchange with a CAD charge.

**MC-1: the escalation handshake is really on the air and reliable** (MAC §8: "Tier escalation request (UWB specifically): Yes [ACK'd] — Must know definitively whether the grant was received")
- **Code.**
  - The frames are sent only under `--sendHandshakeFrames` (default false, `scratch:9406`). Otherwise the handshake is only billed.
  - Defects found:
    - (a) An ACK clears the **oldest** pending frame, not the one with the matching packet id (`ca-tbl-mac.cc:1433-1436`).
    - (b) The anchor ACKs an ESC_REQUEST **before** arbitration runs (`:1162-1168` comes before `:1298`), so a denied request is acknowledged and never retried. The header comment at `ca-tbl-header.cc:301-304` claiming retries is wrong.
    - (c) The pre-emption send path (`scratch:7503-7507`) sends ESC_REQUEST with no `SetRequestTerms`, so it carries 0/0.
    - (d) `pcsDegradation` and `batteryUsedFrac` are updated at `scratch:8560-8588`, after the send, so every request carries the previous cycle's terms.
    - (e) `--uwbArbitration` does nothing without `--sendHandshakeFrames`, and nothing refuses that combination.
    - (f) `PeerRequestDriver` transmits even with handshake frames off.
- **Steps.** Behind `--handshakeFixes` (default false; the preset sets it true), because each fix moves frames:
  1. Match an ACK by packet id.
  2. Arbitrate first, then answer with ESC_GRANT carrying granted/denied plus `queue_pos` (a byte already billed, `ca-tbl-header.cc:288-290`). ACK the GRANT.
  3. `SetRequestTerms` at all three send sites.
  4. Compute the terms before the send.
  5. **ESC_REQUEST only for escalations into UWB** (spec MAC §8: "Tier escalation request (UWB specifically)"). Today it is also sent on de-escalations (`scratch:8228`) and on 868→2.4.
     - The request carries its **target tier**.
     - A de-escalation sends nothing ("Demotion is never deferred", Architecture §9.4).
     - The handshake energy charge follows what is actually sent, so energy moves under the flag, which is stated.

  One further change needs no flag: FATAL for `--uwbArbitration` without `--sendHandshakeFrames` (a refusal, not a behaviour change). Retiring `PeerRequestDriver` belongs to MS-1 (P3), not here.
- **Verification.**
  - The unit suite `ca_tbl_packet_validator` gains an ACK-by-id case and a deny-then-retry case.
  - Null control at defaults.
- **Effort.** 1 day.

**MC-2: Triggered-update / last-gasp** (MAC §3 and §8: "Triggered-update / last-gasp — large correction or dying battery", no ACK, 2–3 jittered repeats)
- **Code.** The only sender is `StartHandshake`'s RECEDING branch (`ca-tbl-mac.cc:1755-1763`), with 3 repeats (`:159`, `:857-860`). There is **no large-correction trigger and no last-gasp**. The scenario never prints a count (M.9). The comment at `scratch:8361-8362` still says the branch never runs.
- **Steps.** Behind `--triggeredUpdates` (preset on):
  1. **Large correction.** When the posterior moves more than `k·sqrt(trace P_prior)` in one cycle (k=3, OURS), send TRIGGERED_UPDATE carrying the self-entry on the active tier.
  2. **Last gasp.** When `--batteryDepleteJ` is crossed, send one TRIGGERED_UPDATE **immediately and unjittered**, and schedule `SetSilent` (`scratch:8115-8126`) for after that frame's airtime.
     - Otherwise the jittered repeats are blocked by `SetSilent` (`ca-tbl-mac.cc:788`, `:857-860`), which is the review's finding.
     - The transmission is billed to the subject's account on the tier it rides. A battery that crosses mid-cycle may overshoot by one frame; that is stated.
  3. Print sent, repeats and delivered counts per trigger on stdout (flag-gated lines), and write them to `triggered_<arm>_seed<N>.csv` (digested). They migrate into `subject_summary.csv` once MS-1 lands (M.9 closed).
  4. Fix the stale comment.
- **Verification.** G24 counts triggers per kind. A validator case covers the last-gasp ordering (sent before silenced).
- **Effort.** Half a day.

**MC-3: anti-entropy exchange and sequence supersession run** (Physical §2.7; MAC §9)
- **Code.** At defaults nothing digests: there are no finders and `sendHandshakeFrames=false`, so network-wide merges are 0 (`scratch:9071-9076`). `StartHandshake`'s DIGEST is sent **ungated** and never answered (`scratch:8385-8392`).
- **Steps.** The preset sets `--sendHandshakeFrames --bidirectionalDigest --digestSummary --tableFuseCI` with `--finderDigestPeriodS=20`, the NC2 value used everywhere in this plan. G24 reports merges, supersessions and CI fusions with their denominators.
- **Effort.** Only the preset.

**MC-4: mobility-state message selection.** The preset sets `--mobilityFromRange`. It is already built (`50a229a`); MC-2 adds the counter.

### 1.D Multi-subject: five measured field nodes (C2-full completion). THE LARGEST ITEM

**MS-1: every field node is a measured subject**
- **Spec.** Network §1: every node has an identity. Transport §5, BATCH_SEARCH: "Searching for a whole missing team"; "a five-person search naturally produces five competing escalation requests". The deployment Q2 in the final audit is "3 gateways, 5 field nodes". Every field node is localised.
- **Code.** The per-tag data structure exists and was merged at `abbe3de` (2026-09-06). `CaTblTagState` at `scratch/ca_tbl_causal_sim.cc:2627-2701` holds the walker, `CaTblEnergyModel`, `CaTblPolicy`, `nodeMac[3]`, counters, errors, sessions, and the per-tag C4 terms `pcsHist`/`pcsDegradation`/`batteryUsedFrac`. The behaviour, though, is tag 0 only:
  - The cycle body is not looped over tags. `:5082-5137` aliases every old name to `tags[0]`.
  - The escalation state machine, tier-1 floor, CAD/sleep/listen billing, handshakes, edge downlink, sessions, battery depletion, TDoA, NLOS scorer, IMU and trace all run for tag 0 only.
  - Tags 1..n-1 are either protocol peers with no policy (`:5560-5636`) or, under `--relayedRanging`, fixed-tier relays (`:5712-5744`). A relay ranges on `--relayTier` (default 2.4 GHz), is stepped with its decision discarded (`:6932`), never escalates, and has no tier-1 floor.
  - Relay energy models are not configured like tag 0's (`:5204-5209` only for tag 0). They keep the class defaults `m_tier1ChargeReplies=false` and `m_edgeServer=false` (`ca-tbl-energy-model.h:380-381`), and get no sleep/idle/CAD/handshake/downlink charges. `AccountSleep` is called only for tag 0 (`:8468`).
  - Output is tag 0 only: `arm_summary.csv` (header `:11439+`), plus one stdout line per relay (`:9142-9158`).
  - Synthetic peer escalation requests carry constants `rd.degradation = 128; rd.battery = 128` (`:5703-5704`).
- **Gap.** Four of the five field nodes are neither localised by the full pipeline nor reported. Every multi-target mechanism (arbitration, batch search, gossip tie-break) therefore runs against synthetic load.
- **Steps.**
  1. **Refactor the cycle body into a per-subject function.** Move the per-tag portion of the loop — from "node moves" through `rangeTier`, the floor, Case A/B, C5, `Step`, the edge downlink, battery, sessions, the SHS/IMU witness and the escalation handshake — into `static void __attribute__((noinline)) RunSubjectCycle(CaTblTagState& s, SubjectContext& ctx, ...)`.
     - Keep the `tags[0]` aliases so the diff is a move, not a rename.
     - Do this in its own commit with **no behaviour change**. The null control must be byte-identical: arm_summary and every trace. It must also pass at `--nTags=3 --relayedRanging`, `--relayChain` and `--fixRouting`, compared against `~/catbl_ref_spec0`.
     - Watch TECH_DEBT 4.1: stack-layout changes have produced 1-ulp trace drift before. Use `noinline` and keep locals in the struct. If the traces drift by an ulp while arm_summary is identical, stop and diagnose before going on.
  2. **Add `--measuredSubjects=N`** (default 1, flag in `SpecOptions`). Subjects 1..N-1 (tag ids 2..N) run `RunSubjectCycle` exactly like tag 0:
     - their own escalation state machine;
     - the tier-1 floor;
     - full energy configuration via the same `SetLoRaPhy`/`SetTier1ChargeReplies`/`SetEdgeServer` calls, plus `AccountSleep`/`AccountListen`;
     - their own ESC_REQUESTs carrying their real `pcsDegradation`/`batteryUsedFrac`, which retires `PeerRequestDriver`'s constants when this flag is on.
     - **A tag that is both a measured subject and a relay** (every relay, once `nTags = measuredSubjects = 5`):
       - It relays with its current fix, and the relay path reads `tags[pt].policy`, which is now the subject's own filter.
       - The relay block's **own ranging, `AccountRangingExchange` and `Step` are skipped** for such a tag (`scratch:6821-6936`). Otherwise it would be ranged, charged and predicted twice per cycle, which breaks §6 (review finding).
       - Its C5 legs toward other subjects still run.
     - **Per-subject components.** Each subject gets its own `ShsDriver`/SHS engine, `CaTblImuDeadReckoner` (DC-1), `Tier1Tdoa`, `NlosScoring`, battery state and session feed.
       - Each takes a dedicated random stream offset by 97 × subject index from tag 0's constants, so tag 0's streams are untouched.
       - Retire `PeerRequestDriver`'s constant 128/128 requests when `--measuredSubjects > 1`.
     - **Finder tasking** (spec: rescuers search for specific people). Under `--finderTasking=roundrobin` (preset), finder k steers toward subject `(k mod N) + 1`'s freshest entry, instead of always tag 0 (`scratch:6639`, `:6699`). The assignment rule is OURS.
     - Refuse `--measuredSubjects > --nTags`.
  3. **Per-subject outputs in NEW files only**, so tag 0's `arm_summary.csv` and traces stay byte-identical:
     - `subject_summary.csv`: one row per (arm, seed, subject), written **only when `--measuredSubjects > 1`**. It has the arm_summary error/energy/tier/escalation columns plus `subjectId`, produced by the same writer function, factored out.
     - `subjtrace_<arm>_seed<N>_subj<id>.csv` for subjects 2..N, under `--trace`. It is deliberately **not** named `trace_*`, so the verifier's glob and G2/G4 are unaffected (rule 13).
     - Both are digested in `run_id.txt`.
  4. **Channel/RNG order is fixed and documented.** Subjects step in id order within the cycle.
     - Under `--measuredSubjects > 1`, **all** subjects, subject 1 included, range in their own slots of a subject round (the `RelayRound` idea, `:2767`). Every subject then sees the same interference model, which the review noted would otherwise differ for subject 1.
     - The round sits in the slice the MAC has already run, a known limit from M.1, restated here.
  5. **§6 invariant per subject.** Each subject's account charges only its own transmissions. The gate checks that per subject, an unused tier's `j_` is 0.
- **Flag / preset.** `--measuredSubjects` (default 1); the preset sets 5, with `--nTags=5`.
- **Verification.**
  - Null control at `--measuredSubjects=1` (default) against `spec0`.
  - Gate **G22** (subjects):
    - `subject_summary.csv` has N×arms×seeds rows (a row count, not a non-empty test; rule 12);
    - subject 1's row equals arm_summary's row column for column;
    - every subject has `steps == duration/dt`;
    - each subject's `fixCycles` is recounted from its trace with awk (rule 3);
    - unused-tier energy is 0.
  - Negative fixtures: a dropped subject row, subject 1 ≠ arm_summary, a charged unused tier, `--measuredSubjects=1` offered as a 5-subject run, and a relay-subject stepped twice in one cycle.
- **Effort.** 7–10 days. This is the riskiest item: the loop body spans `:6200-8600`, and `rangeTier` alone captures about 20 `RunArm` locals.

**MS-2: the gateway geometry, chosen by study for SAR with the least infrastructure** (decision 5; campaign GEO-01 in §3.4)
- **Spec.** "3 gateways, 5 field nodes, 5 km radius". The docx names no shape. The owner asks for the shape best suited to SAR using the least infrastructure.
- **Code.** Placement options in `BuildDeployment` (`scratch/ca_tbl_causal_sim.cc:194-397`):
  - grid, `--gwGrid` (default 3×3 over ±0.9·boxHalf);
  - ring, `--nGateways --gwRingFrac`;
  - arc/crescent, `--gwArcDeg --gwArcCentreDeg`;
  - ridge/line, `--gwRidge --gwRidgeY`;
  - base camp, `--baseCampFrac` (the deployment carries `hasBaseCamp` and `baseCampX/Y`, `:159-164`).
  - The box is square, ±boxHalf.
- **Prior evidence.** HANDOFF_2026_09_09 §4 at seed 7:
  - interior grid: cep50 28 m, rejected as unrealistic;
  - ridge: cep50 1932 m, degenerate;
  - the arc was built as the plausible access-route deployment, and NC2 uses 5 gateways on a 140° arc centred west at `boxHalf=2000`.
  - No study at the spec's scale and gateway count exists. `docs/DESIGN_DECISIONS.md` §5 still says "not built" (stale since `089a9f1`), and the grid/ridge/arc comparison was never filed there (HANDOFF §8).
- **Gap.** Nothing decides the shape, and nothing measures "least infrastructure".
- **Steps.**
  1. **Code: new placement shapes** behind `--gwLayout=legacy|perimeter|cluster|star|lshape|file` (default `legacy` = today's grid/ring/arc/ridge flag logic, byte-identical):
     - `perimeter`: N points evenly spaced along the square boundary at fraction f (`--gwLayoutFrac`), i.e. the "box".
     - `cluster`: N points within radius ρ of the base camp (`--gwClusterRadiusM`), the least-mobile deployment.
     - `star`: arms radiating from the base camp along access tracks (`--gwStarArms`, spacing `--gwStarSpacingM`).
     - `lshape`: two perpendicular access roads meeting at a corner.
     - `file`: explicit coordinates from `--gwLayoutFile` (CSV x,y), used for the optimiser's output.
     - Every shape is placed at the 15 m mast height and carries all three radios (topology 0).
     - Null control with `legacy`. A unit check of each shape's coordinates.
  2. **An optimiser for free placement** (`scripts/geo_optimise.py`, analysis only). It greedily places N gateways to maximise the stage-1 criterion (below). The candidate positions are restricted by the **realism constraint**: the box perimeter band plus declared access routes, or unrestricted for a reported, flagged upper bound.
  3. **Run GEO-01** (§3.4): stage 1 geometric screening, stage 2 registered simulation, stage 3 confirmation under the full preset.
  4. The preset takes the winner's flags.
  5. Record the study and the winner in `docs/DESIGN_DECISIONS.md` §5, replacing the stale text and filing the old grid/ridge/arc comparison.
- **Physics to expect** (from `r80`/`r50`): 868 r80 ≈ 1515 m and r50 ≈ 3217 m. At 3 gateways over a 5 km radius, much of the area has fewer than 3 tier-1 gateways, so Cases B/C/D carry the load, as the docx predicts. More gateways or a better shape buy tier-1 coverage. The study measures that trade.
- **Effort.** Code 1 day; campaign 1–2 days of compute.

**MS-3: rescuers carry UWB** (decision 6)
- **Spec.** Identity classes include RESCUER. "A rescuer's handheld" issues queries. UWB arbitration runs at anchors "within current UWB range".
- **Code.** Finders (ids 501+) are mobile tri-band nodes with identity class RESCUER (`scratch:5866`). `--finderMode=2` is the two-phase search, the NC2 regime.
- **Correction (verified 2026-09-30).** The NC2 regime does **not** reach UWB as configured. Its reference run (seed 7, 1800 s) has `cycles_uwb=0`, `meanNbr_uwb=0` and `trig_tn=720`. The reasons:
  - The evidence gate's default coverage predictor uses a gateway-only anchor map (`scratch:5225-5229`) and needs 3 UWB anchors within 20 m (`ca-tbl-policy.cc:1462-1485`), which never happens.
  - The ground-truth label ignores finders, so `shouldUwb` is structurally false.
- **A second observation (2026-09-30, while building G28).** Reaching the UWB *tier* is not
  the same as getting a UWB *fix*. In the null-control config NC9 (`--nFinders=4
  --finderMode=1 --boxHalf=750 --uwbCoverageIncludesFinders=true
  --escalationRequiresEvidence=false`, seed 1, 600 s), `ca_tbl_full` spends 155 cycles on
  UWB, and `nbrUwb = 0` on every one of them. Every one is fixed by the tier-1 floor and
  leaves UWB by blind demotion. So no PCS threshold at UWB is ever read, and G28 cannot
  observe the UWB pair. The P5 checkpoint must therefore require **UWB-delivered fixes**
  (`fixTier = UWB`), not just `cycles_uwb > 0`.
- **Steps.**
  - The preset sets `--nFinders=4 --finderMode=2 --finderDispersal=1.0 --finderPseudoAnchor=true --finderDigestPeriodS=20`, and **also**:
    - `--uwbEvidenceFromNeighbours=true`: the evidence gate predicts from the measured range on the awake radio (TECH_DEBT I.2: tier-3 cycles went from 0.0 to 5.4);
    - `--uwbCoverageIncludesFinders=true`: the trigger label counts finders (TECH_DEBT H.1).
  - Finders arbitrate their own UWB radio as mobile anchors (NW-5). Finders can be session requesters (TA-1b), and are tasked per subject (MS-1).
  - **Checkpoint before P6.** A short preset probe must show `cycles_uwb > 0` **and UWB-delivered fixes** (`fixTier = UWB` in the trace) with the evidence gate on (decision 7). If it is still 0, stop and bring it to the owner: decision 7 would then leave every tier-3 item (PH-2, NW-5, NW-7, TA-3, TA-4, DC-2) without a population.
- **Effort.** Only the preset, plus the checkpoint probe.

### 1.E Network layer

**NW-0: real mesh links, every radio pair can hear every other (PREREQUISITE for the whole network/transport layer)**
- **Spec.**
  - The system is a mesh: "sub-GHz LoRa mesh".
  - QUERY is "Flooded through the mesh".
  - Proxy responses come from "any node that has recently seen the target".
  - RESPONSE travels "hop by hop" along the reverse path.
  - Anchors "gossip their current Q ... to neighboring anchors".
- **Code.** `EnginePhyProvider::Evaluate` resolves a link only between a field node and an anchor (`scratch:608-631`, `:695-760`). Tag↔tag, finder↔gateway and gateway↔gateway pairs return "not received". The one exception is GRANT_GOSSIP between gateways (`EvaluateFrame`, `:842-855`).
  - A finder's proxy RESPONSE can never reach a gateway.
  - A casualty can never reach a gateway through a peer.
  - The MAC's flood is therefore star-plus-anchor-reflection, not a mesh (TECH_DEBT 3.5: "every anchor is a hidden terminal").
- **Gap.** The mesh the spec describes cannot exist on this PHY substrate.
- **Steps.** Behind `--meshLinks=star|all` (default star; preset all):
  1. `Evaluate` resolves any registered pair on a tier both nodes carry:
     - tag↔tag, where both are field nodes, positions from `m_peerPos`/`m_nodePos`;
     - anchor↔anchor, generalising `EvaluateAnchorPair`;
     - finder↔gateway.
     - The transmitter goes in the engine's node slot; heights come from position z (as `EvaluateAnchorPair` already does, `:880-937`).
  2. **Beacon storm guard.** Under `all`, DISCOVERY becomes one-hop (hop limit 1), because neighbour discovery is one-hop by definition (MAC §8: "next beacon supersedes"). QUERY and TRIGGERED_UPDATE stay flooded, with the existing dedup and jitter.
  3. **Phantom-anchor guard (review finding, HIGH).**
     - `NoteNeighbour` admits the originator of any first-hop frame (`ca-tbl-mac.cc:442-485`), and the poll set *is* that neighbour table (`scratch:7154`).
     - `rangeTier` reads `anchorPosById[anchorId]` with `operator[]` (`:7175`, verified). So a peer id would insert and range against a free "anchor" at (0,0,0).
     - Under `all`, the poll loop uses `find()` and polls **anchor-class ids only** (gateways and finders). Peers are ranged only through C5 relayed ranging, and later Case C.
     - The same `find()` guard is applied at the relay path's lookup (`:6853`).
  4. **Sleep gating (review finding, HIGH, §6).**
     - `CaTblMac` has no sleep gating; only `SetSilent` switches a MAC off. Under `all`, a tag's *sleeping* 2.4 GHz/UWB MACs would receive and rebroadcast peers' floods on tiers the arm never used, unbilled.
     - So a subject's MAC on a tier that is not awake this cycle neither receives nor rebroadcasts. Awake means the active tier, plus 868 while the floor keeps it awake on hybrid arms.
     - A relay rebroadcast on an awake tier is **billed** to the subject's account with `AccountFrameTx`.
  5. This multiplies link evaluations (roughly N² per frame). Report `GetEvaluations()` and wall time in the commit, and measure gate-suite runtime (K.6's concern). The engines also keep per-direction link state when a tag sits in the gateway slot; the fading/shadow caches key on the ordered pair, which is stated.
  6. Gateways stay unbilled, as today. Finders carry no account, so a finder relaying pays nothing, which is stated.
- **Verification.**
  - Null control with `star`.
  - Gate **G26**, **one-hop in P3** (multi-hop RESPONSE needs NW-8): under `all`, a finder↔gateway frame and a tag↔tag frame are delivered; no peer id enters a poll set; a sleeping MAC delivers nothing; dedup keeps rebroadcasts ≤ nodes × hop limit per packet id.
  - Negatives: `star` offered as `all`, a DISCOVERY rebroadcast counted, a peer polled as an anchor, and a sleeping-tier rebroadcast.
- **Effort.** 3–4 days.

**NW-8: the receiver knows the previous hop, so the reverse path really exists** (Transport §1)
- **Code.**
  - `DeliverRx(hdr, out)` gets no transmitter id (`ca-tbl-mac.cc:1725`). Relays keep `origin_id` (`MaybeRebroadcast`, `:1472-1491`).
  - `RememberReversePath(queryId, hdr.GetOriginId())` therefore caches the *originator*.
  - `SendResponse`'s hit and miss branches send the same one-hop frame to the same destination: a **no-op** (`:711-732`). The "flood fallback" is not a flood, because RESPONSE is not a flooded type.
  - `reversePathWindowS` (300 s, `mac.h:296`) is never read.
- **Steps.** Behind `--reversePath` (existing flag; its implementation becomes real, so the old behaviour moves behind `--reversePathLegacy`, or the flag is renamed with the old value kept):
  1. Pass the transmitter id into `DeliverRx`; the channel knows it.
  2. Add a carried (not serialised, like the other payload fields) `prevHop` that relays set.
  3. Cache `reverse_path[query_id] = prevHop` under the time window.
  4. The RESPONSE is unicast to `reverse_path[query_id]` at each hop, and each intermediate node forwards using its own entry.
  5. On a miss, the RESPONSE is flooded with the QUERY's remaining hop limit (spec: "fall back to flood"). Flooding is currently a **per-type** property (`CaTblIsFlooded`), so this needs a carried per-frame `forceFlood` bit honoured by `MaybeRebroadcast`.
  6. Counters: unicast hops, misses, floods and delivered RESPONSEs, printed and digested in `session_*` (TA-1).
- **Verification.**
  - The netlayer unit suite gains a 3-hop chain: RESPONSE follows the reverse path, and a missing cache entry floods.
  - G26 extension: in a preset run, delivered RESPONSEs > 0 and unicast hops > 0.
- **Effort.** 1 day (after NW-0).

**NW-9: QUERY and RESPONSE carry what the spec says**
- **Spec.**
  - `QUERY = {query_id, target_manifest_id_list[], requester_id, ttl, priority}`.
  - A matching node "respond[s] directly with own position table entry". The proxy answers "on behalf of that entry". A node rebroadcasts only when neither applies ("else").
- **Code.**
  - QUERY carries a single manifest id. The priority byte is billed (`ca-tbl-header.cc:292-294`) but not carried.
  - A RESPONSE carries **no entry and no query id**, and its manifest is 0 (`ca-tbl-mac.cc:799-816`, `:845`). **Nothing handles RESPONSE at the requester.**
  - The matched target and the proxy still **rebroadcast** the QUERY (`:1441-1457`).
- **Steps.** Behind `--queryV2` (preset on):
  1. QUERY carries the manifest list (billed `6 + n`, already the formula), plus priority, requester id, query id and ttl (= hop limit), all carried.
  2. RESPONSE carries:
     - the answered entry: the full table entry, incl. seq, hop, source type and velocity;
     - the query id and manifest;
     - tier_source, PCS and an integrity flag;
     - **battery-remaining and degradation terms**, which TA-2 needs to form an escalation request.

     The billed size is the existing entry size (28 B) plus a header, so airtime moves only under the flag.
  3. **Rebroadcast rule, batch-safe (review finding).** A node answers every listed manifest it can: itself as target, or others by proxy. It suppresses the rebroadcast **only if it answered every manifest in the list**. Otherwise it rebroadcasts with the answered manifests removed. A blanket "don't rebroadcast" would stop target 1 from passing a 5-manifest batch query on to targets 2–5.
  4. Add a RESPONSE handler callback on MACs, used by the requester (TA-1).
  5. The table entry gains carried `integrityFlag` (CLEAN/SUSPECT), `pcs`, `batteryRemaining`, `degradation` and `uwbActive` / `grantedBy` (for TA-3), set by `PublishSelfEntry` from the last decision.
- **Verification.** Netlayer and packet-validator cases for the list match, the proxy answer content, and "no rebroadcast after answering".
- **Effort.** 1 day.

**NW-3: the QueryDriver binds each tier to that tier's radio** (TECH_DEBT L.2 side finding)
- **Code.** Every tier's driver is bound to the first ANCHOR MAC in `macs`, which is gateway 101's 868 MAC (`scratch:5658-5680`). So a "2.4 GHz" or "UWB" query is an 868 frame evaluated and timed as another tier.
- **Steps.**
  - Behind `--queryTierBinding=first|per-tier` (default first; preset per-tier), record `anchorMacByTier[ti]` at construction (after `chan[ti].Register(am)`, `:5544`) and bind driver ti to `anchorMacByTier[ti].front()`.
  - When the base-station requester (TA-1) is on, the drivers are replaced by the requester's queries.
- **Verification.**
  - Null control (queries are off at defaults).
  - With `first`, reproduce `results/` C3 cells byte-for-byte.
  - With `per-tier`, the frames of each driver appear on their own channel (the `events_*` tier column).
- **Effort.** 2 hours.

**NW-4: query-triggered escalation bypass (ABSENT)** (Network §4: "a QUERY match with priority == URGENT can itself trigger UWB tier escalation for that specific target (bypassing the normal PCS threshold-crossing trigger)"; Transport §4 marks `request.is_query_triggered`)
- **Code.** There is no path from a received QUERY to escalation (`ca-tbl-mac.cc:1196-1231` only responds). ESC_REQUEST bills a `query_triggered(1)` byte (`ca-tbl-header.cc:281-286`), but nothing carries it. The arbiter's T_query is the constant access category (`ca-tbl-mac.cc:554-557`).
- **Steps.** Behind `--queryTriggeredEscalation` (preset on; requires `--queryV2`):
  1. On an URGENT QUERY matching its own manifest, the MAC latches `urgentQueryPending`. The scenario polls it each cycle for that subject.
  2. `CaTblPolicy::RequestQueryEscalation()`: next `Step()`, `wantEscalate = true` regardless of PCS (spec: bypass the PCS trigger).
     - **Reachability (review finding).** The hook must act before the `<3` early return (`ca-tbl-policy.cc:961`) and before the `!activeDelivered` demotion branch (`:1491`); both precede the escalation branch today.
     - The hook is evaluated in the restructured decision path that DC-4 introduces.
     - Hook firings and actual escalations are counted separately, so a structural zero (no UWB in reach) shows as fired-but-suppressed. The tier steps up once per cycle toward UWB, like normal escalation. Suppress-only gates still apply: the evidence gate stays on (decision 7), so an URGENT query into an area with no predicted UWB coverage is still suppressed, and it is counted as such. Static arms ignore it, being single-band by definition.
  3. The resulting ESC_REQUEST carries `isQueryTriggered=1`.
  4. The arbiter uses `T_query = isQueryTriggered` instead of the access category, so λ4 finally discriminates.
  5. The session requester (TA-2) issues URGENT queries per session priority.
- **Verification.** G24 requires ≥1 query-triggered escalation in the preset's URGENT session. Unit tests cover the policy hook and the arbiter term.
- **Effort.** 1 day.

**NW-5: arbitration exactly as specified** (Network §5)
- **Spec.**
  - `U(r) = λ1·degradation_rate + λ2·(1/battery_remaining) + λ3·staleness + λ4·is_query_triggered`.
  - Arbitration runs at anchors.
  - Grant the argmax "among requests within current UWB range of this anchor".
  - "Denied requests remain queued, re-scored next cycle (aging ... prevents starvation)".
  - Grant/deny rides the ACK'd message.
- **Code.**
  - `ArbitrateRequest` (`ca-tbl-mac.cc:517-594`): `T_batt = reqBattery/255`, i.e. the fraction **spent** (`scratch:8586-8588`); T_query constant.
  - There is **no queue and no re-scoring**. A deny sends nothing, and there is no DENY frame. It is one slot per radio (`mac.h:648-656`).
  - Only ANCHOR-class MACs arbitrate. Finders (RESCUER class) carry UWB but never grant.
  - `pcsDegradation = 1 − exp(−max(0, −slope))` over the last 5 PCS values (`scratch:8564-8585`). That is a rate of change, which is to spec.
- **Steps.** Behind `--arbitrationV2` (preset on; requires `--uwbArbitration`, `--handshakeFixes`):
  1. The request carries **battery remaining**, `1 − used/cap`. The anchor computes `T_batt = min(1/max(rem, ε), 1/ε_cap)·ε_cap`, normalised to [0,1]. The normalisation constant is OURS and stated.
  2. `T_query` comes from the NW-4 bit.
  3. A request queue per arbiter: a denied request is queued with its terms. Each cycle (driven from the scenario loop like `GrantGossip::Cycle`) the queue is re-scored, with staleness aging by the time since the request. The slot goes to the argmax. The winner gets ESC_GRANT (granted), and the others get ESC_GRANT (denied, `queue_pos`).
  4. **Who arbitrates.** Gateways (ANCHOR) arbitrate their own UWB radio. **Finders also arbitrate their own UWB radio**: in this topology a finder is the mobile anchor that carries tier 3 (Architecture §3.1), and the spec's "anchors (fixed or vehicle-mounted)" covers it. This is our mapping, stated.
  5. **"Within current UWB range of this anchor" is enforced, not assumed (review finding).**
     - An ESC_REQUEST is sent on the *pre-escalation* tier (for a UWB escalation that is 2.4 GHz), so hearing it proves nothing about UWB reach.
     - The request carries its target tier (MC-1 step 5). An arbiter queues a UWB request only if it has **its own evidence** that the requester is in its UWB range: a successful UWB exchange with it within the last `--arbUwbEvidenceS` (default 10 s, OURS), or a 2.4 GHz range below the UWB r50 (29.5 m).
     - Otherwise it answers DENY with reason out-of-range.
  6. **NW-4 and NW-5 land in one commit**: the `isQueryTriggered` header bit together with the arbiter's T_query term. They are mutually dependent.
- **Verification.**
  - Unit tests: queue aging lifts a starved request, and the inverse-remaining battery term orders two requests correctly.
  - G24 requires grants, denials and at least one aged-queue grant among the 5 real subjects.
  - Negatives: a queue that never re-scores, and the constant T_query.
- **Effort.** 1.5 days.

**NW-6: anchors gossip their queue as well as their grants** (Network §6: "Anchors periodically gossip their current Q (request queue) and active grants")
- **Code.** GRANT_GOSSIP carries grants only: `CaTblGrantItem{targetId, startS, endS}`, size `1 + 6n` (`ca-tbl-header.h:332-344`, `ca-tbl-header.cc:300-305`, which explicitly says no queue). Gateways only, tier 1 only.
- **Steps.** Behind `--gossipQueue` (preset on; requires `--anchorGrantGossipS > 0` and NW-5):
  - Gossip participants are **every arbiter**: the gateways *and* the finders. After NW-0, finder↔gateway links exist, and finders are UWB arbiters under NW-5. `GrantGossip::AttachAnchor` is extended to finders' 868 MACs. Finders carry no energy account, which is stated.
  - Append queue items `{targetId, U-score byte, queuedSinceS}` (billed `+4k` bytes, OURS layout).
  - On receipt, the tie-break is unchanged (the lower id defers), extended to queued requests: a lower-id anchor holding a queued request that a higher-id anchor has queued or granted drops it.
  - Report double-queued counts.
- **Verification.** G16 is extended: queue items are gossiped, and a double-queued request is resolved. Negative: a queue-less gossip claimed as a queue gossip.
- **Effort.** 1 day.

**NW-7: a UWB escalation commits only on a confirmed grant** (MAC §8; Transport §4 "ON UWB grant confirmed: session.state = LOCATED_PRECISE")
- **Code.** The tier change commits inside `Step()` (`ca-tbl-policy.cc:1544-1553`), and **nothing at the tag consumes ESC_GRANT**. A grep for an ESC_GRANT handler in the scenario finds none. So "a node wrongly believes it has a UWB slot" is the current behaviour, exactly the failure the spec's ACK rule exists to prevent.
- **Steps.** Behind `--grantGatedUwb` (preset on; requires `--handshakeFixes`):
  1. When `Step()` decides to escalate **into UWB**, it records the tier change as *pending*. This reuses the existing deferral machinery (`AbortTierChange` / `deferEscalationOnDutyLimit`) rather than a parallel state.
  2. `d.escalated` is set with a `pending` qualifier, so the scenario's send path, keyed on `d.escalated`, still fires the ESC_REQUEST.
  3. **While pending, the tier is frozen** (review finding). De-escalation branches are suspended, except coverage-loss demotion, so a grant can never jump the node 868→UWB.
  4. On a delivered ESC_GRANT(granted) to that subject, via a new MAC grant callback, the scenario calls `policy.ConfirmUwbGrant()`, and the switch happens at the next cycle.
  5. A denial or a timeout (`ackTimeoutS` × retries) clears the pending state and counts `uwbGrantDenied` / `uwbGrantTimeout`.
  6. 2.4 GHz escalation is unchanged: the spec's ACK'd grant is for UWB specifically.
  7. The grant carries the anchor's id, which the subject publishes in its entry (`grantedBy`, NW-9) for TA-3.
  8. `static_uwb` is pinned to UWB, needs no grant, and is exempt.
- **Verification.**
  - Policy unit tests: no UWB without a grant; a grant switches the tier next cycle; no demotion while pending.
  - Its own gate fixture now. G24 later requires every hybrid-arm UWB cycle in the preset to be preceded by a delivered grant, recounted from `events_*` (digested under the preset) against the trace (rule 3).
- **Effort.** 1 day.

**NW-1: hop provenance reaches confidence and the RESULT**
- **Spec.**
  - Physical §2.5: "trace(Σ) — the aggregate uncertainty from the EKF state, already folding in RF noise, staleness, and hop-decay".
  - Final plan item 1: "Position confidence is modeled as a function of the path a fix traveled — hop count, per-hop RSSI/SNR variance, per-hop NLOS likelihood".
  - Transport §3: RESULT carries `hop_count_at_fix`.
- **Code.**
  - The measured tag's `policy.Step(dt, meas, 0, ...)` passes a literal 0 hop count (`:8018`). Relays do the same (`:6932`, `:3289`).
  - `res.hopCountAtFix = 0` (`:8176`). `CaTblRangeMeasurement` has no hop field, so depth is lost at `activeMeas.push_back` (`:7836-7844`).
  - The hop-decay in Σ does exist: the C5 admission inflates `σ_eff² = σ² + uᵀP_B u` (`:7802`, `ca-tbl-relay-admission.cc`).
  - Per-hop RSSI/SNR variance and per-hop NLOS likelihood never reach σ_eff (TECH_DEBT C.1: "per-hop RSSI/SNR variance still never reaches the score").
  - `hopWeight = 0` (`ca-tbl-policy.cc:101`), with no CLI flag. `--hopProvenanceForm=1` divides PCS by `(1 + hopPenalty·relayedVarFrac)` (`ca-tbl-policy.cc:899-915`).
  - `--maxRelayDepth` defaults to 1 (`:9412`).
- **Gap.** The count is never carried. The per-hop variance and NLOS terms of the spec are missing. Depth 2 is opt-in.
- **Steps.**
  1. Use the `hopDepth` field DC-11 already added to `CaTblRangeMeasurement` (rule 14; default 0, direct links stay 0). Set it at the C5 push from `rr.hopDepth`, which the chain overwrites via `ForTag` (`:3106`).
  2. Compute the fix set's `maxHop` next to `relayedVarThisCycle` (reset `:7461`, accumulate `:7847`). Pass it as Step's `hopCount` only when `meas == activeMeas` and the cycle fused (`!d.measurementRejected`, ≥3 measurements). Set `res.hopCountAtFix` from it. Do the same in the relays' Step calls using `depth[b]`.
  3. Per-hop variance and NLOS into σ_eff (spec), behind `--hopVarianceTerms`:
     - **The source does not exist yet (review finding).** `CaTblLinkVariability` exists only as `var868` over tag 0's anchor links (`scratch:4508`, `:4677`), and relay legs are never scored. So: add one `CaTblLinkVariability` keyed by (relay, anchor) and (subject, relay) pairs, fed on every relay-leg exchange, plus the tier's NLOS score on that leg.
     - σ_eff² += the per-hop RSSI-variance term, converted to range variance by the tier's inversion.
     - Per-hop NLOS: multiply the leg's variance by `nlosSigmaInflation` when its NLOS score ≥ threshold. This reuses the existing constant.
     - The conversion is OURS and says so.
  4. **`hopWeight` stays 0.** The spec's PCS formula (§2.5) has no hop term; hop enters through Σ. This matches RULES.md §2. Record in the register that the spec folds hop into Σ, which the code already does, plus steps 1–3.
  5. The preset sets `--relayedRanging --relayChain --maxRelayDepth=2` (or higher; the A2 budget governs) and `--hopVarianceTerms`.
- **Verification.**
  - Null control at defaults.
  - Gate **G23**, which reads **`relay_chain_*.csv`**, not a trace column. The trace header (`scratch:6029-6068`) has no hop column and gets none. In a relayed run, hop depth takes ≥2 distinct non-zero values on legs that reached Step. `RESULT.hopCountAtFix` in `session_*` equals the max depth of the fused set. σ_eff rows show the added terms, recounted.
  - Negatives: literal-0 hop, variance term disabled, depth capped at 1.
- **Effort.** 1–2 days.

**NW-2: confidence-weighted routing of every subject's position-fix traffic**
- **Spec.** Network §3:
  - `path_cost = Σ[α·NLOS_score + β·RSSI_variance + γ·relay_jitter]`, `argmin` among routes with `hop_count ≤ min + slack`;
  - it applies to position-fix traffic only, while gossip uses ETX;
  - slack starts at 1 and is "tune empirically".
- **Code.**
  - `FixRouter` (`:3361-3543`, cycle `:3749-3983`) routes relays only.
  - "index 0, the measured tag, never routes" (`:3507`), because a routed frame on a tier the tag did not transmit on would break §6.
  - α=β=γ=1, slack=1, all OURS (`ca-tbl-routing.h:97-105`). `SetWeights` is never called. ETX is the default selector; `--fixRoutingConfidence` selects by the cost.
- **Gap.** The measured subjects' fixes are never routed. The weights are not tunable or tuned.
- **Steps.**
  1. After MS-1, every measured subject is a router.
     - **What is routed under the edge estimator (review finding).** Under the default `--gatewayEdgeServer=true`, the fix is computed *at the gateway*, so a subject has no fix of its own to originate. What it routes is its position-fix **traffic**: its entry and range report.
     - That is ENTRY_PUSH, plus TRIGGERED_UPDATE and RESPONSE, which the header also marks position-fix traffic (`ca-tbl-header.cc:249-250`).
     - It is routed toward a gateway when it has no direct gateway link, and **only on its active tier this cycle**, so §6 holds.
     - `FixRouter` today builds tables only on `--relayTier` (`scratch:3780-3800`). Extend it to per-subject active-tier tables.
  2. Populate the routing table's `pcs` and `path_confidence_penalty` fields, which are left 0 today (`scratch:3399-3401`). Expose `--routeAlpha/--routeBeta/--routeGamma/--routeSlack` (defaults 1,1,1,1, labelled OURS) and call `SetWeights`.
  3. "Tune empirically": a small registered sweep, part of the spec campaign (§3 CAMP-SPEC). It reports how divergence and fix delivery depend on the weights. The weights stay OURS, chosen by stated criterion.
  4. The preset sets `--fixRouting --fixRoutingConfidence`.
- **Verification.** G18/G18b are extended to subjects ≥2 routing their own fixes. The §6 check runs per routed frame.
- **Effort.** 2–3 days after MS-1.

### 1.F Transport / application layer

What exists:
- `CaTblSessionManager` has `SearchAndLocalize`, `BatchSearch`, `OnResult`, `Tick` and `BatchPrecisionSpread` (`ca-tbl-session.h:218-261`).
- Its state machine matches the spec: SEARCHING → LOCATED_COARSE → LOCATED_PRECISE → TRACKING → STALE → CLOSED, with STALE recovering to TRACKING (`ca-tbl-session.cc:120-248`).
- The RESULT struct and the `CaTblRenderFor` rule (SUSPECT overrides tier/PCS) exist.
- All of it is behind `--sessionLayer` / `--batchSearch` (default false).

**TA-1: a real requester, and sessions fed by RESPONSE frames**
- **Spec.**
  - "A rescuer's handheld/base station issues a search".
  - "ON any RESPONSE received for this session: result = extract_position_and_provenance(RESPONSE)".
  - The Base Station / Mesh Sink is anchor hardware "plus uplink".
- **Code.**
  - The session is opened with `requesterId = 1u`, which is the measured tag itself (`scratch:5905-5928`), and it is fed **the tag's own EKF estimate every cycle** (`:8162-8182`, `targetManifestId = 1`, `x = policy.EstX()`).
  - No RESPONSE ever reaches a session. `m_requesterId`/`m_priority` are written but never read (`ca-tbl-session.cc:97-98`).
  - Only manifest 1 is fed, so `BatchPrecisionSpread` is always UNDEFINED.
- **Gap.** The session layer is a state machine driven by the subject's own filter, not a search.
- **Steps.** Behind `--sessionOverMesh` (preset on; requires `--queryV2 --reversePath --meshLinks=all`). **Both requesters** (decision 8):
  1. **TA-1a, the base station behind the gateway backhaul.** It runs the preset's **batch** session over manifests 1..5.
     - A RESPONSE delivered to *any* gateway is handed to the base station's session manager over the backhaul. The backhaul is unbilled and already modelled for the edge estimator.
     - This is the spec's Base Station/Mesh Sink ("same hardware as anchor, plus uplink").
     - It issues QUERYs through the per-tier gateway MACs (NW-3) on the session's period (`--sessionQueryPeriodS`).
  2. **TA-1b, a rescuer's handheld (finder).** Under `--finderSessions` (preset on), each finder runs its own `CaTblSessionManager`.
     - It opens a session for the manifest it is assigned (round-robin over the 5 subjects), or for all of them.
     - It issues QUERYs from its own MACs, and receives RESPONSEs over the mesh through the reverse path (NW-0, NW-8). Responses must reach a moving node: the reverse path is per query and time-windowed, and a miss falls back to a flood.
     - A finder's session escalation requests go to **its own arbiter** when it holds UWB (NW-5), else to the nearest gateway's via the mesh.
     - Output: `session_*` rows tagged with the requester id (0 = base station, 501+ = finder).
  3. `sessions.OnResult` is called only from the RESPONSE handler (NW-9), with the RESULT built from the delivered entry: position, Σ, tier_source, PCS, hop count, integrity flag and timestamp.
  4. Configurable `--sessionStalenessS` (default 120) and `--sessionTimeoutS` (default 600), because the setters exist but are never called.
  5. Per-session output `session_<arm>_seed<N>.csv`:
     - every state transition with time and the RESULT that caused it;
     - time-to-first-fix (SEARCHING → COARSE);
     - time-to-precision (→ PRECISE);
     - per-target spread for batch sessions.
     - Digested.
- **Verification.**
  - Session unit suite cases: a RESPONSE-driven transition, a stale→tracking recovery, and two requesters on one target.
  - Gate **G27**: in a preset run, both the base-station and at least one finder session reach LOCATED_COARSE from a delivered RESPONSE (not the policy), and time-to-first-fix > 0 and finite. Every RESULT in `session_*` matches a delivered RESPONSE in `events_*`, recounted.
  - Negative: the legacy policy feed offered as over-mesh.
- **Effort.** 2.5 days (after NW-0/8/9). TA-1b adds a day for per-finder managers and moving-requester routing.

**TA-2: SEARCH_AND_LOCALIZE: the requester drives escalation**
- **Spec.** Transport §4: on a RESULT with `PCS < threshold` and integrity CLEAN, the requester submits `EscalationRequest{target, degradation_rate, battery, staleness}` to the nearest anchor. With priority URGENT and a non-UWB tier, the request is marked query-triggered. The anchor scores it against all pending requests.
- **Code.** There is no requester-side escalation. The only escalation is the tag's own PCS decision.
- **Steps.** Under `--sessionOverMesh`:
  - **Where the request goes (review finding).** Under topology 0 a gateway offers UWB only to something standing beside it. Sending the base station's request to "the nearest gateway" would grant slots nobody can serve.
    - Requests are routed to the arbiter that has UWB-range evidence for the target (NW-5 step 5). In practice that is a finder near the target.
    - The base station learns this from the target's RESPONSE entries: the finder ids in its fresh neighbour set, which are carried.
    - The request reaches that finder over the mesh.
    - With no arbiter in reach, the request is recorded as unroutable, not sent.
  - Terms come from the RESPONSE entry (battery remaining, degradation; NW-9).
  - The winning grant is sent by that arbiter to the target as ESC_GRANT, and the target's NW-7 path commits the escalation.
  - The tag's own PCS-driven request (Physical §2.8) keeps working in parallel. Both routes land in the same arbiter queue, exactly as the spec's "invisibly to this single session's logic" says.
- **Verification.** G27 extension: at least one grant originates from a requester-submitted request, with `isQueryTriggered` on the URGENT session.
- **Effort.** 1 day.

**TA-3: LOCATED_PRECISE on a confirmed UWB grant**
- **No oracle (review finding).** The session learns of a grant only from what reaches it: the target's next RESPONSE entry carries `uwbActive` / `grantedBy` (NW-9).
  - "ON UWB grant confirmed: LOCATED_PRECISE" is then evaluated at the requester, one delivery later, which is realistic.
  - UWB RESULTs keep the session in TRACKING.
- **Explicit cancel → CLOSED** (spec Transport §2): a finder whose closest approach to its assigned target is < 2 m (OURS) cancels its session, as "target physically reached".
- **Verification.** Unit test plus G27.
- **Effort.** Half a day.

**TA-4: BATCH_SEARCH over the five measured subjects**
- **Spec.** One session, one QUERY with the manifest list, one reverse-path cache, and all escalation requests landing in the same arbiter queue. The report's headline is the difference in time-to-precision between the highest- and lowest-priority targets.
- **Code.** `BatchSearch` exists, but only manifest 1 is ever fed. Peers are not subjects.
- **Steps.** After MS-1 and TA-1, the preset opens one `BatchSearch` over manifests 1..5. `BatchPrecisionSpread` is reported in `session_*` and stdout.
- **Verification.** G27 requires spread to be defined (≥2 targets reached PRECISE) in a longer preset cell, or records why not.
- **Effort.** 2 hours after its dependencies.

**TA-5: RESULT provenance is real.** `hopCountAtFix` comes from NW-1, not a literal 0 (`scratch:8176`). The integrity flag comes from the entry (NW-9). The render rule then acts on delivered results. This is already covered by NW-1, NW-9 and TA-1.

### 1.G System, energy and deployment items

**SY-1: gateway edge server bills the delivery of tier-2/3 ranges** (L.2.3)
- **Spec.** Gateways "do the EKF fusion and NLOS/hop computation, not the battery-constrained field node".
- **Code.** The downlink is billed (`AccountEdgeServerDownlink`, `ca-tbl-energy-model.cc:345-363`, call `:8095-8098`). Getting a tier-2/3 range from the node to the gateway is not billed (TECH_DEBT L.2.3).
- **Steps.** Behind `--edgeUplinkRanges` (default false; preset true):
  1. In `rangeTier()` after `AccountRangingExchange` (`:7414-7417`), when `gatewayEdgeServer && tier != 868 && !out.empty()`, bill one uplink frame on that tier with `AccountFrameTx(tier, t, airtime)`. Airtime comes from `LoRaTimeOnAirS` (2.4 GHz) or `UwbFrameTimeS` (UWB), with a payload of one range record per answered anchor (bytes OURS, from the table-entry wire format).
  2. The tier is already transmitting this cycle, so §6 holds.
  3. On UWB too. The spec's UWB TWR is Poucet's START/ACK/DATA_REPLY, which leaves the range at the **initiator**, the tag (docx Physical §1.3). So the tag must send it up; this is billed like 2.4 GHz. The review corrected an earlier draft that exempted UWB.
  4. Rough scale: 2.4 GHz ≈ 0.51 mJ TX per 12-byte frame, UWB ≈ 0.049 mJ. These are arithmetic estimates, to be measured.
- **Verification.** An energy-only gate in the pattern of G21 (`scripts/edge_default_gate.py`): only energy moves, only 2.4 GHz/UWB `tx_` rise, only on rows with tier-2/3 exchanges, and 868 is unchanged.
- **Effort.** Half a day.

**SY-2: radio-part basis LR1121** (PROV-03)
- **Spec.** Tiers 1 and 2 use the LR1121 dual-band die; tier 3 uses a DW3000.
- **Code.** SX1262 currents (`ca-tbl-energy-model.cc:32-48`), SX1280 (`:50-66`), DW3000 (`:68-87`). `SetProfile(tier, profile)` exists (`ca-tbl-energy-model.h:148`) and is never called. The repo does not mention LR1121 anywhere.
- **Steps.** Add `--radioBasis=sx|lr1121` (default sx; preset lr1121).
  1. Populate an LR1121 profile per band (TX at the scenario's powers, RX, idle/standby, sleep, CAD) **from the Semtech LR1121 datasheet**. The document number, revision and table are cited in the source; the PDF is read locally once poppler-utils is available.
  2. Apply it via `SetProfile` for 868 and 2.4 GHz on every subject's model, and on the gossip accounts.
  3. Any figure the datasheet does not give (for example CAD current at 2.4 GHz) is marked OURS with its derivation.
- **Verification.** An energy-only gate **at defaults**: the same engine evaluations, and only `j_/tx_/rx_/idle_/sleep_/cad_` and `totalJ`/`accuracyPerJoule` move. Under the preset, energy feeds the ESC_REQUEST battery term and so arbitration, so "energy-only" does not hold there. That is stated.
- **Effort.** Half a day, plus the datasheet read.

**SY-3: tier-1 SF/BW configurable**
- **Spec.** Physical §1.1: "configurable spreading factor (SF7–SF12) and bandwidth (125/250/500 kHz)".
- **Code.** Hard-coded as `phy[868] = {7, 125e3, 14.0}` (`:11207-11210`), with no flag.
- **Steps.** Add `--sf868`, `--bw868` and the matching tier-2 pair.
  - Defaults are the current values.
  - They are validated against the sensitivity tables' SF range: G14 already checks the SF index.
  - `phy[]` flows into the energy model, the MACs, the relay-slot airtime check and FixRouter, so no other site changes.
  - The preset keeps SF7/125 kHz. The spec makes it configurable, not a particular value.
- **Effort.** Hours.

**SY-4: GPS-PPS anchor sync.** Conformant under TDoA; nothing to build.
- The final audit says GPS lock at the anchors resolves both survey and sync with "no holdover-drift uncertainty".
- `--tier1Tdoa` draws a PPS residual per stamp (`--tdoaPpsSigmaNs`, 20 ns, u-blox M8T) against surveyed positions.
- Record this as conformant in the register. The earlier audit's "partial" rating was too strict.

**SY-5: environmental fault classes for the evaluation**
- **Spec.** Execution plan Phase 5: "switch-decision accuracy under induced degradation (deliberately covering an antenna, cold-soaking a battery...)". The novelty text lists cold-sagged battery RSSI skew, antenna detuning from ice, and canopy multipath.
- **Code.**
  - `--injectRangeBias` / `--injectFaultAnchors` / `--injectBiasFraction` give common-mode or single-anchor range bias on tier 2.
  - `--injectSilentId` silences a node; `--batteryDepleteJ` models a depleted battery.
  - There is no transmit-power or RSSI-skew fault and no antenna-gain fault.
- **Steps.** Add `--injectTxPowerDropDb` and `--injectAntennaLossDb` (a tag-side link-budget offset over the same seeded window), applied in `rangeTier` to the node's own TX power or received power. They are labelled as fault models, OURS in magnitude.
  - This gives the integrity check the "statistically clean, physically wrong" faults the docx names.
  - They consume no RNG draws.
- **Effort.** Half a day.

### 1.H Defects and stale records found while planning (recorded in P1, fixed where it costs nothing)

These were found by the three code explorations of 2026-09-30. None is in the register yet.

**Code defects** (each fixed in its own null-controlled commit, or behind the flag of the item that touches it):
1. `CaTblPolicy::RevertToPrior` is a no-op on the suspect path: it runs before any update, so a rejected cycle is a plain coast. The header (`ca-tbl-policy.h:193-197`) claims P is frozen. **Fix:** the comment (DC-7).
2. `m_consecutiveRejections`, `m_reinitFallbackCount` and `m_recoveryFirings` are never initialised in the constructor (`ca-tbl-policy.cc:559-569`). **Fix:** initialise them. This is a no-behaviour change today, because the vector value-initialises them.
3. `ApplyProvenance` divides a **negative** v2 PCS by a factor ≥ 1, raising confidence (`ca-tbl-policy.cc:852`, `:899-915`). **Fix:** make it form-aware behind `--hopProvenanceForm=1`, which is the only place it acts. Record the old behaviour.
4. An ACK clears the oldest pending frame, not the matching one (`ca-tbl-mac.cc:1433-1436`). An ESC_REQUEST is ACKed before arbitration (`:1162-1168`). A pre-emption ESC_REQUEST carries 0/0 terms (`scratch:7503-7507`). Terms are one cycle stale (`scratch:8560-8588`). **Fix:** MC-1.
5. `reversePathWindowS` is never read, and reverse-path caching caches the originator, which makes it a no-op. **Fix:** NW-8.
6. A matched target or proxy still rebroadcasts the QUERY, and a RESPONSE is empty. **Fix:** NW-9.
7. `--uwbArbitration` is inert without `--sendHandshakeFrames` and nothing refuses it. `--imuFusion`'s stated requirements (`--shsSnapPosition`, `--shsTrustedFixK`) are not checked. **Fix:** a FATAL for the first, and a WARNING for the second.
8. Dead configuration: `imuConsistencyK`, `imuSigmaFloorM`, `instantEscalate` and `m_prevEstX/Y` are never read by the policy. **Record only.**
9. With `--sparseCaseA --relayedRanging`, Case A fuses a position from two ranges (`scratch:7663`), and then `Step()` updates on the same ranges plus relays. The measurement is double-counted. **Fix:** DC-11.
10. `rangeTier` looks up `anchorPosById[anchorId]` with `operator[]` (`scratch:7175`). An id absent from the map silently becomes an anchor at (0,0,0). It is latent today, because only anchors enter poll sets, and it becomes live the moment tag↔tag links exist. **Fix:** a `find()` guard (NW-0).
11. **A spec divergence to record:** PCS term 1 is the bounded transform `traceRef/(traceRef + trace)` (`ca-tbl-policy.cc:857`), not the docx's `1/trace(Σ)`. It is a monotone equivalent chosen so term 1 cannot dominate. It is kept and recorded, and CAL-02's calibrated weights act on it.

**Help-text and comment errors.** All are fixed with the item that touches them:
- `--pcsForm` lists only legacy|v2.
- `--pcsHysteresisBand` claims a 0.20 gap on every tier.
- `--imuCrossCheck` calls itself a displacement check.
- `--nTags` and the C2-peer comment (`scratch:5569-5571`, `:10772-10779`) say peers have no policy, which is false under `--relayedRanging`.
- `ca-tbl-policy.cc:961-964` says Cases B/C "are not implemented".
- The `scratch:8361-8362` comment says the receding branch never runs.
- `--sparseCaseAMargin` is in Mahalanobis units, not metres.

**Stale documents.** Corrected in P1, except `RULES.md`, which is the owner's:
- TECH_DEBT 3.3 still says the C2-full merge decision is open. It was merged at `abbe3de`.
- TECH_DEBT 3.2 has stale line references and says "A2 caps depth at 1", but A2 is now adaptive.
- TECH_DEBT L.2.4 cites `:8009`; it is now `:8018`.
- CAL01 results cites `:7874`.
- K.9 cites the step-length submodule at `:101`; it is now `:113-116`.
- `docs/DESIGN_DECISIONS.md` §5 and line 138 say the brief's deployment is "not built" and "untested". It is built (`089a9f1`), and K1c, K10 and L.2 ran it.
- `MASTER_PLAN.md` §2.1's transport/session row reads ABSENT, but `--sessionLayer` and a 29-check suite exist.
- `RULES.md` §2 cites `ca-tbl-policy.cc:83` for `hopWeight`; it is at `:101`. This goes with M.11, for the owner.

**Unrecorded spec divergences from the 2026-09-30 audit.** Each goes into `MASTER_PLAN.md` §2.1 with its reason, before any code:
- Case D is absent.
- The query-triggered bypass is absent.
- A SUSPECT fix is rejected rather than downweighted.
- The entry velocity is 0.
- The arbitration battery term is the fraction spent, and T_query is constant.
- Gossip carries no queue.
- Sessions are fed the node's own filter.
- The escalation commits without a grant.
- There are no mesh links beyond tag↔anchor.

### 1.I Coverage map: every lacking audit row → the item that closes it

| audit row (docx §) | plan item |
|---|---|
| tier-1 SF/BW configurable (Phys 1.1) | SY-3 |
| tier-1 TDoA (Phys 1.1) | PH-1 |
| UWB CIR NLOS score (Phys 1.3) | PH-2 |
| LR1121 radio basis (hardware) | SY-2 |
| entry velocity / E fields (Phys 2.1) | DC-10, NW-9 |
| EKF IMU control input (Phys 2.2) | DC-1 |
| sparse cases on the propagated prior (Phys 2.4) | DC-11 |
| Case B by IMU heading (Phys 2.4) | DC-2 |
| Case C relative shape (Phys 2.4) | DC-3 |
| Case D (Phys 2.4D) | DC-4 |
| PCS term 3 = tier-specific score (Phys 2.5) | PH-2 + preset `--pcsNlosClassifier` |
| PCS weights by LOOCV (Phys 2.5, audit Gap B) | DC-5, DC-6, CAL-02 |
| SUSPECT downweights PCS (Phys 2.6) | DC-7 |
| integrity eligibility "NLOS_score LOW" (Phys 2.6) | DC-8 |
| witness cross-check (novelty item 2) | DC-9 |
| supersession + Covariance Intersection (Phys 2.7) | MC-3 |
| ESC handshake on air + ACK (MAC 8) | MC-1, NW-7 |
| triggered update / last gasp (MAC 3, 8) | MC-2 |
| mobility-state handshake (MAC 9) | MC-4 |
| mesh links (network, all) | NW-0 |
| hop provenance into Σ, RESULT (Phys 2.5, Net, Transp 3) | NW-1 |
| confidence routing of subjects' fixes (Net 3) | NW-2 |
| per-tier query binding (Net 4) | NW-3 |
| query-triggered escalation (Net 4) | NW-4 |
| arbitration terms, queue, deny (Net 5) | NW-5 |
| gossip queue + grants (Net 6) | NW-6 |
| grant-gated UWB (MAC 8, Transp 4) | NW-7 |
| reverse path (Transp 1) | NW-8 |
| QUERY/RESPONSE contents (Net 4) | NW-9 |
| sessions from RESPONSE, requester (Transp 2, 4) | TA-1 |
| requester-driven escalation (Transp 4) | TA-2 |
| PRECISE on grant (Transp 2) | TA-3 |
| batch search over 5 targets (Transp 5) | TA-4 |
| RESULT provenance (Transp 3) | TA-5 |
| edge server bills range delivery (final plan) | SY-1 |
| GPS-PPS (audit Q1) | SY-4 (conformant under TDoA) |
| 5 measured field nodes (audit Q2) | MS-1 |
| 3 gateways / 5 km (audit Q2) | MS-2 |
| rescuers carry UWB (identity class RESCUER) | MS-3 |
| fault classes for evaluation (exec Phase 5) | SY-5 |
| per-entry prediction with source-type Q (Phys 2.2) | DC-13 |
| rescuers search for specific people (identity, Net 1) | MS-1 finder tasking |
| explicit cancel → CLOSED (Transp 2) | TA-3 |
| QUERY ttl field (Net 4) | NW-9 |
| routing covers all position-fix types (Net 3) | NW-2 |
| evaluation comparators: fixed threshold, single-hop confidence (exec Phase 5) | §3.5 CAMP-SPEC |
| PCS term-1 form (Phys 2.5) | recorded divergence, §1.H #11 |
| all opt-in mechanisms on together | §3.1 `--specProfile` |

---

## 2. Build order

The order is load-bearing: later phases read what earlier phases produce. Every step is its own commit (or commits) with a null control.

| phase | what | why here | est. |
|---|---|---|---|
| **P0** | Finish TECH_DEBT M.2: implement `--uwbNlosModel` exactly per `report/PREREG_UWB_NLOS.md` §2, run its 40 registered cells, write `report/UWB_NLOS_RESULTS.md`. Then freeze `~/catbl_ref_spec0` with NC1–NC8 (rule 11), and add the `SpecOptions` struct (no behaviour, null-controlled). | It is already registered at `166d431`, and the registration binds the next commit to it. Every later null control needs the frozen reference and its opt-in-path configs. | 2 d |
| **P1** | Documentation first: `MASTER_PLAN.md` §2.1 divergence rows, TECH_DEBT stale lines, and `docs/SPEC_CONFORMANCE.md` started with all 52 rows. Then the no-behaviour fixes (§1.H items 1, 2, 7, help texts). Then each flag-gated item **with its own gate and fixture** (rule 10): DC-6, DC-7, DC-8, DC-10, NW-3, MC-1 (incl. UWB-only requests), MC-2, SY-1, SY-3, SY-5. Then the MS-2 `--gwLayout` shape code and **GEO-01 stage 1** (geometric screening, `geo_screen.py`/`geo_optimise.py`). | Small and independent. They close Bar-2 gaps without waiting on the big refactors. | 8–10 d |
| **P2** | DC-11 sparse solving inside `Step()` (with the one-time measurement-struct extension, rule 14, and the double-count fix), then DC-1 synthetic IMU, then DC-4 Case D, DC-2 Case B heading, DC-9 witness on the IMU vs the raw fix, DC-13 entry prediction. | Three spec items depend on an IMU, and so does the integrity check's independent prior (MASTER_PLAN C3). | 7–9 d |
| **P3** | MS-1 multi-subject: the no-behaviour move, then `--measuredSubjects`, relay/subject dedup, per-subject components, finder tasking, `subject_summary`/`subjtrace_*`, and G22. In parallel: NW-0 mesh links, with G26 one-hop. | Every multi-target mechanism after this reads real subjects. The network/transport layer needs mesh links. | 9–12 d |
| **P4** | Network and application on real subjects and the mesh, in data-flow order: NW-8 previous hop and reverse path (with `forceFlood`), NW-9 QUERY/RESPONSE contents (batch-safe rebroadcast), NW-7 grant-gated UWB, **NW-4 + NW-5 in one commit** (query bit plus arbitration queue, terms and UWB-range eligibility), NW-6 gossip incl. finders; then NW-1 hop provenance (per-leg variability) and NW-2 routing of subjects' position-fix traffic; then TA-1a/TA-1b sessions, TA-2 requests to UWB-capable arbiters, TA-3 grant via RESPONSE plus cancel, TA-4, TA-5; finally DC-3 Case C (shared peer ranges, base-station MDS). | Needs P2 (IMU), P3 (subjects) and NW-0. The order follows the data: frames carry content, then routing, then sessions read it. | 14–17 d |
| **P5** | SY-2 LR1121 basis. The UWB-reach checkpoint (MS-3): a preset-minus-geometry probe must show `cycles_uwb > 0` and UWB-delivered fixes with the evidence gate on, or the owner is consulted. **GEO-01 stage 2** (registered) on the P4 build, which fixes the preset geometry. MS-3 rescuer settings. | Energy basis and geometry are preset settings, and GEO-01 needs the P4 mechanisms that carry load at sparse geometry. | 3–4 d + compute |
| **P6** | `--specProfile` preset (incl. the §3.1 additions), gate G24 (aggregating, with the last-gasp run), CAL-02 registered and run (legacy-PCS data generation, held-out evaluation), then CAMP-SPEC registered and run (incl. GEO-01 stage 3 and the comparator arms). | The preset composes everything above; the campaigns measure it. | 5–6 d + compute |
| **P7** | Documentation sweep: SPEC_CONFORMANCE register, MASTER_PLAN §2.1 recount (parsed, not hand-counted), TECH_DEBT new section N, `docs/SCENARIO_FLAGS.md` regenerated from `--PrintHelp`, handoff. `RULES.md` numbers (M.11) only with the owner's say-so. | Nothing is done until the record says so. | 1–2 d |

Total: roughly **50–62 working days plus compute** (revised up from 28–38 after the design review: P4 alone sums to 14–17 d, MS-1 to 7–10 d, and NW-0 to 3–4 d). P1 and P2 can run in parallel after P0. P3 → P4 → P5 is the critical path.

**New gates.** Each has a negative-fixture self-test script.

| gate | checks | item |
|---|---|---|
| G22 | subjects | MS-1 |
| G23 | hop provenance | NW-1 |
| G24 | the preset is conformant | §3.2 |
| G25 | synthetic IMU | DC-1 |
| G26 | mesh links and reverse path | NW-0, NW-8 |
| G27 | sessions over the mesh | TA-* |

Existing gates G16, G18/G18b and G20 are extended where noted.

---

## 3. The preset and the campaigns

### 3.1 `--specProfile=true`
- It is resolved right after `cmd.Parse` (`:10863`), in the pattern of `--relaySlots=auto` (`:10930-10998`).
- For each spec flag the user did **not** set explicitly, it assigns the spec value. An explicit flag always wins.
  - Explicitness is detected by registering those flags with sentinel defaults, or through ns-3 `CommandLine` callbacks, as `relaySlots` does (`:10626`).
- It writes `spec_profile.txt` (resolved name=value for every flag the preset touched, plus the docx section each serves). The file is digested and exists only under the preset. It also prints a NOTE on stderr.
- **Refusal:** the preset is FATAL with `--topology=1`, which is the legacy model and never the spec.
- Default off, so every existing run is byte-identical. That is checked by the NC and by G2/G6.
- The flag's help text lists every setting and its reason.

**What the preset sets:**

| group | settings |
|---|---|
| Physical | `--tier1Tdoa`, `--nlosClassifier`, `--pcsNlosClassifier`, `--nlosSource=propagation` (the legacy tag keys on a Markov state that "attenuates nothing", `ca-tbl-policy.h:65-75`), `--radioBasis=lr1121`, `--terrainPerTierFreq=true` (per-tier terrain wavelength, D-4; a physics-correctness fix, not a docx item), UWB channel model per §5 |
| Decision core | `--imuPredict`, `--sparseInStep`, `--sparseCaseA`, `--caseD`, `--caseCRelative`, `--pcsForm=calibrated` + CAL-02 weights/thresholds, `--suspectRecovery=downweight --suspectForcesEscalation=false`, `--integrityNlosFromScore`, `--imuCrossCheck --corroborationCheck --imuWitnessSource=model`, `--entryVelocity --entryPrediction`, `--escalationRequiresEvidence=true` (decision 7, unchanged) with `--uwbEvidenceFromNeighbours=true`, `--uwbCoverageIncludesFinders=true` |
| MAC | `--sendHandshakeFrames`, `--handshakeFixes`, `--triggeredUpdates`, `--mobilityFromRange`, `--bidirectionalDigest --digestSummary --tableFuseCI` |
| Network | `--meshLinks=all`, `--relayedRanging --relayChain --maxRelayDepth=2 --hopVarianceTerms`, `--fixRouting --fixRoutingConfidence`, `--queryV2 --queryTierBinding=per-tier --queryProxy --reversePath`, `--queryTriggeredEscalation`, `--uwbArbitration --arbitrationV2`, `--anchorGrantGossipS=5 --gossipQueue`, `--grantGatedUwb` |
| Transport / application | `--sessionLayer --sessionOverMesh --batchSearch --finderSessions`, URGENT priority on the batch session, `--vizEvents=true` with `events_*` digested (rule 13) |
| System | `--gatewayEdgeServer=true` (already default), `--edgeUplinkRanges`, SY-5 faults **off** (they are for the evaluation matrix, not the preset) |
| Deployment | `--measuredSubjects=5 --nTags=5`, GEO-01's winning `--gwLayout`/N/scale, `--boxHalf=5000`, `--nFinders=4 --finderMode=2 --finderDispersal=1.0 --finderPseudoAnchor=true --finderDigestPeriodS=20 --finderTasking=roundrobin` |

G24 also runs a second short preset run with `--batteryDepleteJ` set low. That is the only way the last-gasp population exists, and the preset itself does not deplete batteries.

### 3.2 Gate G24: the preset is conformant
- A short preset run: 5 subjects, 900 s, all arms needed.
- Asserts that each spec mechanism has a **non-zero population**, using the repo's "firing plus input" rule (TECH_DEBT H.3/I.5): Case A/B/C/D counts, suspects under score-eligibility, CI merges, query-triggered escalations, routed subject fixes, hop depth ≥2, session transitions reaching LOCATED_PRECISE, arbitration deny/pre-empt among real subjects, gossip verdicts, IMU predict steps.
- It lists any mechanism with a zero population as **FAIL**, never as a pass-by-absence.
- Negative fixtures: the preset with each mechanism's flag forced off, one at a time (the flag-efficacy idea of `scripts/flag_efficacy.sh`).

### 3.3 CAL-02: PCS weights and thresholds by LOOCV (decision 3)
- **Registered first** (`report/PREREG_CAL02.md`).
- **Avoid the circularity (review finding).** The terms' distributions depend on the policy that generated them. So the registration fixes the **data-generating configuration**: the full preset with PCS held at `legacy`.
  - Calibration is fitted on seeds 1–20 of that configuration.
  - The calibrated preset is then evaluated on **held-out** seeds 21–40.
  - A shift between the two is reported.
- The weights come from CAL-01's procedure (`scripts/cal01_loocv.py`, already (3, 0, 0) on other regimes), re-run on that data.
- Per-tier thresholds need per-tier labels. The UWB threshold is fitted against `shouldUwb`. The 2.4 GHz and 868 thresholds are fitted against a per-tier "would the next tier have solved" label from the trace's `vis2p4`/`visUwb`. The registration states this.
- Thresholds are LOOCV-fitted per tier against a registered objective. Candidate: trigger F1 against the ground-truth `shouldUwb` label, or the A ≥ 0.95 at R ≤ 25 m requirement. The objective is picked in the registration. CAL-01 §2 shows `3·t1` never reaches 1.55, so the old thresholds cannot be reused.
- Fix M.7 in the C++ machinery first: fit the control with an intercept, as `CaTblCalibrateLoocv` currently does not (`ca-tbl-loocv.cc:148-162`). Otherwise the OK verdict it prints is an artefact.
- Implementation: `--pcsForm=calibrated`, reading weights from `--pcsW1..3` (DC-6 makes 0 expressible) and thresholds from `--pcsThrUp<tier>`/`--pcsThrDown<tier>`.
- The report states the expected and likely outcome: under the calibrated form `ca_tbl_full` behaves like `fixed_threshold`. That is the spec followed faithfully, and it is what the evidence supports.

### 3.4 GEO-01: the least-infrastructure gateway geometry for SAR (decision 5)

**The question.** Over the spec's search area, which gateway shape and count gives SAR-adequate localisation with the fewest gateways, using only positions a vehicle can reach?
- The search area is `boxHalf=5000`, a 5 km radius taken as the half-width of the square.
- 4 finders are present (decision 6).

**Candidates.**

| shape | how it is built | N |
|---|---|---|
| ring/circle | `--nGateways --gwRingFrac` | 3, 4, 5, 6, 8 |
| crescent/arc | `--gwArcDeg` ∈ {90, 140, 180, 240}, centred at the trailhead (west) | same N |
| box perimeter | `--gwLayout=perimeter` | same N |
| cluster at base camp | `--gwLayout=cluster` | same N |
| star | `--gwLayout=star` | same N |
| L-shaped roads | `--gwLayout=lshape` | same N |
| line/ridge | `--gwRidge` | same N |
| optimiser placement, realistic | `--gwLayout=file`, perimeter/access band only | same N |
| optimiser placement, unrestricted | `--gwLayout=file`, no restriction — upper bound, flagged unrealistic | same N |
| interior grid | `--gwGrid` 2–3 | reference only; rejected on realism (HANDOFF_2026_09_09 §4) |

**Stage 1: geometric screening.** No simulation; minutes; `scripts/geo_screen.py`.
1. Probe the current build at the spec environment (`--probe=true`) to get tier-1 PDR against distance.
2. Over a 50 m lattice of the area, compute for each candidate:
   - P(≥3 gateways decode), from the probe PDR with independent links (stated);
   - the RSSI-inversion σ, projected through GDOP, giving a predicted CEP;
   - P(≥2) and P(≥1), which are the Case A/B/D coverage.
3. Report coverage fractions and the predicted CEP50/95 per candidate.
4. Prune to the best 2 shapes per N. The pruning rule is registered in advance.

**Stage 2: registered simulation.** `report/PREREG_GEO01.md` is committed before any cell.
- Pruned candidates × 10 seeds × 1800 s.
- Arms `static_868` and `ca_tbl_full`, the 4-finder regime, on the P4 build with the preset minus geometry.
- Metrics:
  - availability, `fixCycles/steps` of any tier;
  - CEP50/CEP95;
  - `tFirstFixS` and `tFirst100mS` (both already columns);
  - the finders' time to first UWB contact;
  - `totalJ`.
- **Selection rule** (registered; the exact thresholds are fixed in the prereg):
  - the **smallest N** at which a realistic shape meets the operational criterion;
  - the proposed criterion is availability ≥ 0.95 (the spec's A, Architecture §1.3) and median `tFirst100mS` within a bound;
  - among those, the best CEP95;
  - ties go to fewer access points needed (cluster/arc before ring/perimeter).
- The spec's R ≤ 25 m is reported against each candidate, not required. Tier-1 RSSI cannot meet it at this scale, and only UWB via finders can.

**Stage 3: confirmation.** The winner and the runner-up are re-run under the full `--specProfile` inside CAMP-SPEC. If the order flips, that is reported, not re-tuned.

**Outputs.** `results/geo01_<date>/`, `report/GEO01_RESULTS.md`, and the `docs/DESIGN_DECISIONS.md` §5 rewrite.

**Timing (review finding).** Stage 1 runs early, in P1, because it only needs the shape flags and a probe.
- **Stage 2 runs after P4**, in P5. At 3–8 gateways over 5 km, Case D, sparse solving in `Step()`, relays, the mesh and 5 subjects are exactly what carry the load. A stage 2 run before they exist would pick a winner for a system that no longer exists.
- Stage 2 therefore runs on the P4 build with the preset minus its geometry.
- The preset's geometry is fixed from stage 2. Stage 3 confirms it inside CAMP-SPEC.

### 3.5 CAMP-SPEC: the spec configuration measured
- Registered before any cell.
- Preset × the six arms × 20 seeds.
- Plus:
  - the NW-2 routing-weight sweep;
  - the SY-5 fault matrix, for integrity precision/recall, with DC-7's inflated-R recall consequence pre-registered;
  - a batch-search cell, for arbitration fairness as time-to-precision spread;
  - the **spec's Phase-5 comparator arms**: fixed-threshold switching (`fixed_threshold`, exists), and a **single-hop confidence baseline, "LoRaWAPS-style"**, i.e. the preset with relayed measurements excluded from every fix set and `--hopVarianceTerms` off, evaluated on error against hop count and confidence calibration across hop counts.
- Outputs go to `results/spec_<date>/`, never over an existing directory.
- The metrics are the docx's own: CEP50/95, PCS–error correlation against the trace control, integrity-check precision/recall under injected faults, time-to-first-fix, time-to-precision, precision spread across batch targets.

---

## 4. Verification, end to end

For every commit:
1. `CC=gcc-11 CXX=g++-11 ./ns3 build` (optimized).
2. The ten unit suites (235 checks today), plus any new suite (IMU model, Case D, calibrated PCS, subjects).
3. `bash ~/catbl_ref_spec0/nc.sh '<build>' <tag>` gives NULL CONTROL: PASS. That is NC1–NC2 at defaults, plus whichever of NC3–NC8 cover the opt-in paths the commit touches (rule 11), all with the new flag off.
4. For a replacement flag: old value = previous commit, byte-for-byte, in at least 5 configurations.
5. The mechanism's gate plus its negative-fixture self-test.
6. The full gate suite (`bash scripts/catbl_gates.sh`, unpiped, TMPDIR outside /tmp). The expectation is 19 PASS with the documented 4 red (G4, G5, G6, G11), plus the new gates green. `git status` is unchanged before and after.
7. The G12 attribution check passes (no trailer).

At the end of the programme:
- A `--specProfile` 900 s run passes G24.
- The CAMP-SPEC read-out is written.
- `docs/SPEC_CONFORMANCE.md` shows all 52 rows as either CONFORMANT (with a population), or DIVERGENT with a recorded reason and owner sign-off.

---

## 5. Items that need the owner during execution, not now

1. **The UWB channel model in the preset.** `legacy` or `ieee802154a`, decided after P0's `report/UWB_NLOS_RESULTS.md`. The registration fixes the *default* as legacy regardless, because 802.15.4a has no disaster-area data.
2. **Tools for datasheets.** `sudo apt install poppler-utils`, to read the Bosch BMI270 and Semtech LR1121 PDFs locally so their numbers can be cited. The alternative is that the owner pastes the tables. Without either, DC-1 falls back to the MPU-6050 figures already cited in the repo, and SY-2 waits.
3. **Registration thresholds** chosen when each prereg is written:
   - GEO-01's selection criterion: the availability and `tFirst100mS` bounds;
   - CAL-02's threshold objective;
   - the NW-2 routing-weight sweep criterion.
4. **Push.** The branch is local-only. Pushing goes via the Windows clone (`SETUP_NEW_MACHINE.md` §6), when the owner chooses.
5. **`RULES.md` edits** (M.11 numbers, the `hopWeight` line reference). These are the owner's.

## 6. Risks, stated

- **Tier 3 may be unreachable under the preset even with finders.** The NC2 reference shows 0 UWB cycles, and the preset keeps the evidence gate on. The P5 checkpoint (MS-3) exists for this. If the checkpoint fails, the owner revisits decision 7 before P6 is built on it.
- **This plan was reviewed** by an independent design pass (2026-09-30). The four blocking findings are folded into the items above:
  - the NW-0 phantom anchor;
  - two §6 breaches (sleeping radios relaying, and relay-subjects charged twice);
  - the missing UWB-reach flags;
  - the under-costed P4.

  So were about 30 medium findings: DC-1 correlated errors, DC-4 reachability, NW-5 eligibility, the batch-safe rebroadcast, CAL-02 circularity, GEO-01 timing, and gates landing with their items.

- **The MS-1 refactor can move traces by an ulp** through codegen (TECH_DEBT 4.1). That is mitigated by the no-behaviour first commit and a stop-and-diagnose rule.
- **NW-0 multiplies link evaluations.** At 5 subjects + 4 finders + N gateways, the evaluations per frame grow roughly with the number of registered nodes. Wall time is measured at the NW-0 commit. If the gate suite becomes impractical, the preset gate runs at 600 s.
- **Rule 8 applies across every preset comparison.** The preset adds transmissions, so no magnitude compares across the preset and the default. Only within-preset paired arms, and population or control-flow claims, are admissible.
- **Some mechanisms may still have a near-zero population under the preset:**
  - CI equal-sequence arrivals;
  - witness refusals un-absorbed by the evidence gate (decision 7).

  G24 reports these as findings with their denominators and never fakes them. The register records the reason.
- **The spec's own predictions may fail**, e.g. calibrated PCS ≡ flat threshold. Following the spec faithfully is the goal; the results are reported as they fall.
