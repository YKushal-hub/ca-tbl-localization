# CA-TBL — completion plan, 2026-10-14

Written after merging `origin/main` (merge `8f121da`) and auditing the IMU. Scope is the
**engine**: ns-3 code, experiments, gates, analysis. No thesis prose (the repository's
working rules restrict this tree to the engine).

Everything below is either **measured this session** (marked ✔ with how), **read from a
repo document** (cited), or **a recommendation** (marked ▶). Estimates are my own and
unmeasured.

## 0. Status — updated the same day, after the owner's decisions

| decision | answer |
|---|---|
| D-2 IMU | **Build it.** Done: §2 below, commits `c7f12d7` (sensor, G17) and the wiring commit after it |
| D-5 Python study | **Ignore unless it harms the project.** ✔ It does not: nothing under `scripts/`, `scratch/python_handoff/`, `src/`, the gates, or any authoritative document runs or cites `multimodal_generator*.py`, `evaluate_3state.py` or `validate_multimodal.py`. The one engine reference to `simstatus.md` (`lora-ranging-submodule.cc:32`) cites its §4.1 CRLB arithmetic, not the IMU. Left untouched |
| D-3 C2–C4 | **Deferred (owner, 2026-10-15)**, after the IMU result: the architecture still fails A0 v2 at R = 25 m with or without the inertial prior, and D-8 moves every tier-2 number first. Revisit once D-8 and the evidence gaps are closed |
| D-8 | **Planned fix campaign (owner, 2026-10-15).** `--t2SensitivityRowKHz` built in the worktree (default off; A7 passes with it); stage 1 registered in `report/PREREG_D8_SENSITIVITY.md`; stage 2 re-measures Claims 2, 4 and the joint criterion after stage 1 |
| D-4 A0 | **May be new.** Registered as `report/PREREG_A0_V2.md`: A0's rule plus the fourth branch, on new cells only |
| D-1 trailers / G12 | Not answered. Still open |

**Found while executing, not in the original register:**

- ✔ **The merge is clean on the default path.** The merged binary reproduces
  `results/baseline_v7` byte-for-byte: `arm_summary.csv` and all six traces, by the sidecar's
  own SHA-256 digests (`source 8f121da dirty 0`). Phase 0.1 is closed.
- ✔ **T-ev-8: five directories the positive-results document cites were never in git, on
  any ref, and are no longer on disk** — `results/joint_criterion`, `siting_repair`,
  `claim2_rebase`, `claim4_src`, `p1_condfloor`. Commit `9a636d7` states the tracked set is
  "every summary figure the deliverables cite"; for these five it is not. Their invocations
  survive in `src/catbl-propagation-study/experiments/*_REGISTRATION.md`, so they are
  regenerable (post-D1, the code path reproduces); until they are regenerated and tracked,
  Results 1, 2, 4 and 6 of `CA-TBL_Positive_Results.md` have no evidence on this machine.
- ✔ **G12b was already red on the merged tree** — 10 tracked files carry a vendor name in
  their text, among them `report/PREREG_C9_ESCALATE.md`, `report/PREREG_PCS_FORMS.md` and
  `scratch/python_handoff/analyse_c9_escalate.py` from the 2026-09-05 branch. None of the
  files added for C3 do.
- ✔ **1 241 files in the tree were root-owned**, some predating this session, the rest from
  building as root. Returned to the tree's owner; builds and commits now run as that user,
  because `./ns3` refuses root (`refuse_run_as_root`, keyed on `$USER`).

---

## 1. The IMU

### 1.1 What exists — and where

**In the ns-3 engine: nothing.** No IMU model, no flag, no energy line. The predict step is a
4-state constant-velocity filter (`CaTblEkf::Predict`, `ca-tbl-policy.cc:189`, process noise
1.0 m/√cycle position, 0.5 velocity). Every "dead reckoning" in the engine's comments
(`ca_tbl_causal_sim.cc:24,233,702,2675`, `ca-tbl-policy.cc:781`) is **coasting on that
constant-velocity prediction** when fewer than three ranges arrive. `MASTER_PLAN.md` §2.1
lists the inertial prior as **ABSENT** (C3 / DEC-D).

**The thing you are looking at is in the prior study's Python pipeline**, tracked under
`scratch/`: `multimodal_generator.py`, `multimodal_generator_parallel.py`,
`evaluate_3state.py`, `validate_multimodal.py`, described in `simstatus.md` (which carries a
HISTORICAL banner: *"Do not cite numbers from this file"*). No authoritative document cites
it (✔ grep of Architecture, Simulation, Technical Report, MASTER_PLAN, docs/*). It is
disconnected from CA-TBL: different tier-1 observable (TDOA vs RSSI), pre-D1, no ns-3 link.

### 1.2 Why it is garbage — defects, each reproduced

Reproduced by running the functions verbatim (`docs/COMPLETION_PLAN_imu_probe.py`) unless marked.

| # | Defect | Evidence |
|---|---|---|
| IMU-1 | **The drift model has one error term out of the five that matter.** Only white accelerometer noise (`σ·t^1.5/√3`, correct for that one term). No accel bias, no gravity leakage from attitude error, no gyro bias *instability*, no turn-on offset, no scale factor | At t = 120 s the implementation returns **2.28 m**. A 1 mg accelerometer bias alone gives ½bt² = **70.6 m** at 120 s, 17.7 m at 60 s |
| IMU-2 | **The gyro term is not the integral it claims to be.** `v·t·sin(bt)`; the cross-track error from a heading ramp is `v(1−cos bt)/b ≈ v·b·t²/2` | 0.489 m vs 0.244 m at 120 s: **2× too large**. And it is periodic — returns to ~0 at bt = π, so drift "heals" |
| IMU-3 | **"5°/h, typical uncalibrated consumer-grade drift" is mislabelled.** | ✔ Checked against the Bosch BMI270 data sheet (BST-BMI270-DS000-04 rev 1.2): uncalibrated zero-rate offset ±0.5 °/s = **1 800 °/h**, temperature coefficient ±0.015 °/s/K; bias instability is not specified at all. 5°/h is two to three orders below an uncalibrated consumer part |
| IMU-4 | **Not an EKF.** `ekf_fuse` is a scalar inverse-variance blend of three magnitudes: no state, no covariance propagation, no innovation, no gating, no cross-covariance | `multimodal_generator.py:84-93` |
| IMU-5 | **The filter is an oracle.** The "variance" of the radio fix is the *realised* error: `rl = max(lora_err, 1)`, `rs2 = abs(s2_err) + 0.3` | same lines. The filter knows the truth it is estimating |
| IMU-6 | **The output is a σ, not an error — so the headline "IMU never hurts" is a tautology.** A parallel combination of variances is below every input by construction | ✔ 100 000 random trials: fused ≤ min(inputs) in **100 %**; median fused/lora_err = **0.0034**. `simstatus.md` §9 item 4 states it as a *finding* |
| IMU-7 | **Non-IMU arms receive a free 0.5 m IMU.** In `evaluate_3state.py` methods 1-3 set `tsf = 0`, but `ekf_fuse` floors the IMU term at 0.5 m, and `fpe_imu` is applied to every `iact` device | ✔ function-level: TDOA error 5 → 4999 m all report **0.498–0.500 m**. ▶ The output CSV (`results/5method_3topo_comparison.csv`) is not on this machine, so I have **not** confirmed it reached a published number |
| IMU-8 | **`tsf` is drawn, not simulated.** `rng.exponential(interval)` independent of any trajectory. The IMU never sees motion, turns, or stationary periods | `multimodal_generator.py:143`; `evaluate_3state.py:96` |
| IMU-9 | **The validation check cannot fail.** "Drift monotonicity r > 0.7" correlates `imu_drift(tsf)` with `tsf`. Even when it fails it only appends a `[WARN]` | `validate_multimodal.py:70-77,115-124`. Violates working rules 6 and 9 (forced-failure evidence; design against the failure mode) |
| IMU-10 | **Three divergent copies.** The `evaluate_3state.py` copy hard-codes `0.00294`, `1.732`, and *omits the gyro term entirely* | `evaluate_3state.py:6` |

**Disposition: do not repair it.** The correct response is to quarantine it (§3, T-1) and
build the model fresh inside the engine (§2).

### 1.3 What is required

The IMU is not decoration. Three requirements already on record:

1. **Spec:** Ch. 10 §10.3.1 specifies an inertial dead-reckoning prior in the predict step
   (`MASTER_PLAN.md` C3; `docs/CONTEXT_PACK.md:126`).
2. **The integrity check's "independent prior"** (§10.7.2) is currently *propagated from
   previous radio fixes*, so it is not independent in the sense specified (`CONTEXT_PACK.md`).
   CAL-05 has since shown the check **is a working detector** (`report/CAL05_DETECTION_CURVE.md`),
   which is the condition DEC-D set for the requirement to matter.
3. **The binding constraint in the evidence is availability, not accuracy.** A0 returned FAIL
   in 64/64 because availability binds at 0.78–0.90 while accuracy sits at 3–5 m
   (`report/A0_APPLIED.md`). Cycles with < 3 ranges "coast" (`ca-tbl-policy.cc:781`) and the
   unconditional metrics are dominated by that coasting — `static_uwb` fixes on 11.6 % of
   cycles and reports 150.6 m unconditional vs 55.2 m cep50 (`ca_tbl_causal_sim.cc:702`).
   `PHASE10_PREREGISTRATION.md` already charges no-fix cycles "the dead-reckoned error".

   ▶ **An IMU changes the dominant term in the headline unconditional metrics.** That is why
   it must be built correctly, and why it must be applied to **every arm** (rule 5) — an IMU
   given only to the tiered arm would manufacture an advantage.

---

## 2. Work package: IMU (C3 / DEC-D)

Follows the repository's required shape for a new mechanism exactly: default-off flag, null control, pre-registration, gate with
negative fixture, provenance. Estimates are mine.

| WP | Deliverable | Notes | Est. |
|---|---|---|---|
| **IMU-0** | **Decision (owner): build, or formally exclude.** | If excluded, Ch. 10 must not specify a prior the system never has (`THESIS_TASKS.md` DEC-D). ▶ Recommend build — §1.3 | hours |
| **IMU-1** | `ca-tbl-imu-model.{h,cc}`: sensor error model driven by the *true* trajectory | White noise (from noise density), bias random walk / instability (Gauss–Markov), per-run turn-on bias, scale factor. **Own RNG via `SetStream`; never touches `macRng`** (`:1932`) — otherwise it perturbs every arm (rule 8). Each parameter carries `source: cited \| ours`; values from a named datasheet — I have supplied none, and have not verified any | 2–3 d |
| **IMU-2** | Mechanisation: planar pedestrian dead reckoning (heading integration + stride/step model) with ZUPT | ▶ Not free-running strapdown: the arithmetic in IMU-1's row shows 1 mg bias → 70 m in 120 s, unusable. **Caution:** the mobility model's dwell is *not* stationary — it moves at `0.3 × speed` (`ca_tbl_causal_sim.cc:834`), so ZUPT needs a stated threshold or a mobility change; decide before coding | 3–4 d |
| **IMU-3** | `--imuPrior={off,on}`, default **off**. Predict step takes an IMU displacement increment with a **model-derived** covariance | The filter's assumed Q comes from the *parameters*; the sampled truth error is separate. Never feed realised error in as σ (IMU-5). Include a deliberately mis-specified-bias case — an over-confident filter is exactly what the integrity check should then catch | 2 d |
| **IMU-4** | Energy line in `ca-tbl-energy-model` from a datasheet current, charged per cycle to every arm that has the IMU | §6 invariant unaffected (the IMU is not a transmitting tier), but add a verifier check that IMU energy is never booked to a tier | 1 d |
| **IMU-5** | Verification, independent of the implementation (below) | Rule 6/9: write down what a firing looks like on disk first | 3 d |
| **IMU-6** | Pre-registered campaign (below) | `PREDICTION.md` committed **before** the first cell (rule 13) | 3–4 d incl. compute |
| **IMU-7** | `Architecture.md` section; update `CONTEXT_PACK.md` row; close DEC-D | Hand the thesis project a note; do not write Ch. 3 / Ch. 10 here | 0.5 d |

**IMU-5 verification — each needs a negative fixture built in `/tmp`:**

1. **Closed-form error growth** (computed by a *separate* script, not the model's code — the
   six-decibel lesson): white accel noise → σ_pos = σ_a·t^1.5/√3; constant bias → ½bt²;
   gyro bias → cross-track `v·b·t²/2`. Monte-Carlo over ≥ 20 seeds inside CI.
2. **Allan deviation** of the generated stream recovers the input noise density and bias
   instability.
3. **ZUPT bounds** drift during stationary periods.
4. **Null control:** `--imuPrior=off` byte-identical to the pre-change binary — diff
   `arm_summary.csv` and every trace.
5. **Reachability (rule 4):** an exact-zero IMU contribution is a control-flow claim. Count
   IMU predict calls; assert > 0 when on.
6. **Forced failure:** zero one error term at build time → the gate must go red.

**IMU-6 campaign — questions, not predictions (the numbers come from IMU-1's error budget):**

- E1 *Bridging:* error during no-fix cycles, IMU vs constant-velocity coast, all arms.
- E2 *Availability:* does the IMU move the 0.78–0.90 availability bind? **Depends on A0's
  open branch (§3, C-1)** — cannot be registered until the criterion is settled.
- E3 *Independent prior:* repeat CAL-05 with `--imuPrior=on`. Honest framing: the IMU prior is
  still anchored to the last accepted radio fix, so it is independent in its *increment*, not
  its origin; error grows with time since that fix. Register the prediction that detection
  improves for a bounded gap length and not beyond, with that as the falsifier.
- 20 paired seeds minimum; seeds are the independent unit; no single-seed cross-arm claims.

---

## 3. Technical-debt register

### 3.1 Contradictions between documents (the front page misleads)

| ID | Contradiction | Where |
|---|---|---|
| T-doc-1 | `Simulation.md` §14 says **"Claims 1–7 established with statistical support"** and **"no further experimental work"**; its own §11 ledger shows Claim 1 withdrawn (W12), Claim 3 withdrawn twice (W4, W11), Claims 2 and 4 re-scoped, Claim 7 re-scoped | `Simulation.md:945,939-948` vs `:551-570` |
| T-doc-2 | `MASTER_PLAN.md` (remote, 09-05) predates the local W11/W12 withdrawals and says A5 / D-1 states that differ from local work. **Two lines of work, each unaware of the other**: local did D5 repair (`MIN`) and complementary siting; remote did `weighted`/`gated` and lists A5 "not started" | `MASTER_PLAN.md:238-244`; `SESSION_2026_09_05.md` §5 |
| T-doc-3 | `docs/WORK_PACKAGES.md` still lists D-3 READY (already fixed) | `MASTER_PLAN.md` D-3 |
| T-doc-4 | Tier-3 `r80`: 12.8 m vs ≈ 13.5 m in two authoritative docs | `MASTER_PLAN.md` §4.4 |
| T-doc-5 | `CA-TBL_Positive_Results.md`: 3 of 50 figure assertions fail — 84.7 % vs computed 84.8 %; median taken as upper-of-two (170.0) vs averaged (167.5); 6.53× is an *arithmetic* mean where the project's own estimator is geometric | commit `2bac44f` |

▶ **Fix:** one reconciliation commit per document, citing the ledger; then a gate that parses
the §11 claim-status column and fails if any other document asserts a status that differs
(with a negative fixture).

### 3.2 Evidence and provenance

| ID | Debt | Detail |
|---|---|---|
| T-ev-1 | **§4.1 reliability result does not reproduce.** Published 19/20 vs 0/20, energy 1.56×. Re-run at the dwell it is stated at: 1/20 and 0/20, 52×/21×. Corroborated by the 320-cell grid: 0/320 | `SESSION_2026_09_05.md` §3. No claim-ledger row yet reflects this |
| T-ev-2 | **A cited results directory was absent** (`results/p10_grid`) while cited for two headline results, and **no gate checks that a cited directory exists** | same |
| T-ev-3 | `[M-24]` rests on `dirty 4`; closing needs a fifth campaign, ~3.8 h | `MASTER_PLAN.md` §4.4 |
| T-ev-4 | Nine probe-derived numbers untraceable (incl. the r80 that sizes the geometry) | re-run `--probe=true`, capture stdout |
| T-ev-5 | **1 273 of 3 277 trace directories (~7 GB) can never be regenerated** — pre-D1 (1 047), no commit (164), no sidecar (62). Enumerated in `RESULTS_NONREGENERABLE.tsv` | commit `9a636d7` |
| T-ev-6 | **This machine holds 27 MB under `results/`** (✔ `du`), against ~18 GB cited. Traces are not here; `scripts/regenerate_results.sh --all` is "days of single-threaded compute" | decide what analysis needs traces |
| T-ev-7 | A0 returns FAIL 64/64 on a **degenerate** criterion (`D_T = D_B = 0`). A fourth branch is proposed and *deliberately not applied* — correct, since amending after seeing data is what pre-registration forbids | `report/A0_APPLIED.md` |

### 3.3 Code and gates

| ID | Debt | Detail |
|---|---|---|
| T-code-1 | **Merge not null-controlled.** The merge builds and the MAC suite is 38/38, but I did **not** run G1–G13, `verify_causal_sim.py`, or a byte-identity diff of `arm_summary.csv` against the pre-merge binary | Do first (§4, Phase 0) |
| T-code-2 | **Five `pcsForm` values now coexist**. The merge comment I inherited asserts `MIN` "is the form the campaigns were run against" — ▶ unverified by me; check against the sidecars | `ca-tbl-policy.cc:~700` |
| T-code-3 | `wip/b2-coverage-predicate` — "ACCEPTANCE UNVERIFIED", 6 files, +944/−864. Merge, rework, or delete | local branch |
| T-code-4 | `ca_tbl_causal_sim.cc` is **4 495 lines**; `RunArm()` is a single-tag monolith (`nodeId = 1`). Splitting it is a prerequisite for C2 and a risk to every byte-identity claim | do *with* C2, not before |
| T-code-5 | D-2 `LoRaDistributionSampler` ignores its seed — recorded decision not to fix | leave; re-confirm determinism after the merge |
| T-code-6 | D-5 receiver sensitivity **unverified** — a 6 dB error passed every gate | A7: independent closed-form check |
| T-code-7 | G3 red by design; G11b permanently red; **G12 red on trailers** (below) | known |
| T-code-8 | `./ns3` refuses root; I built via `cmake --build cmake-cache --target scratch_…` | document in README |

### 3.4 Repository hygiene

| ID | Debt |
|---|---|
| T-hyg-1 | 193 untracked root-level files, 150 of them `.md` (prompts, replies, handoffs). Nine `physics_scale_sweep_*.csv` at ~140 MB each at the repo root; `model.pt`, `Untitled.java`, `chat.txt`, `build.log`, loose `check_*.py` |
| T-hyg-2 | 103 tracked `scratch/*.py`, a large share one-off scripts from the prior ML/IMU study. DEC-G (role of the ML dataset) is still open |
| T-hyg-3 | 17 documents carry HISTORICAL banners; staleness is policed by prose, not a check |
| T-hyg-4 | **G12 vs commit trailers.** `report/G12_ATTRIBUTION_CONFLICT.md` records that the repo gates *against* AI attribution while the harness appends a `Co-Authored-By` trailer. **The merge commit I made last turn carries one**, so it adds to the red count. Not amended — your decision (§5, D-1) |

▶ **Hygiene rule:** nothing is deleted. Move to an archive directory *outside* the tree,
list in `EXCLUDED.md`, keep the gates' manifest logic intact. Rule 7 applies.

---

## 4. Sequence

```
Status markers: [done] measured and committed   [run] in progress   [next] next up

Phase 0  Ground truth (before any new feature)                      ~2 d
  0.1  [done] merge reproduces baseline_v7 byte-for-byte (summary + 6 traces, by digest)
  0.2  bash scripts/catbl_gates.sh, UNPIPED, record G1–G13 as a table
       expected red: G3, G11b, G12   — anything else is news
  0.3  verify_causal_sim.py (141 checks / 161 gates)
  0.4  Resolve T-code-3 (wip branch) and T-code-2 (verify the MIN comment)

Phase 1  Stop the documents lying                                    ~2 d
  T-doc-1…5, T-ev-1 ledger row, then the status-consistency gate (+ negative fixture)

Phase 2  Evidence gates                                              ~2 d
  T-ev-2: gate that every directory a document cites exists and has a sidecar
  A7 / T-code-6: independent closed-form sensitivity check, + negative fixture
  T-ev-4: re-run --probe=true, capture stdout

Phase 3  IMU (§2)                                                    ~3 wk -> 1 day
  [done] IMU-0..5: sensor c7f12d7, wiring 9f5085a, G17 (10 fixtures, filter NEES)
  [done] A0 v2 registered a4d6bf4; IMU campaign registered cb60c71
  [run]  IMU-6 primary: 120 invocations, seeds 301-330; then §8 bias sensitivity
  [next] IMU-7 result doc + ledger row; then E3 -- CAL-05 with the inertial prior,
         its own registration (fault-injection grid)

Phase 4  Track A remainder (compute-heavy, little code)              ~2 wk
  [done] A0 v2 registered; first applied to the IMU campaign's four cells
  A1  --dwell sweep at b3000 (no code change)
  A4  tier-1 calibration  → then A5 reconciled with local siting work
  A3  intra-cycle handshake deferral

Phase 5  Track C -- owner decides after the IMU result (D-3)        ~4–6 wk
  C2 two tags (+ split of the monolith, --nTags=1 byte-identity)
  C3 QUERY/RESPONSE → C4 arbitration.   C5 stays out of reach.

Phase 6  Closure
  T-ev-3 (M-24 fifth campaign), T-hyg sweep, regenerate-or-delete decision for results/
```

**Critical path:** 0 → 1 → 3. Phases 2 and 4 can interleave with 3's compute. Phase 5 is the
largest unknown and the first to cut.

**Abort conditions (register before each phase):**
- Phase 0.1 shows any flag-off diff → stop; nothing else is comparable (rule 8).
- IMU-5 closed-form check disagrees with the model → stop; do not run IMU-6.
- IMU-6 shows the IMU helps only the arm it was added to → the design leaked (rule 5).

**Do not:** port or "fix" the Python IMU; set `hopWeight > 0`; run a fourth PCS form (three
have failed the same control, `PCS_FORMS_RESULTS.md`); regenerate tracked results in place;
edit sidecars; write thesis prose here.

---

## 5. Decisions only you can make

| ID | Decision | Recommendation |
|---|---|---|
| D-1 | G12 / trailers: leave, rewrite history, or amend the gate. Also: amend my merge commit's trailer? | Earlier record recommends amending G12 to allow the trailer. Gates are not edited to pass, so it needs you |
| D-2 | **IMU: build or formally exclude** (DEC-D) | Build — §1.3. Exclusion is defensible only if Ch. 10 stops specifying the prior |
| D-3 | Build C2–C4 (DEC-E said build) or stop at C1 | Decide after Phase 3; Phase 5 is the largest unknown |
| D-4 | A0's degenerate FAIL: register a fourth branch as a *new* amendment | Yes, and the new cells must postdate it |
| D-5 | Quarantine the Python study (T-hyg-2) and settle DEC-G | Archive outside the tree |
| D-6 | Delete `results/`? | Not before T-ev-6 is decided; 1 273 directories are unrecoverable |

---

## 6. Hand-off — state at end of session, 2026-10-14

**Committed (local `main`, not pushed):** `c7f12d7` IMU sensor + G17 · `9f5085a` `--imuPrior`
wiring, flag-off byte-identical at 4 flag sets · `a4d6bf4` A0 v2 registration · `cb60c71` IMU
campaign registration, runner, scorer.

**Running in the background** (they finish on their own):
- IMU campaign → `results/imu_prior/` (120 invocations, log `/tmp/imu_campaign.log`).
  **Do not commit anything until it ends**: every invocation records HEAD at start-up.
- Gate suite at `cb60c71` → `/tmp/gates_cb60c71.log`.
- Worktree configure `/home/kushal/ns/catbl-wt` (build dir `build-wt`, A3 + A7 probe).

**Built, uncommitted, waiting for the campaign to end** (all under `scripts/`, `report/`, docs):

| item | files | next action |
|---|---|---|
| E3 — CAL-05 with the IMU prior | `report/PREREG_E3_IMU_INTEGRITY.md`, `scripts/sweeps/run_e3_imu_integrity.sh`, `scripts/e3_imu_integrity_analysis.py` | commit, then run (400 invocations) |
| §8 bias sensitivity | `scripts/sweeps/imu_prior_sensitivity.sh`, `scripts/imu_prior_sensitivity_analysis.py` | run after the primary |
| G18 cited-evidence gate | `scripts/cited_evidence_gate.py`, `scripts/catbl_cited_evidence_fixture.sh` (6/6 directions OK) | wire into `catbl_gates.sh` **after the running suite ends**; allow-list `results/x` (a quoted specimen) in an `EXCLUDED.md` ```reviewed-cited-evidence``` block. Red today: 15 cited paths absent |
| regeneration of 4 missing cited dirs | `scripts/sweeps/regen_cited_evidence.sh` (reconstruction, labelled) | run, then `python3 scripts/make_figures.py` to reconcile |
| A7 sensitivity fidelity (D-5) | `scripts/sensitivity_fidelity_check.py`, `scripts/catbl_sensitivity_fixture.sh` (5/5 OK), probe `catbl-wt/scratch/ca_tbl_sensitivity_probe.cc` | build probe in worktree, run check, wire as a gate |
| A1 dwell, re-specified | `report/PREREG_A1_DWELL.md`, `scripts/sweeps/run_a1_dwell.sh`, `scripts/a1_dwell_analysis.py` | commit, run |
| A3 intra-cycle handshake | worktree diff: `ca_tbl_causal_sim.cc`, `ca-tbl-energy-model.{h,cc}` | build, null control (4 flag sets), registration, campaign |
| `.gitignore` | whitelists `results/**/imu_summary.csv` | commit with the IMU results |
| docs | MASTER_PLAN, CONTEXT_PACK, THESIS_TASKS rows; this plan | commit after results |

**Progress after the restart (2026-10-15, before the IMU campaign finished):**
- A3 coded in the worktree; flag-off null control IDENTICAL at all four flag sets (24 files);
  smoke run shows 1 intra-cycle handshake, 0 pre-emptions, **1 blocked tier-1 exchange**.
  Patch saved; it lands in `main` with the A7 probe after the campaign.
- A7 run against the engines' real accessor: tier 2 −106.0 dBm vs −112.0 dBm data sheet at
  812.5 kHz (+6.0 dB), matching the 1625 kHz row; tier 1 −123 vs −124 dBm. D-8 is measured.
- G18 and G19 wired into `scripts/catbl_gates.sh`, both red by design; `EXCLUDED.md` gains the
  `reviewed-cited-evidence` block; `publication_ready.sh` still passes.
- Gate suite at `cb60c71`: PASS G1–G9, G13, G17; FAIL G9b, G10 (only the uncommitted
  registrations' runners), G11 (known), G12 (trailers, D-1), G12b (10 pre-existing files).
- Phase 1 document fixes made (uncommitted): tier-3 r80 12.8 m in the Technical Report;
  `Simulation.md` §14 restated from the §11 ledger; `WORK_PACKAGES.md` statuses;
  `MASTER_PLAN.md` §1 dated banner.
- A3 registration, runner and scorer written; `scripts/sweeps/run_registered_queue.sh` runs
  every new registration at one commit.

**Progress, 2026-10-15 (queue at `e94d04d` running; nothing below committed yet):**
- IMU campaign scored and committed (`89ac3b6`); result written, `report/IMU_PRIOR_RESULT.md`.
  Bias sensitivity scored: the short-outage gain survives 0.05–0.5 dps, eroding at 0.5.
- **IMU long-outage defect root-caused and fixed in the worktree.** Sensor states stay
  calibrated; only the position covariance fails — the EKF drops the second-order along-track
  shortening. An unscented predict is not enough (NEES 178 → 54 at 1000 s); moment-matched
  coasting, variance only, is: NEES 1.5–2.7 to 1000 s, mean error unchanged.
  `--imuCoastMomentMatch` (default off) wired; gate G20 + fixture written (plain EKF fails it).
- **D-8 fix built in the worktree:** `--t2SensitivityRowKHz=812.5` (default off), A7 passes with
  it, the scenario refuses a mismatched row. Stage-1 registration, runner, scorer written.
- Final worktree flag-off null control (D-8 + coasting + diagnostics) running.
- After the queue: land the worktree in atomic commits, wire G20, commit D-8 stage-1
  registration, run it.

**New defects found this session, recorded not fixed:**
- **D-8 (candidate): tier-2 receiver ~6 dB pessimistic.** The SX1280 table in
  `lora-2p4-physics-parameters.cc` is the data sheet's **1625 kHz** row (Table 6-1, rev 3.2:
  SF7 −106 dBm) while tier 2 is configured at **800 kHz** (812.5 kHz row: SF7 −112 dBm). The D1
  fix comment cites −106 as corroboration. A fix moves every tier-2 result — planned campaign,
  not a patch. A7 fails on it by design.
- **The escalation handshake never arms the duty-cycle off-time** (only
  `AccountRangingExchange()` calls `ArmDutyCycle`), and ranging never consults the governor.
  A3 corrects both under its flag only.
- Tier 1 checks out: SX1262 Table 3-8 (rev 1.2) SF7/125 kHz −124 dBm vs engine −123 dBm.
- Tier-3 r80: Technical Report says ≈ 13.5 m (line 70), Architecture and the probe say 12.8 m.
