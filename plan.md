# CA-TBL — plan to improve localisation accuracy, and where ML belongs

> **For the next 6–7 hours use `plan_6h.md`** (two independent parts, no ML). This file keeps the long-range list; its WP-A2 is stale because the TDoA gate is now in the preset (`e308c14f`).

Plan, 2026-10-08. Evidence is in `context.md` (cited as C§n). Work obeys RULES.md §1–§4.
Remit: the engine. No thesis prose here.

## 0. Conclusions first

1. **The accuracy that matters is limited by the filter's consistency, not by ranging.** Ranging
   errors are calibrated and independent (C§6.6). The EKF treats persistent IMU errors as
   independent increments, so its covariance is wrong by 3–54× in variance, depending on arm and configuration (C§6.5). Gating on a
   wrong covariance backfires (C§6.5). Fixing the covariance is the largest measured lever:
   open-loop replay, endgame, tag 0: CEP50 4.9 → 1.7 m, CEP95 17.8 → 5.4 m (C§6.8).
2. **At 5 km the CEP95 tail is one confident wrong TDoA fix on a loose prior** (C§6.4). The cure
   exists as a flag (`--tdoaInnovationGate`: −139.4 m CEP95 in the simulator, N.25). It is not
   adopted.
3. **ML has little to add in this simulator today.** Two feasibility tests were negative (C§7.3),
   and there are structural reasons (C§7.2). The one ML item with large potential, a learned inertial
   odometer, cannot be demonstrated without real IMU data. §4 says what to do with each.
4. **The requirement is read two ways** (C§2, C§6.2). Until the owner picks one, "improving
   availability" is ambiguous. This plan reports both.

## 1. Rules every work package follows

Straight from RULES.md; stated once.

| rule | what it means here |
|---|---|
| default-off flag | each mechanism gets a `cmd.AddValue` in the `main()` block, help text as an argument |
| null control | flag off ⇒ `arm_summary.csv` and every trace byte-identical to the pre-change binary, on both configurations, before any cell is read |
| pre-registration | `report/PREREG_<name>.md` committed before the first measured cell; predictions numeric, falsifiers reachable, abort conditions stated |
| gate + negative fixture | a gate per mechanism in `scripts/catbl_gates.sh`, a `/tmp` fixture that makes it fail, in the same commit |
| provenance | `run_id.txt` with `dirty 0`; frozen worktree and binary as in `scripts/lora_cmp2_run.sh` |
| rule 8 | compare arms only inside one binary; add reproduction cells against the previous binary; cross-binary magnitudes are not claims |
| seeds | fit or tune on 21–40, evaluate on 1–20, paired, seed bootstrap (FIXES-N26's convention) |
| atomic commits | one change per commit, evidence in the message, no assistant attribution |
| do not | fix the PCS saturation casually; edit `run_id.txt`; regenerate tracked results in place; set `hopWeight > 0` |

Configurations: **A** = `--specProfile` (5 km); **B** = A + `--boxHalf=750 --finderMode=1`;
**C** = A + `--nGateways=8 --gwLayout=legacy` (sensitivity). Adoption rule: reuse FIXES-N26's —
in A and B, availability gain with interval above 0, tag-0 CEP95 within +10 % of the preset's,
subject energy not higher; **plus** both availability definitions reported (C§2).

## 2. Ranked work packages

| # | WP | kind | evidence | expected effect (open-loop unless stated) | cost (estimate) |
|---|---|---|---|---|---|
| 1 | **WP-0** instruments | code, small | C§6.3, §6.1 | none by itself; precondition for 2–5 | about 1 week |
| 2 | **WP-A1a** IMU process-noise scale | code, ~20 lines | C§6.8 control | B CEP95 −41 %, CEP50 −33 % at ×10 | 3 days + 1 campaign |
| 3 | **WP-A2** adopt the TDoA innovation gate | registration only | N.25; C§6.4 | A CEP95 −139 m (simulator, measured) | 2 days + 1 campaign |
| 4 | **WP-A1b** error-state IMU filter | code, moderate | C§6.5, §6.8 | B CEP50 −65 %, CEP95 −70 % with gates (−57 % / −48 % without) | 2–3 weeks |
| 5 | **WP-A3** accuracy-based availability and a dead-reckoning comparator | analysis + decision | C§6.2, §6.3 | reframes results | 3 days |
| 6 | **WP-A4** dual base, 2.4 GHz on demand | code, small | FIXES-N26 §5 | 5 km energy back under the preset's | 1 week |
| 7 | **WP-A5** UWB: cap the tier, measure the saving | code tiny + campaign | C§6.9, N.26 | energy, no accuracy cost expected | 1 week |
| 8 | **WP-A6** tier-1 SF/BW sweep at 5 km | compute only | C§5.1, C§6.4 | unknown; sizes TDoA stamp supply | 2 days |
| 9 | **WP-A7** TDoA root choice by prior; two-stamp TDoA | measure first | C§6.4 | unknown | weeks |
| — | ML track | conditional | C§7 | §4 | §4 |

Order: 1 → (2 ∥ 3) → 4; 5 any time; 6–9 after 4's result.

---

## 3. Non-ML work packages

### WP-0 — Instruments (precondition)

**Why.** Every finding in C§6 needed a log and a knob the tree lacks. The 2026-10-08 work used an
uncommitted patch (`analysis/accuracy_replay/rangelog_patch.txt`) and a Python re-implementation.

| item | change | note |
|---|---|---|
| `--rangeLog` | promote the patch: tag 0's `Step()` input and the queued TDoA fix, written beside the trace, digested | draws no RNG; null control byte-identical; gate + fixture |
| `--imuBiasScale`, `--imuScaleScale`, `--imuArwScale` (default 1) | multipliers on `CaTblImuParams` | the IMU error is currently unreachable from the CLI; without this no 5 km number can state its sensitivity |
| `--arms=dr_only` | an arm that never fuses radio (prediction only, truth start) | the comparator that says whether the radio adds anything; C§6.3 found it adds the tail at 5 km |
| `--maxTier=1\|2` | cap escalation | for WP-A5; default 2 (UWB allowed) |
| replay fidelity | port DC-7 inflation, Case A/B/D into `replay.py`; gate: median \|replay − simulator\| < 1 m on `static_868`, `ca_tbl_full` | until then, replay results are variant differences only (C§6.1) |

**Done when.** Null controls pass; the replay mirror reproduces the simulator's estimate to < 1 m
median on both arms; `dr_only` appears in `arm_summary.csv` only when named (`--arms=all`
unchanged).

### WP-A1a — IMU process-noise scale (the cheap control first)

**Why.** Replay B, both filters starting at 1 m (C§6.8): `ca_tbl_full` CEP50 / CEP95
4.9 / 17.8 → 3.3 / 10.5 at ×10, 3.0 / 10.4 at ×30; `static_2p4` 4.3 / 12.9 → 2.7 / 9.6.

**Change.** `--imuQuScale` (default 1.0): multiply the IMU increment covariance handed to
`PredictWithControl`, for tag 0 and for the subjects. `scratch/ca_tbl_causal_sim.cc` at the two
`SetImuControl(...)` call sites.

**Registered predictions (closed loop, tag 0, seeds 1–20).**
- B, ×10: per-seed mean CEP95 change ≤ −20 % (half the replay effect, to absorb loop effects).
- A, ×10: CEP95 not worse than the preset's by more than +10 %.
- Availability (fix) within −0.01 of the preset in A and B; subject energy not higher by > 5 %.
- Integrity-check SUSPECT share and escalation counts are reported, not predicted (they move
  with P).

**Falsifier / abort.** B CEP95 improves by less than 10 % at every k in {3, 10, 30}: the replay
effect does not survive the loop, and A1b is re-scoped.

**Also reports** the NEES median of `error²/trace` per tier (target: from 37 toward 1).

### WP-A2 — Adopt the TDoA innovation gate

**Why.** (i) Simulator, paired 20 seeds: 5 km tag-0 CEP95 −139.4 m [17.2, 284.7] at −0.003
availability and −0.06 J (N.25). (ii) Replay, 5 km: −484 m [−863, −144] on `static_868` (10/0
seeds). (iii) Mechanism understood (C§6.4): one TDoA fix, claimed σ ≈ 25 m, 2.6 km wrong, on a
prior of 100 m.

**The interaction that makes this a factorial, not a flag flip.** On an inconsistent covariance
the gate hurts in B: `ca_tbl_full` CEP95 +6.9 m [1.2, 13.1] (replay, C§6.5). So register
**2 × 2**: {gate off, on} × {Qu scale 1, k*}, where k* is A1a's best. Adopt the gate only if it
is non-negative in B under the retained Qu scale.

**Change.** None: `--tdoaInnovationGate` and `--tdoaInnovationChi2` exist (G89). The registration
is the work.

**Prediction.** A: CEP95 −100 m or better at Qu scale 1; B: no worse than +10 % at k*; energy
unchanged ±1 %. **Falsifier.** B CEP95 worse by > 10 % at k*.

### WP-A1b — Error-state IMU filter (heading bias and scale as states)

**Why.** Largest measured effect (C§6.8). Paired per seed, augmented + gates vs baseline, replay:

| arm, cfg | Δ CEP50 (m) | Δ CEP95 (m) | seeds better (CEP95) |
|---|---|---|---|
| `ca_tbl_full`, B | −4.10 [−5.91, −2.50] | −9.04 [−12.91, −5.68] | 19/20 |
| `static_2p4`, B | −3.00 [−4.22, −1.89] | −6.86 [−9.27, −4.86] | 19/20 |
| `static_868`, B | −8.56 [−12.19, −5.69] | −41.05 [−65.65, −20.95] | 20/20 |
| `static_2p4`, A | −7.05 [−11.92, −3.04] | −20.48 [−37.74, −7.95] | 13/20 |

It stays at ~2 m (B) as the gyro bias grows ×1 → ×30 where the baseline goes 5 → 12 m (C§6.8).
It is also the only variant that makes gates safe.

**Change.** A second filter class selected by `--imuErrorStates` (default off), behind the same
interface the policy uses (`X, Y, Pxx, Pxy, Pyy, TracePos, PriorX/Y, Update, FusePositionCov,
PositionNis, MahalanobisFromPriorWithR, RevertToPrior, InflatePos`, velocity accessors).
- States `[x, y, ψ, b, s]`; prediction `Δ = (1−s) R(−ψ) u`, `ψ += b·dt`; priors from
  `CaTblImuParams` (and from WP-0's multipliers).
- **Add a bias random walk `q_b`** (not in the replay). The simulator's bias is constant, so the
  replay flatters the filter; a real gyro bias drifts. `q_b` is OURS and must be swept.
- Velocity states are removed from the filter's own prediction (the IMU supplies displacement);
  expose `Vx/Vy` as `u/dt` so `mobilityFromRange`, `entryVelocity` and the witnesses are unchanged.
- The IMU dead-reckoner's `ResetAccumulation()` logic is replaced by the states: no more
  increment-covariance bookkeeping.

**Stress tests (registered, before the headline).** Each of: bias random walk at 10× the
generator's `σ_b`; a step change in bias mid-run; a mis-declared prior (filter σ_b 0.1× and 10× the
truth); scale error 3× the declared. Pass: no worse than the baseline EKF at the same IMU
quality. These replace "the model matches the generator" with evidence it does not need to.

**Registered predictions (closed loop).** B, `ca_tbl_full`, tag 0: CEP50 ≤ 3.0 m and CEP95 ≤ 11 m
(replay 1.7 / 5.4 with gates, 2.1 / 9.2 without; loop margin ≈ 2×); median `error²/trace` ≤ 10
(37.5 baseline, 6.3 in replay). A: CEP95 not worse than the preset's; tag-0 within-25 m share not
lower.

**Dependencies and side effects.** PCS thresholds were fitted on the old P (CAL-03). They are
inputs, not code: **do not alter the PCS functional form**. After adoption, re-fit CAL-03's
weights and thresholds on seeds 21–40 by its own procedure, as a separate registered step. The
integrity check's distance (N.15) uses P: re-measure its fault-free SUSPECT share (target
≤ 0.05, FIXES-N26 Q7) before and after.

**Falsifier / abort.** Closed-loop B CEP95 improvement < 20 % over A1a's best, **or** any stress
test worse than baseline by > 10 %: stop at A1a and record the gap.

**Risk.** The replay is matched to the generator by construction (C§6.8). The stress tests are
the mitigation; they are not proof for real hardware.

### WP-A3 — Availability by accuracy, and a dead-reckoning comparator

**Why.** C§6.2: the reported availability ignores accuracy. `ca_tbl_full` is 5.6× `static_2p4` on
it at 5 km, and behind it on "within 25 m" (0.615 vs 0.683). C§6.3: dead reckoning alone is
25.7 / 112.8 m at 5 km, the system 24.0 / 626.5 m.

**Do.**
1. Owner decision: which definition is *the* requirement (C§10 item 1).
2. Add "within 25 m on the cycle" and "within 25 m **with** a radio fix" as columns in
   `camp_spec_analyse.py` / the paper tables; keep fix-availability.
3. Report `dr_only` (WP-0) in every comparison table.

**No engine change beyond WP-0.** Any statement about "meeting A ≥ 0.95" must name the definition.

### WP-A4 — Dual base, 2.4 GHz on demand

**Why.** FIXES-N26's one failing term: 5 km subject energy 16.69 vs 12.37 J. The dual base (`D`)
alone costs +5.35 J there, because 2.4 GHz is ranged every cycle where it rarely reaches; it
carries the availability (+0.118 at 5 km).

**Change.** `--loraDualBaseOnDemand` (default off): range 2.4 GHz only when the 868 MHz floor has
fewer than three ranges, or a 2.4 GHz neighbour was heard in the last TTL. Registered by the
handoff as the follow-up (`HANDOFF_2026_10_05.md` §0).

**Prediction.** 5 km subject energy ≤ 13.6 J (+10 % on the preset's 12.37) with availability gain
≥ +0.08; endgame unchanged from `ALL`. Both bounds are OURS. **Adoption under the unchanged rule
needs ≤ 12.37 J**, so meeting the prediction does not imply adoption; relaxing the rule is the
owner's registered amendment, not this plan's. **Falsifier.** Either bound missed.

### WP-A5 — UWB: stop paying for it, measure what it buys

**Why.** 900 UWB-active cycles in B, 0 fixes, never three finders in range, UWB 64.6 % of endgame
subject energy (C§6.9; N.26). Probe PDR at 10 m is 0.30.

**Do.** With WP-0's `--maxTier=1`, run B with UWB disabled, paired 20 seeds.

**Prediction.** Subject energy −30 % or better (estimate: UWB is 64.6 % of it, and about half is
recoverable once the escalations stop); availability and CEP95 within the intervals of the preset
(no UWB fix exists to lose). **Falsifier.** CEP95 or availability worse outside the
interval: UWB contributes something the fix count hides (Case D, IMU reset), and WP-A5 becomes a
mechanism study.

**Do not** spend effort on "better UWB discovery" until this result is in: 4 finders for 5
subjects cannot supply three UWB nodes per subject (C§6.9).

### WP-A6 — Tier-1 SF and bandwidth at 5 km (no code)

**Why.** At 5 km 868 MHz PDR is 0.26–0.53 (3–6 km) and 3-stamp TDoA attempts are 3.8 % of the total
(190 of 4 948, C§6.4). `--sf868` and `--bw868` exist (SY-3, G36). A higher SF buys sensitivity and costs
airtime and duty.

**Do.** Sweep `--sf868` ∈ {7, 8, 9, 10} at A, seeds 1–20, with and without the TDoA gate. Report
3-stamp attempts, tag-0 CEP95, duty cap hits, energy.

**Prediction.** Registered before running; none is made here because no airtime-versus-stamp
model exists. If the sweep shows a state-dependent optimum (for example, SF by range), only then
is a learned rate adapter worth a registration (§4, ML-6).

### WP-A7 — TDoA root choice and two-stamp TDoA (measure first)

**Why.** The 3-stamp closed form picks between two roots with the RSSI ranges only
(`ca-tbl-tdoa.h`). In the C§6.4 event the filter's own prior (error 3 m, σ ≈ 100 m) would have
told roots 2.6 km apart at a glance; the RSSI chi² that chose between them was 0.135 against 4.31
(`tdoa_static_868_seed1.csv`, t = 42.5 s). Separately, 41 % of B's TDoA attempts and 23 % of A's
have exactly two stamps and are discarded.

**Do (measurement, no engine change).** Log both roots with their distances to truth and to the
prior; count wrong-root selections and how often the prior would have chosen correctly. Estimate
the two-stamp hyperbola's Fisher information against the 2.4 GHz and DR information in B.

**Then, only if the counts justify:** `--tdoaPriorRoot` (choose by prior Mahalanobis distance,
RSSI as tiebreak) and `--tdoaPartial` (a two-stamp range difference as a scalar update).
**Hypothesis, not a finding:** WP-A2 already removes most of the harm; these two buy the remainder.

---

## 4. ML track

**Standing position (opinion, from C§6–§7).** In this simulator the ranging data are calibrated
i.i.d. noise, NLOS is an oracle, and the IMU is a two-parameter generator. A model can only
rediscover these. The efficient route to accuracy is estimator structure (§3). ML is justified
where the *simulator is not the data source*, or where a measured gap remains after §3.

**Admission test for any ML component** (all required, in the pre-registration):
1. A named baseline that is not ML (a one-line rule or a filter change) and its number.
2. Training on seeds 21–40; evaluation on 1–20; seed-grouped cross-validation for any feature study.
3. A closed-loop effect, paired, in a default-off flag with a byte-identical null control.
4. Inference that is deterministic and dependency-free in C++ (trees exported as generated `if`
   chains; an MLP as a hand-written forward pass with weights in a tracked file).
5. A statement of what the model learned about the *simulator* versus the *world*.

| id | candidate | verdict | evidence | next step |
|---|---|---|---|---|
| ML-1 | learned fix-quality score (replaces PCS) | **do not do** | C§7.3 T2: ρ 0.26 / 0.35, top-decile AUC 0.63 / 0.69 vs trace 0.73 / 0.72; CAL-01 | reopen only with a feature the trace lacks (per-link innovation history, IMU consistency) and the T2 protocol |
| ML-2 | learned TDoA reliability | **do not do** | C§7.3 T1: AUC +0.003 / +0.013 over the NIS gate | WP-A2 |
| ML-3 | learned NLOS classifier | **not yet** | NLOS is an oracle in R and TDoA σ (C§6.7); population 0.3 % at 2.4 GHz, empty at UWB; current detector precision 3–8 % | first an ablation with the oracle removed (needs a flag); a classifier is worth building only if that ablation shows a gap, and its value is "gap closed" |
| ML-4 | learned inertial odometry (step length, heading) | **highest potential, not demonstrable here** | dead reckoning decides the 5 km result (C§6.3); SHS diverges (K.9) | see below |
| ML-5 | surrogate-assisted layout optimisation | **hypothesis** | gateways in a 21° sector from the east (C§5.2); GEO-01 tried 10 layouts (best A = 0.329) | see below |
| ML-6 | learned SF/BW adapter | **conditional on WP-A6** | none yet | decide after the sweep |
| ML-7 | `ml_pipeline/` RSSI regressor | **retire or re-scope** | not on a decision path (C§7.1); duplicates the simulator's path loss | owner decision DEC-06; see below |

### ML-4 — learned inertial odometry

**Why it matters.** The dead reckoner decides the 5 km result and its error model is the least
realistic part of the simulator (C§6.3). A learned odometer would replace the step-length and
heading models the SHS engine fails at (K.9).

**Why it cannot be shown here.** The simulator's IMU error is `u = (1+s)R(ψ)Δ` applied to true
displacement; a network trained on that learns `s`, `ψ` and noise. Any measured gain would be a
gain on a two-parameter generator.

**What would make it real.**
1. A public pedestrian-inertial dataset with ground truth, a trunk or pocket mount, and walking
   speeds like 1.5 m/s. Dataset choice, licence and citation must be checked first (CONTEXT_PACK
   §1: no invented citations); none is verified in this session.
2. Train and validate on that data **outside the simulator**. Report drift per distance on held-out
   subjects.
3. Use the held-out drift to set `CaTblImuParams` (bias, scale, ARW) and WP-A1b's priors and
   `q_b` from data. This alone is a realism gain and needs no network in the loop.
4. Only then swap the learned model in as the source of `u_k` behind a flag, with WP-A1b as the
   consumer that absorbs its residual bias.

**Gate.** Steps 1–3 first. Do not build step 4 until WP-A1b and WP-0's `--imuBiasScale` exist.

### ML-5 — surrogate-assisted layout optimisation

**Hypothesis.** Availability and accuracy depend strongly on gateway layout (GEO-01), and the
spec's three gateways sit on one side of the box. A Gaussian-process surrogate with Bayesian
optimisation over (gateway positions, finder dispatch) can search more cheaply than GEO-01's grid
of ten. Each evaluation is about 85 s per three arms per seed (C§9).

**Constraint.** The layout is a spec and owner decision (DESIGN_DECISIONS §9). The result is advice
about the deployment, not a change to the preset. Both availability definitions must be the
objective (WP-A3). Evaluate with held-out seeds; a surrogate fit on 1–20 and scored on 1–20 is
not a result.

**Do not start** before WP-A1b: the consistent filter moves every layout's score.

### ML-7 — the existing pipeline

`ml_pipeline/` has no consumer, and its dataset (868 MHz only, random links) is not the CA-TBL
scenario. Choices for the owner: (a) delete it from the handoffs and README; (b) keep it as the
seed of ML-5's surrogate by replacing its target (RSSI) with scenario outputs. This plan
recommends (a) unless ML-5 is approved.

---

## 5. Pre-registrations to write, in order

| file | covers | before |
|---|---|---|
| `PREREG_WP0_INSTRUMENTS.md` | null controls and replay-fidelity gate | any code merge |
| `PREREG_A1A_QU_SCALE.md` | WP-A1a | first cell |
| `PREREG_A2_TDOA_GATE.md` | WP-A2, 2 × 2 with A1a | first cell |
| `PREREG_A1B_ERROR_STATE.md` | WP-A1b with stress tests | first cell |
| `PREREG_A5_UWB_CAP.md` | WP-A5 | first cell |
| `PREREG_A6_SF_SWEEP.md` | WP-A6 | first cell |

Each carries: population and denominator (seeds 1–20, 720 cycles, tag 0 for CEP; five nodes for
availability), the estimator (paired seed bootstrap), numeric predictions, falsifiers, abort
conditions, and "what this campaign cannot establish".

## 6. Decisions needed from the owner

Only those that change the work.

1. **Which availability definition is the requirement** (accurate estimate vs radio fix). Blocks
   WP-A3's framing and every "meets the requirement" sentence.
2. **Is the IMU error model acceptable as is?** If not, WP-0's multipliers plus ML-4 steps 1–3 come
   before any 5 km claim.
3. **Start-at-truth with a 100 m prior**: scenario choice or artefact? It decides whether the 5 km
   tail is a result or a setup effect (C§6.4: a 1 m prior removes most of it).
4. **Approve or decline ML-5** and **DEC-06** for `ml_pipeline/`.

## 7. What would change this plan

- WP-A1a's closed-loop B effect under 10 % at every k: the replay does not survive the loop, and
  the order becomes WP-A2 → WP-A5 → WP-A4.
- A real-IMU residual drift that is not a constant bias plus scale: WP-A1b's states must be
  re-derived (a bias random walk is already planned for).
- An oracle-removed NLOS ablation that costs accuracy: ML-3 moves up.
- A new feature the covariance trace lacks that predicts error out of sample: ML-1 reopens.

## 8. Not planned, and why

| item | reason |
|---|---|
| fix the PCS | RULES.md §2; PCS carries no information beyond the trace (CAL-01); its inputs change under WP-A1, its form does not |
| UWB discovery improvements | 0 fixes, never three finders in range (C§6.9) until WP-A5 reports |
| learned ranging correction | errors calibrated and independent; only a −8…−15 % long-range bias, small against σ ≈ 0.6 d (C§6.6) |
| particle or grid filter for tier 1 | with dead reckoning carrying the state, tier-1 information is near zero at 5 km; no measured headroom |
| hop-aware confidence | no population (`hopWeight` 0) |
