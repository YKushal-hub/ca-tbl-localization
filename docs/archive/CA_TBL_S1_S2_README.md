# CA-TBL Phase S1 + S2 + S2.5 — Causal Handoff, Energy Accounting, Tiered Geometry

> **The results in `results/causal/arm_summary.csv` dated before 2026-07-30 are void.**
> Do not quote them. See "S2.5 — why the first run was invalid" below. They must be
> regenerated after `./ns3 build`.

## S2.5 — why the first run was invalid

S1 and S2 were structurally correct: the tier decision does gate the radio, and only the
active tier is charged. But the *scenario* they ran in made two of the three tiers
physically inoperable, so the numbers measured geometry, not policy.

One anchor triangle with 100 m sides was shared by all three tiers, while the node
random-walked inside a 1.2 km box. Consequences, all visible in the voided summary:

| Observed | Why it happened |
|---|---|
| `static_uwb` had `noFixCycles = 600/600` | UWB reaches ~100 m LOS / 30 m NLOS. It could not see 3 anchors 100 m apart from almost anywhere in a 1.2 km box. Its 288 m RMSE was pure dead reckoning. |
| `static_uwb` cost 0.68 J vs `static_868`'s 5.07 J | A radio that never completes a ranging exchange is never charged for one. The energy axis of claim 1 was **inverted** — the "ceiling" arm was the floor. |
| 497–600 of 600 cycles produced no fix in 5 of 6 arms | Every RMSE was EKF coasting. |
| `static_2p4` beat `ca_tbl_full` 3.8× on accuracy-per-joule | A single-band arm won because the hybrid's high tier did not function. |
| `suspects = 0` in `ca_tbl_full` | The integrity gate — the locked novelty claim — never fired once. |

A tier can only be shown to be worth escalating to if there exist places where it works
and places where it does not. A single flat anchor triangle cannot express that.

### What changed

**Tiered deployment** (`BuildDeployment()` in `scratch/ca_tbl_causal_sim.cc`), which is
also what the design document actually specifies:

| Tier | Anchors | Placement | Role |
|---|---|---|---|
| 868 MHz | 5 gateways | 4 corners + centre of a 3 km box | always available, coarse — the coverage floor |
| 2.4 GHz | 3 per site × 4 sites | 150 m triad radius at each facility | usable within a few hundred metres of a site |
| UWB | 3 per site × 4 sites | 35 m triad radius, co-located with tier 2 | usable only *inside* a site |

Spacing is taken from this repo's own measured data, not assumption: SF7 at 868 MHz holds
PDR ≥ 0.945 at 3 km in Suburban per `results/metric2_pdr_distance_seed7.csv`, and forest
(PLE 2.3) is less lossy than suburban (2.75). UWB's 100 m LOS / 30 m NLOS is the
citation-backed figure from Bug #9 in `simstatus.md`.

Tier 2 and tier 3 are deliberately **co-located**. If their coverage were in different
places, an escalation failure could be a siting artefact rather than a decision error.

**Facility-tour mobility** (`Tourist` replaces `Walker`). An unconstrained random walk
almost never enters a small site, so the escalation question is never posed. The tour poses
it repeatedly and in both directions, which is what makes hysteresis and de-escalation
observable at all.

**Ground-truth coverage labels.** Every step records `vis868 / vis2p4 / visUwb` — the count
of anchors of each tier within that tier's nominal range. This is pure geometry, computed
independently of the physics outcome, so it is not circular. `shouldUwb := visUwb >= 3`
gives the trigger a confusion matrix, and `arm_summary.csv` now carries
`trig_tp / trig_fp / trig_fn / trig_tn / trig_precision / trig_recall / trig_f1`. This is
the substrate claim 3 was missing.

**Pre-flight probe.** `--probe=true` sweeps reception rate and ranging error vs distance per
tier and writes `probe_range_sweep.csv`. The anchor spacing above is a claim about the
physics; this is the measurement that supports or refutes it. **Run it first.**

**Note on SF12:** it is not used for tier 1. In `metric2_pdr_distance_seed7.csv`, SF12 PDR
is 0.255–0.355 even at 500 m, far *below* SF7's 1.00 — an inversion that is
collision-dominated (SF12 airtime is ~30× SF7, so far more interferer overlap). Worth a
separate investigation; for now SF12 is not a usable tier-1 setting in this simulator.

## What S1 + S2 fixed

The previous pipeline (`ca_tbl_trajectory_generator_multi.cc` + `ca_tbl_fusion.py`) generated
868 MHz, 2.4 GHz **and** UWB rows for every timestep unconditionally, then let the Python policy
choose which tier's rows to read.

That made the escalation decision **non-causal**. UWB receptions existed even for steps where the
policy never activated UWB. Three claims were unprovable as a result:

| Claim | Why it failed |
|---|---|
| Hybrid beats single-band | Energy saved by *not* escalating could not be measured — the unused radio had already "run" |
| Handoff is worth it | Reduced to post-hoc data selection, not a control decision with consequences |
| Handoff triggers properly | Escalation latency and thrashing cost were invisible, so hysteresis could not be shown to do anything |

Now exactly one tier transmits per cycle — the one the policy selected on the previous cycle — and
only that tier is charged energy. An arm that never escalates accrues exactly zero UWB joules.

## New files

| File | Purpose |
|---|---|
| `src/lora-physics-engine/model/ca-tbl-energy-model.{h,cc}` | Per-tier radio energy: datasheet currents, Semtech ToA, UWB frame time, CAD / ranging / escalation / sleep charging, duty-cycle tracking |
| `src/lora-physics-engine/model/ca-tbl-policy.{h,cc}` | `CaTblEkf` (4-state range-only EKF), PCS, Mahalanobis integrity gate, escalation FSM with hysteresis, six policy modes |
| `scratch/ca_tbl_causal_sim.cc` | The causal simulation; runs all six arms over identical seeds and channel realisations |
| `scratch/python_handoff/verify_causal_sim.py` | Invariant checks — run this before reading any result |

`src/lora-physics-engine/CMakeLists.txt` has both new sources and headers registered.

## Build and run

```bash
cd ~/ns/ns-3-dev
./ns3 build
mkdir -p results/causal

# STEP 0 — archive the void pre-S2.5 numbers so they cannot be misread later
mkdir -p results/causal_VOID_pre_s2.5
mv results/causal/*.csv results/causal_VOID_pre_s2.5/ 2>/dev/null

# STEP 1 — pre-flight. Does each tier actually work at the distance it is sited at?
./ns3 run "ca_tbl_causal_sim --probe=true --outdir=results/causal"
```

Gates on the probe output. **If any fail, fix `BuildDeployment()` before going further** —
this is exactly the check whose absence produced the voided results:

- 868: PDR still usable (> 0.8) at 2000 m.
- 2.4 GHz: PDR falls off between ~200 m and ~600 m, not before.
- UWB: PDR high below ~70 m, near zero by ~130 m. **If UWB PDR is ~0 even at 5 m, the tier
  is misconfigured** and every `static_uwb` number is dead reckoning again.

```bash
# STEP 2 — single seed, all six arms
./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --outdir=results/causal"

# STEP 3 — verify before believing anything
python3 scratch/python_handoff/verify_causal_sim.py results/causal

# STEP 4 — seed sweep once the invariants pass
./ns3 run "ca_tbl_causal_sim --seedStart=1 --seedCount=20 --duration=3600 \
           --outdir=results/causal --trace=false"
```

Duration default is now 3600 s, not 600 s. At 1.2 m/s a 600 s run covers 720 m — barely one
facility transit, which is not enough escalation events to say anything about a trigger.

Geometry knobs if the probe demands changes: `--boxHalf`, `--uwbTriad`, `--t2Triad`,
`--speed`, `--dwell`.

## The six arms

| Arm | Role |
|---|---|
| `static_868` | Never escalates. Long range, coarse, cheap. Energy floor. |
| `static_2p4` | Pinned to tier 2. |
| `static_uwb` | Always-on UWB. Accuracy ceiling, energy ceiling. |
| `fixed_threshold` | Flat variance trigger, no confidence model, no integrity check. **The prior-art baseline.** |
| `ca_tbl_no_integrity` | PCS escalation only — isolates the integrity check's contribution. |
| `ca_tbl_full` | The proposed system. |

## How to read `arm_summary.csv`

The headline metric is `accuracyPerJoule` = `1 / (rmseM × totalJ)`. RMSE alone always picks
`static_uwb`; joules alone always picks `static_868`. Neither is the claim being made.

**Read `noFixCycles` first, before any accuracy number.** If it is a large fraction of
`steps` for an arm, that arm is measuring dead reckoning and its RMSE must not be quoted.
The verifier now hard-fails any arm below 50% fix coverage rather than warning, because a
20% warning threshold is what let the voided results through.

Expected shape if the architecture is sound:

- `static_uwb` — best RMSE, **worst** energy. (Pre-S2.5 it was worst RMSE and best energy;
  that inversion was the symptom of the broken geometry.)
- `static_868` — worst RMSE among arms that get fixes, best energy.
- `ca_tbl_full` — strictly between both on both axes, and above `fixed_threshold` on
  `accuracyPerJoule`.
- `cyclesInUwbCoverage` — a meaningful minority of steps, roughly 10–30%. At 0 there is
  nothing to escalate to; at ~100% escalation is unconditionally correct. Either way the
  policy is not being tested.
- `trig_f1` — `static_uwb` gets recall 1.0 and poor precision by construction;
  `static_868` gets 0. Only the policy arms' rows are informative. This is the first
  metric in the project that scores *the decision itself* rather than its consequences.

**If `ca_tbl_full` does not beat `fixed_threshold`, that is a result, not a bug.** It means the
confidence model is not earning its complexity over a flat threshold, and the honest response is to
report it and investigate why — not to tune thresholds until the ordering flips. Tuning six
parameters until the proposed arm wins on the same data used to tune them is exactly what a
reviewer will ask about.

## Known limitations — state these, do not hide them

1. **Thresholds are hand-set, not calibrated.** `1.55 / 1.70 / 2.30` are carried over verbatim from
   the Python reference. The plan calls for LOOCV calibration (step S7). Until that is done, any
   result sensitive to these values is not yet a result.
2. **`hopWeight` is 0.** PCS has a hop-provenance term wired in but disabled, because the scenario
   is single-hop — there is no mesh, so hop count is structurally always zero. **Do not claim
   hop-provenance confidence degradation until S5 lands.**
3. **The integrity gate requires all links to report LOS.** Inherited from the Python. It makes the
   flag specific but almost certainly costs recall. The LOS-fraction cut-off should be swept in S7.
4. **Sparse geometry degrades to dead reckoning.** Fewer than three ranges means the filter coasts.
   Case A/B/C resolution (2 anchors / 1 anchor + 1 peer / 0 anchors) from the design is not
   implemented. S2.5 makes this survivable on tier 1 (5 gateways, ≥3 visible across the box) but it
   remains the operative state whenever the node is between facilities on tier 2 or 3 — which is by
   design, and is what the escalation decision has to reason about. Watch `noFixCycles`; the
   verifier now hard-fails any arm below 50% fix coverage instead of warning at 20%.
5. **MCU energy is not modelled**, only radio energy. Regulator efficiency and PA ramp transients
   are also excluded.
6. **Still no packets and no MAC.** Ranging exchanges are charged energy and airtime, but no frame
   objects are constructed and no channel contention occurs. Claims 4 (custom handshake) and 5
   (specialized packets) remain at zero until S4.
7. **The trigger ground truth is geometric, not error-based.** `shouldUwb := visUwb >= 3` asks
   "could UWB have solved here", which is necessary but not sufficient for "escalating here was the
   right call" — escalating into coverage the node is about to leave still scores as correct. The
   stronger label (escalate iff it would reduce error by more than the energy is worth) belongs in
   S7 alongside the LOOCV calibration. Until then, report `trig_f1` as a coverage-tracking score,
   not as decision optimality.
8. **Facility siting is a designed layout, not a measured one.** Four sites on a symmetric square
   is a modelling choice; results will move with siting, and the sensitivity to it has not been
   swept. `--boxHalf/--uwbTriad/--t2Triad` exist so this can be done.

## Cross-check against the Python oracle

`scratch/python_handoff/{ekf_tracker,ca_tbl_fusion}.py` are retained deliberately. The C++ EKF uses
sequential scalar updates where the Python uses a batch update with a dense innovation covariance —
these are algebraically equivalent for diagonal `R`, which is what is used. Running both over the
same measurement sequence should agree to numerical tolerance. If they diverge, the C++ port is
wrong, not the Python.

## Next

1. **Run the probe, then regenerate.** Nothing in this file is a result until
   `probe_range_sweep.csv` clears its gates and `verify_causal_sim.py` exits 0. The previous
   summary is void.
2. **S3** — formal baseline sweep with confidence intervals. The arm list is in place; what is
   missing is 20+ seeds and CIs on every metric. `--seedCount=20 --trace=false`.
3. **S4** — packets + MAC layer. The next substantive build, and what unblocks claims 4, 5 and the
   "physics applied to real packets" framing of claim 6.

### If claim 1 still fails after S2.5

If a static single-band arm genuinely beats `ca_tbl_full` on `accuracyPerJoule` once the geometry
is sound, that is reported and diagnosed, not tuned away. The diagnostic order is: (a) is
`cyclesInUwbCoverage` in the 10–30% band, (b) is escalation latency/handshake cost dominating the
gain, (c) are the `1.55/1.70/2.30` thresholds placed anywhere near the PCS distribution the
scenario actually produces — check the `pcs` column histogram in the traces before assuming the
policy is wrong. Tuning six parameters on the same data used to evaluate them is the first thing a
reviewer will attack.
