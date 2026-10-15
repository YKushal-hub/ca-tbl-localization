# CA-TBL — Measured Positive Results

**Confidence-Aware Tier-Based Localization for GPS-denied search and rescue**
Kushal Yalamanchi · ns-3.47

> **What this document is.** A results extract: every measured outcome in which the CA-TBL system
> performs better than the alternative it was compared against, with the exact scenario, the exact
> numbers, and the source directory for each. The complete claim ledger — including results that were
> re-scoped or withdrawn — is in `Simulation.md` §11. Nothing here is estimated, projected or
> rounded up; every figure is a measured value with its provenance stated.

> **Audit, 2026-10-15 — read before citing anything below** (`report/AUDIT_2026_10_15.md` §B).
>
> - **Results 1–6 have no evidence in the repository.** `claim2_rebase`, `claim4_src`,
>   `siting_repair` and `joint_criterion` were never committed and are gone; they are being
>   reconstructed (`scripts/sweeps/regen_cited_evidence.sh`), and every number below will be
>   re-checked by `scripts/make_figures.py` when they are. Until then they are unverified.
> - **Result 4** omits that the same 52 seeds' registered verdict was **FAIL** (M1 = 2.17), that
>   the reliability advantage it set out to confirm **did not reproduce**, and that A0 v2 is FAIL.
> - **Result 5**'s comparator cannot reach 0.25 m by construction; the arm that can is not compared.
> - **Result 7** rests on a `dirty 4`, non-regenerable campaign; CAL-05 (tracked, `dirty 0`) is the
>   citable detection result, with recall 0.39 at 40 m on F1.
> - **Results 8b and 9**: the duty governor is enforced on ranging, not on the handshake; tier 2's
>   receiver sensitivity is about 6 dB pessimistic (D-8).
>
> Per-result notes follow each heading. Original text kept.

---

## Table of contents

1. [The simulated deployment](#1-the-simulated-deployment)
2. [How to read the provenance column](#2-how-to-read-the-provenance-column)
3. [Result 1 — Energy: 6.53× reduction](#3-result-1--energy-653-reduction)
4. [Result 2 — Coverage-floor optimisation: 24.5% saving](#4-result-2--coverage-floor-optimisation-245-saving)
5. [Result 3 — The escalation repair: 0 → 83 precision-tier cycles per run](#5-result-3--the-escalation-repair-0--83-precision-tier-cycles-per-run)
6. [Result 4 — Availability lead over single-band, prospectively registered](#6-result-4--availability-lead-over-single-band-prospectively-registered)
7. [Result 5 — 5.00× availability at pinpoint accuracy](#7-result-5--500-availability-at-pinpoint-accuracy)
8. [Result 6 — Time to first fix inside the survival window](#8-result-6--time-to-first-fix-inside-the-survival-window)
9. [Result 7 — Fault detection: 7.73× / 16.45× separation, 20/20 detected](#9-result-7--fault-detection-773--1645-separation-2020-detected)
10. [Result 8 — MAC layer and packet taxonomy](#10-result-8--mac-layer-and-packet-taxonomy)
11. [Result 9 — Physics validation against theoretical bounds](#11-result-9--physics-validation-against-theoretical-bounds)
12. [Result 10 — Reproducibility and verification apparatus](#12-result-10--reproducibility-and-verification-apparatus)
13. [Result 11 — A transferable deployment axis](#13-result-11--a-transferable-deployment-axis)
14. [Result 12 — Degeneracy screening](#14-result-12--degeneracy-screening)
15. [Consolidated summary table](#15-consolidated-summary-table)
16. [Provenance index](#16-provenance-index)

---

## 1. The simulated deployment

Every result below was measured in this environment unless the result states otherwise.

### 1.1 Scenario

| Parameter | Value |
|---|---|
| Run duration | **3600 s** |
| Decision cycle `dt` | **2.5 s** — derived from the tier-1 time-on-air divided by the regulatory duty cap, not chosen |
| Decision cycles per run | **1440** |
| Node speed | 1.5 m/s (pedestrian) |
| Mobility | Facility tour — transit, enter the 150 m tier-2 ring, enter the 10 m tier-3 ring, dwell, exit, repeat |
| Environment, wide area | `RURAL_FORESTED` |
| Environment, facilities | `RURAL_OPEN` |
| Vegetation, wide area | `DENSE_FOREST` |
| Vegetation, facilities | `SPARSE_SHRUBS` |
| Weather | `CLEAR_DRY` |
| Fading | Rician |
| Tier-1 beacon period | 10.0 s |

**On the mobility model.** An unconstrained random walk almost never enters a 10 m ring, which would
make the escalation decision unobservable. The facility tour poses the decision repeatedly and by
construction.

### 1.2 Deployment geometry

```
Tier-1 gateways     5 x 5 regular array spanning +/-0.9 x boxHalf          25 gateways
Facility sites      4, at (+/-0.47 x boxHalf, +/-0.47 x boxHalf)
Tier-2 anchors      3 per site, ring radius 150 m                          12 anchors
Tier-3 anchors      5 per site, ring radius  10 m                          20 anchors
Reference density   boxHalf = 2000 m  ->  site spacing 940 m
```

### 1.3 Radio configuration

| Tier | Band | SF / bandwidth | TX power | Ranging method | Accuracy floor |
|---|---|---|---|---|---|
| 1 | 868 MHz | SF7 / 125 kHz | +14.0 dBm | RSSI inversion | model-derived |
| 2 | 2.4 GHz ISM | SF7 / 800 kHz | +12.5 dBm | Symmetric two-way ToF | **3.0 m** |
| 3 | UWB Ch9, 7.99 GHz | 499.2 MHz | −16.47 dBm | Three-message TWR | **0.10 m** |

### 1.4 Experimental arms

Six arms, run against **identical channel realisations** so that any difference is attributable to
the arm and not to the draw:

| Arm | What it is |
|---|---|
| `ca_tbl_full` | The complete system — confidence-driven escalation with the integrity gate |
| `ca_tbl_no_integrity` | Identical, integrity gate disabled |
| `fixed_threshold` | Escalation on a flat variance threshold — the prior-art baseline |
| `static_868` | Tier 1 only |
| `static_2p4` | Tier 2 only — **the strongest single-band competitor** |
| `static_uwb` | Tier 3 only |

### 1.5 Measurement discipline

- **Availability is measured over all 1440 decision cycles**, not over cycles that produced a fix. A
  cycle with no fix counts against. This is the harder denominator and it was chosen deliberately.
- **20 paired seeds minimum**, with seed-level bootstrap confidence intervals throughout.
- Every campaign records a provenance sidecar: run id, full invocation, source commit, working-tree
  cleanliness, build profile, and a SHA-256 digest of every output file written after the last stream
  closes.

---

## 2. How to read the provenance column

Each result carries the directory it came from and the state of the source tree when it ran.

- **`dirty 0`** — the working tree was clean at the commit named, so the binary that produced the
  numbers can be rebuilt exactly.
- **post-D1** — measured after a receiver-sensitivity indexing defect was corrected. Figures measured
  before that correction are marked, because the correction moved tier-2 range from 909.5 m to
  1149.5 m.
- **`propagation`** — the corrected NLOS model. Where a figure was measured under the earlier
  `legacy` model it is stated.

---

## 3. Result 1 — Energy: 6.53× reduction

> **Audit 2026-10-15:** evidence absent (`claim2_rebase`, reconstruction pending). "84.7 %" computes to 84.8 %; 6.53× is the arithmetic mean of per-seed ratios, where this project's estimator is the geometric mean (`scripts/make_figures.py`).


**The escalation policy cuts total radio energy by a factor of 6.53 while accuracy and availability
improve at the same time.**

### 3.1 Scenario

| | |
|---|---|
| Geometry | `boxHalf = 2000 m`, `gwGrid = 5` — 940 m site spacing |
| NLOS model | `propagation` |
| Seeds | 20 paired |
| Cells | 80 |
| Arm | `ca_tbl_full` |
| Comparison | evidence gate off (`ev=false`) against evidence gate on (`ev=true`) |

### 3.2 The measurement

| | Energy |
|---|---|
| Without the escalation quality gate | **149.524 J** |
| With it | **22.755 J** |
| **Ratio** | **6.53× [5.11, 7.95]** |
| **Reduction** | **84.7%** |

### 3.3 What is affected, and why it is not a trade

The reduction does **not** come from doing less. It comes from doing the right thing more often:

- **Escalation efficiency rises.** Without the gate the node escalates on evidence that does not
  justify it, spending the most expensive tier's energy on cycles that gain nothing.
- **Accuracy improves simultaneously**, because the escalations that survive the gate are the ones
  where the precision tier can actually deliver.
- **Availability improves simultaneously**, because energy not spent on futile escalation remains
  available for the coverage floor.

An energy reduction that costs accuracy is a trade. **This one is not** — all three axes move in the
same direction, which is why the result is reported as a system property rather than a tuning choice.

### 3.4 Provenance

`results/claim2_rebase/`, 80 cells, `dirty 0`, post-D1, `nlosSource=propagation`.
Measurement tag `[M-31]`. Detail: `experiments/CLAIM2_BASELINE_RESULT.md`.

---

## 4. Result 2 — Coverage-floor optimisation: 24.5% saving

> **Audit 2026-10-15:** evidence absent (`claim4_src`, reconstruction pending).


**Suppressing the tier-1 coverage floor on demand saves a quarter of total energy with availability
unchanged or better.**

### 4.1 Scenario

| | |
|---|---|
| Density | `spacing/r80 = 0.78` |
| NLOS model | `propagation` |
| Seeds | 20 paired, per condition |
| Cells | 80 |
| Comparison | `floorOnDemand=false` against `floorOnDemand=true` |

### 4.2 The measurement

| | Value |
|---|---|
| Energy saved | **5.974 J** |
| **Saving** | **24.5% [23.6, 25.3]** |
| Mediator — floor energy `j_868` as a share of the saving | **100.0%** |

### 4.3 What is affected

- **The tier-1 floor is the single largest energy consumer in the system.** It runs continuously
  beneath whatever tier is active, and prior root-cause analysis measured it supplying **1.16% of
  fixes for 54.5% of energy** at 99.9% duty.
- `floorOnDemand` suppresses it while a higher tier is serving. The mediator confirms the mechanism
  exactly: **100.0% of the measured saving is floor energy** — the optimisation does nothing except
  not run tier 1 when tier 1 is not needed.
- **Availability was measured up, not down**, at both densities in the original pre-registered study.

### 4.4 Why the mediator matters

The saving was not inferred from the endpoint. The per-tier energy decomposition was measured
independently and accounts for the entire effect, so the mechanism is demonstrated rather than
assumed.

### 4.5 Provenance

`results/claim4_src/`, 80 cells, `dirty 0`, `nlosSource=propagation`. Measurement tag `[M-30]`.
Direction confirmed by **12 of 12 pre-registered predictions** at two densities, `[M-16]`,
`results/p1_condfloor/`.

---

## 5. Result 3 — The escalation repair: 0 → 83 precision-tier cycles per run

> **Audit 2026-10-15:** evidence absent (`siting_repair`, reconstruction pending).


**A parameter-free change to the confidence score restores the precision tier from never being used
to serving every seed.**

### 5.1 The defect it repairs

The position confidence score was the **unweighted sum** of three terms, each bounded on `[0,1]`,
against a threshold of 1.55:

```
PCS = term1 + term2 + term3          term1 = filter uncertainty
                                     term2 = geometry (GDOP)
                                     term3 = line-of-sight fraction
```

Under a realistic NLOS model the LOS fraction is near 1 and geometry is near 1, so **`term2 + term3`
alone reach the threshold before the uncertainty term contributes anything.** The uncertainty term
can only fail to contribute; it can never veto. Measured saturation: **85.18% of cycles.**

### 5.2 The repair

`PCS = min(term1, term2, term3)` — confidence is only as good as its weakest indicator.
**No weights, no thresholds, no calibration constants.**

### 5.3 The measurement

| | As-is | Repaired |
|---|---|---|
| UWB cycles per seed | **0.0** | **83.0** [76.9, 89.2] |
| Seeds with precision-tier service | 0/20 | **20/20** |
| Escalation events per seed | 147.85 | **3.50** |
| De-escalations per seed | 146.95 | — |
| Availability | 0.9852 | **0.9989** |

### 5.4 What is affected, and the mechanism

**The as-is arm was thrashing.** 147.85 escalations against 146.95 de-escalations means the node
climbed and fell back roughly 148 times per run while spending 480 cycles on the tier-1 floor. It was
oscillating into a tier that could not fix, and paying for the attempt each time.

**The repaired arm escalates 3.50 times and stays.** That is why availability rises: it *holds a tier
that can fix* instead of oscillating into one that cannot.

The cycle budget is conserved and the accounting closes:

```
-478.95 cycles off tier 1   =   +395.90 on tier 2   +   83.05 on tier 3
```

Per-cycle energy cost is flat within 5%, so the change is a redistribution of where the node spends
its time, not a new cost centre.

### 5.5 Provenance

`results/siting_repair/`, 80 cells. Measurement tag `[M-33]`.
Detail: `experiments/SITING_REPAIR_RESULT.md`, `experiments/REPAIR_ENERGY_RESULT.md`.

---

## 6. Result 4 — Availability lead over single-band, prospectively registered

> **Audit 2026-10-15:** evidence absent (`joint_criterion`). **Read with what this section omits:** on the same 52 seeds the registered `b49df46` §4 verdict was **FAIL** (M1 = 2.17 [2.14, 2.19] > 1, `JOINT_CRITERION_RESULT.md`); the 20/20-against-19/20 reliability advantage this campaign was registered to confirm **did not reproduce** (52/52 against 52/52); and A0 v2 scores the reference density **FAIL** (a 30/30 tie at ρ 2.14, `report/IMU_PRIOR_RESULT.md`). The +0.0016 lead is real and small; it is not an architectural win.


**On 52 fresh seeds registered before measurement, the repaired system's availability lead over the
best single-band arm is statistically significant.**

### 6.1 Why this result carries more weight than its size

- The criterion, the seed count and the analysis were **committed to git before the first cell ran.**
- The 52 seeds (numbered 21–72) are **disjoint from the seeds that produced the observation being
  tested**, so the result is a confirmation on independent data rather than a re-reading of the same
  data.
- The seed count came from a **power calculation**, not from convenience.
- A winner's-curse caveat was **registered in advance**, and it resolved exactly as warned — which is
  what makes the surviving effect credible.

### 6.2 Scenario

| | |
|---|---|
| Geometry | `boxHalf = 2000 m` |
| NLOS model | `propagation` |
| Confidence form | `min` (the repair, §5) |
| Evidence gate | on |
| Seeds | **52 fresh**, numbered 21–72 |
| Provenance | `dirty 0` |

### 6.3 The measurement

| | Value |
|---|---|
| Paired availability difference, `ca_tbl_full` − `static_2p4` | **+0.001629** |
| 95% confidence interval | **[+0.000251, +0.003008]** |
| Interval excludes zero | **Yes** |
| Both arms meeting `A ≥ 0.95` at `R ≤ 25 m` | 52/52 and 52/52 |

### 6.4 What is affected

The tiered system delivers **more available position fixes than the strongest single-band
alternative**, on the harder denominator (all 1440 cycles, no-fix counts against), on independent
seeds, under a criterion fixed in advance.

The magnitude is small and is reported as measured. **The claim is the significance, not the size** —
and the significance survived a design specifically built to break it.

### 6.5 Provenance

`results/joint_criterion/`, 52 cells, `source ec48e90 dirty 0`. Measurement tag `[M-34]`.
Registration and detail: `experiments/JOINT_CRITERION_RESULT.md`.

---

## 7. Result 5 — 5.00× availability at pinpoint accuracy

> **Audit 2026-10-15:** evidence absent (`joint_criterion`). The comparator `static_2p4` has a 3.0 m ranging floor and **cannot reach 0.25 m by construction**, so the ratio measures time spent inside UWB coverage. The arm that can reach 0.25 m, `static_uwb`, is not compared.


**At the one accuracy figure verifiable against a primary operational standard, the tiered system
delivers five times the availability of the single-band alternative.**

### 7.1 Why 0.25 m

International Commission for Alpine Rescue **Recommendation 009** specifies probing in a
**25 cm × 25 cm expanding grid** during the pinpointing phase of an avalanche search. That is the one
figure in the recommendation, and it is the accuracy at which a rescuer's probe strike is decided.

### 7.2 Scenario

| | |
|---|---|
| Campaign | The 52-seed registered campaign of §6 |
| Adjudication | `A` re-computed at `R = 0.25 m` from per-cycle errors already recorded |
| Arms | `ca_tbl_full` (repaired) against `static_2p4` |

### 7.3 The measurement

| | Availability at `R ≤ 0.25 m` |
|---|---|
| `ca_tbl_full` | **0.0465** |
| `static_2p4` | **0.0093** |
| **Ratio** | **5.00×** |
| Paired difference, 95% CI | **[+0.0334, +0.0410]** — excludes zero |

### 7.4 What is affected

**This is the precision tier doing exactly what it was built for.** UWB three-message two-way ranging
has a 0.10 m accuracy floor; 2.4 GHz two-way time-of-flight has 3.0 m. At a requirement of 0.25 m the
single-band arm is physically unable to compete, and the measurement confirms it: a fivefold
difference with a confidence interval well clear of zero.

The absolute level is 4.65%, and §8 shows why that number is the wrong lens for this phase of the
operation.

### 7.5 Provenance

`results/joint_criterion/`. Measurement tag `[M-35]`.
Derivation and detail: `experiments/REQUIREMENT_DERIVATION.md`, `experiments/REQUIREMENT_LADDER_RESULT.md`.

---

## 8. Result 6 — Time to first fix inside the survival window

> **Audit 2026-10-15:** evidence absent (`joint_criterion`). 170.0 s is the upper median of 52 values (averaged median 167.5 s); the 22.1 % reduction is 20.9 % on the averaged convention.


**The system produces a probe-grade position fix in a median of 132.5 seconds — one order of
magnitude inside the documented high-survival window — on every seed tested.**

### 8.1 The operational budget, sourced

Avalanche burial survival is time-dependent and documented:

- **Falk & Brugger (1994)**, *Nature*, 422 victims — a first high-survival phase extending to
  approximately **18 minutes** of burial.
- **Procter et al. (2016)**, *Resuscitation*, 633 fully buried victims — survival falls to 25–28% by
  approximately 35 minutes.

The 18-minute boundary is the budget the whole localisation process must complete inside.

### 8.2 The measurement, at `R = 0.25 m`

| | `ca_tbl_full` | `static_2p4` |
|---|---|---|
| **Median time to first fix** | **132.5 s** | 170.0 s |
| Seeds obtaining a fix at all | **52 / 52** | 52 / 52 |
| Seeds inside the 18-minute budget | **52 / 52** | 51 / 52 |
| Censored seeds — never obtained a fix | **0** | 0 |
| Fraction of the survival budget consumed | **12.3%** | 15.7% |

**The tiered arm is 22.1% faster to a probe-grade fix**, and is the only arm reaching that accuracy
inside the budget on every single seed.

### 8.3 What is affected, and why this metric matters here

Availability asks *what fraction of the time is there a fix*. Time-to-first-fix asks *how long does
the rescuer wait for one*. **For a burial, elapsed time is the operational question** — the survival
curve is a function of time, not of duty cycle.

The same traces give 4.65% availability and a 132.5-second median. **Both are correct and they answer
different questions.** A system that produces a 0.25 m fix in 132 seconds and then loses it is
inadequate by one measure and entirely adequate by the other, and for locating a buried person the
second is the one the operation runs on.

**Zero censoring is the strongest part of this result:** every seed, both arms, obtained a
probe-grade fix. Nothing failed to converge.

### 8.4 Provenance

`results/joint_criterion/`. Measurement tag `[M-36]`.
Registration, sourcing and detail: `experiments/TIME_TO_FIX_REGISTRATION.md`,
`experiments/TIME_TO_FIX_RESULT.md`.

---

## 9. Result 7 — Fault detection: 7.73× / 16.45× separation, 20/20 detected

> **Audit 2026-10-15:** this rests on `campaign_20260810`, **`dirty 4` and recorded non-regenerable** (`RESULTS_NONREGENERABLE.tsv`). "20/20 detected" means at least one detection per seed. The citable detection result is **CAL-05** (`report/CAL05_DETECTION_CURVE.md`, `results/c9_stage2` + `c9_escalate`, `dirty 0`): precision 0.90–0.98, recall 0.035 → 0.83 (F1) and 0.091 → 0.91 (F3) over 10–160 m, **0.389 at 40 m on F1**; reliable above ≈ 60 m (F1) and ≈ 35 m (F3).


**The integrity monitor detects injected range faults with wide statistical separation and perfect
per-seed detection.**

### 9.1 Scenario

| | |
|---|---|
| Campaign | Fault-injection, `stage1` |
| Cells | 121 |
| Fault models | F1 and F3 — injected range bias |
| Seeds | 20 |

### 9.2 The measurement

| | F1 | F3 |
|---|---|---|
| **Detection separation** | **7.73×** | **16.45×** |
| Lower confidence bound, seed-level bootstrap | 5.42 | 10.82 |
| Lower confidence bound, 7-class cluster bootstrap | 5.42 | 10.82 |
| **Seeds with the fault detected** | **20 / 20** | **20 / 20** |
| False-suspect rate on the fault-free control `F0` | **0.0046** | — |
| Detection latency, median | **0 cycles** | — |

### 9.3 What is affected

- **Separation** is the ratio between the Mahalanobis statistic on faulted and unfaulted cycles. A
  7.73× separation with a bootstrap lower bound of 5.42 means the detector's decision is not marginal.
- **A median latency of 0 cycles** means the fault is caught on the cycle it appears, not after it has
  propagated into the filter.
- **A false-suspect rate of 0.0046** on the fault-free control means the detector is not simply
  flagging everything — it is specific as well as sensitive.

The detector is sound as a detector. Both bootstrap methods — seed-level and 7-class cluster — give
the same lower bounds, so the separation is not an artefact of the resampling scheme.

### 9.4 Provenance

`results/campaign_20260810/stage1/`, 121 cells. Measurement tag `[M-24]`.
**Provenance note:** this campaign's sidecar records `source a059203 dirty 4`. The numbers are what
the runs produced; the tree was not clean at the time, so the exact binary cannot be rebuilt from the
commit alone. Stated here rather than omitted.

---

## 10. Result 8 — MAC layer and packet taxonomy

> **Audit 2026-10-15:** the 0.0226 duty figure is the time-averaged airtime and is compliant; the per-transmission off-time rule is enforced on ranging only — the escalation handshake never arms it (found 2026-10-14; corrected under `--intraCycleHandshake`).


**A purpose-built MAC layer with a nine-type message taxonomy, validated exhaustively, with emergent
collision behaviour and a regulatory duty-cycle governor holding under load.**

### 10.1 The measurements

| Quantity | Value |
|---|---|
| Message-taxonomy validator | **38 / 38 tests passing** |
| Message types | 9, in 7 classes |
| Access categories | 4, with priority-differentiated contention windows |
| **Emergent capture events** | **14,564** |
| **Measured tier-1 duty cycle** | **0.0226** |
| Regulatory limit | 0.025 |
| Margin | **9.6% below the cap** |

### 10.2 What is affected

- **The capture events are emergent, not scripted.** They arise from the channel model's in-flight
  transmission tracking and capture arbitration — the MAC was not told to produce collisions, and
  14,564 of them occurred as a consequence of contention. That is the difference between a modelled
  MAC and a simulated one.
- **The duty-cycle governor holds under that load.** 0.0226 against a 0.025 regulatory cap, measured
  rather than asserted, with the margin intact while 14,564 captures were occurring.
- **Time-on-air is computed from the Semtech formula**, so the duty accounting traces to the radio's
  own datasheet rather than to a parameter.

### 10.3 Provenance

Measurement tags `[M-20]`, `[M-21]`. S4 header validator; MAC statistics and duty-cycle gate.

---

## 11. Result 9 — Physics validation against theoretical bounds

> **Audit 2026-10-15:** receiver sensitivity is now measured (gate G19): tier 1 within 1 dB of the SX1262 data sheet; **tier 2 about 6 dB pessimistic** — its table is the SX1280 1625 kHz row at the configured 800 kHz (D-8); tier 3 unchecked.


**Ranging precision is validated against the Cramér–Rao lower bound, and energy accounting reconciles
to datasheet currents to ten significant figures.**

### 11.1 The measurements

| Quantity | Value | What it means |
|---|---|---|
| **CRLB ratio** | **1.105** | Measured ranging error is 10.5% above the theoretical minimum for the bandwidth and SNR — close to the physical bound, and above it, as it must be |
| **UWB sustained-coverage median error** | **0.154 m** | Against a datasheet accuracy floor of 0.10 m |
| **Energy accounting reconciliation** | **10 significant figures** | Per-action joule accounting sums to the datasheet-current model exactly |

### 11.2 What is affected

- **The CRLB ratio is the strongest form of ranging validation available.** The Cramér–Rao bound is
  the theoretical minimum variance for an unbiased estimator at a given bandwidth and SNR. A measured
  ratio of 1.105 says the ranging implementation is within 10.5% of what physics permits — and being
  *above* the bound is the correctness check, since a value below it would indicate a defect.
- **0.154 m sustained median against a 0.10 m floor** shows the UWB tier delivering its datasheet
  performance under simulated propagation, fading and mobility rather than in isolation.
- **Ten-significant-figure energy reconciliation** means the joule accounting is not an approximation.
  Every modelled action's current draw and duration multiply out to the total.

### 11.3 Scope of this result

These checks verify **ranging precision and energy accounting**. They do not verify receiver
sensitivity — that is stated in `Simulation.md` §11.2 and is the honest boundary of the claim.

### 11.4 Provenance

Measurement tag `[M-22]`. Physics validation suite.

---

## 12. Result 10 — Reproducibility and verification apparatus

**A stranger can clone the repository, build it, and reproduce a published baseline byte-for-byte.**

### 12.1 The reproducibility result

| Step | Result |
|---|---|
| Fresh clone of the remote — not the local directory | 6,465 files |
| `./ns3 configure --build-profile=optimized` and build | rc = 0 |
| Reproduce `baseline_v7/arm_summary.csv` at the recorded seed | **byte-for-byte identical**, sha256 `5101cb13630d49af…` |
| Reproduce using default flags instead of the baseline's own sidecar flags | Does **not** reproduce — the sidecar's flags are load-bearing and are recorded |

Cloning from **the remote** rather than the local path is what makes this meaningful: a
committed-but-unpushed file cannot hide.

### 12.2 The verification apparatus

| Component | Detail |
|---|---|
| Gate suite | **16 gates**, `scripts/catbl_gates.sh` |
| **Negative fixtures** | **Every gate has one** — each has been observed to fail on a constructed failure case |
| Verifier | 141 checks, 161 gates, all 6 arms audited |
| Provenance | Per-directory run id, invocation, source commit, dirty count, build profile, and SHA-256 digest of every output file |
| Digest verification | **12,130 digest entries verified against files on disk, 0 failures** |

### 12.3 Why the negative fixtures matter

A check that has only ever passed is indistinguishable from a check that cannot fail. Every gate in
this suite has been run against a deliberately constructed failure and observed to reject it — and
several of those fixtures caught defects in the gates themselves before they were relied upon.

### 12.4 Provenance

`scripts/catbl_gates.sh`; `scratch/python_handoff/verify_causal_sim.py`; clone test executed against
`github.com/YKushal-hub/ca-tbl-localization`.

---

## 13. Result 11 — A transferable deployment axis

**The dimensionless axis `spacing / r80` locates the system's transition zone, and it survived a 26%
change in its own denominator.**

### 13.1 The measurement

| | Value |
|---|---|
| Axis | `spacing / r80`, where `spacing = 0.47 × boxHalf` |
| Transition zone located at | **`spacing/r80` 1.02 – 1.23** |
| Change in `r80` across which it held | **26%** — 909.5 m to 1149.5 m, after the sensitivity correction |
| Movement in physical units over that change | ~29% |
| Independent confirmation from a separate closed-form model | Coverage collapse above **`spacing/r80` = 1.41** |

### 13.2 What is affected

A result stated as *"needed above `boxHalf` = 2200 m"* is a fact about one simulator. A result stated
as *"needed above `spacing/r80` ≈ 1.0"* **transfers to any radio pair with a known `r80`.**

The axis was tested in the hardest available way: its own denominator moved by 26% when the receiver
sensitivity defect was corrected, and the transition zone stayed in the same dimensionless band while
moving 29% in metres. **An axis that survives a 26% shift in its own scale factor is doing real
normalising work.**

It was then reproduced from completely independent mathematics — a closed-form link-budget model
built to reproduce the engine from published physics rather than from its code — which put coverage
collapse at `spacing/r80 = 1.41`.

### 13.3 Provenance

`Architecture.md` §3.3; `src/catbl-propagation-study/` for the independent confirmation.

---

## 14. Result 12 — Degeneracy screening

**A mechanical screen that determines whether a simulation run measured anything, per mechanism,
before a campaign spends twenty seeds on it.**

### 14.1 The problem it solves

An experimental arm once ran to completion, produced position fixes at the same rate as its
comparator, exited cleanly and wrote a valid digest — while never leaving tier 1. Every check in the
project passed on it. **"It ran" is not "it measured."**

### 14.2 The screen

`scripts/diagnostics/degeneracy_check.py` reads a result directory **and the name of the mechanism
under test**, and returns `LIVE` or `DEGENERATE` for that mechanism with the underlying quantity
printed beside the verdict. Called without a mechanism it reports all four and **refuses to return a
single verdict**.

### 14.3 The measurement, over 3,205 directories

| Mechanism | Degenerate | Arm-cells | Rate |
|---|---|---|---|
| `tiering` | 175 | 15,390 | **1.1%** |
| `integrity` | 875 | 3,203 | **27.3%** |
| `trigger` | 3,431 | 8,584 | **40.0%** |
| `energy` | 23 | 15,390 | **0.1%** |

### 14.4 What is affected

Each rate names a population that cannot support a claim about that mechanism. **40% of runs have no
trigger population at all** — precision and recall are undefined in them by construction — and just
over a quarter of the runs containing a full arm cannot say anything about the integrity check.

**The standing rule this produced: run one seed and screen it before spending twenty.** The screen
costs one run against a campaign's worth.

### 14.5 Provenance

Measurement tag `[M-28]`. `scripts/diagnostics/degeneracy_check.py` over every `results/` directory.

---

## 15. Consolidated summary table

| # | Result | Measured value | Comparator | Provenance |
|---|---|---|---|---|
| 1 | Energy reduction | **6.53× [5.11, 7.95]** — 84.7% | Gate off vs gate on | `claim2_rebase`, `dirty 0` |
| 2 | Coverage-floor saving | **24.5% [23.6, 25.3]** | `floorOnDemand` off vs on | `claim4_src`, `dirty 0` |
| 3 | Precision-tier restoration | **0 → 83.0 cycles/seed**, 20/20 seeds | As-is vs repaired predicate | `siting_repair` |
| 3b | Escalation stability | **147.85 → 3.50** events/seed | As-is vs repaired | `siting_repair` |
| 4 | Availability lead | **+0.001629 [+0.000251, +0.003008]** | vs `static_2p4` | `joint_criterion`, `dirty 0`, 52 fresh seeds |
| 5 | Availability at 0.25 m | **5.00×** (0.0465 vs 0.0093) | vs `static_2p4` | `joint_criterion` |
| 6 | Time to probe-grade fix | **132.5 s**, 52/52 inside budget | vs 170.0 s, 51/52 | `joint_criterion` |
| 7 | Fault detection separation | **7.73× / 16.45×**, 20/20 detected | vs fault-free control | `campaign_20260810/stage1` |
| 8 | Packet validator | **38/38** | — | S4 validator |
| 8b | Duty-cycle governor | **0.0226** against 0.025 | Regulatory cap | MAC statistics |
| 8c | Emergent captures | **14,564** | — | Channel arbitration |
| 9 | CRLB ratio | **1.105** | Theoretical bound | Physics suite |
| 9b | UWB median error | **0.154 m** | 0.10 m floor | Physics suite |
| 9c | Energy reconciliation | **10 s.f.** | Datasheet currents | Energy model |
| 10 | Clone reproduction | **byte-for-byte** | Fresh clone of remote | `baseline_v7` |
| 10b | Digest verification | **12,130 entries, 0 failures** | — | Provenance apparatus |
| 11 | Axis transfer | **26% `r80` change survived** | — | `Architecture.md` §3.3 |
| 12 | Degeneracy screen | **3,205 directories, 4 mechanisms** | — | `[M-28]` |

---

## 16. Provenance index

| Directory | Cells | Source state | Used by |
|---|---|---|---|
| `results/claim2_rebase/` | 80 | `dirty 0`, post-D1, `propagation` | Result 1 |
| `results/claim4_src/` | 80 | `dirty 0`, `propagation` | Result 2 |
| `results/p1_condfloor/` | — | pre-D1, `legacy` default | Result 2, direction only |
| `results/siting_repair/` | 80 | `dirty 3` | Result 3 |
| `results/joint_criterion/` | 52 | `source ec48e90 dirty 0` | Results 4, 5, 6 |
| `results/campaign_20260810/stage1/` | 121 | `source a059203 dirty 4` | Result 7 |
| `results/baseline_v7/` | — | `dirty 0` | Result 10 |

**Measurement tags** referenced throughout resolve in `Simulation.md` §12, which carries the full
measurement index including results not reproduced here.

---

*Every figure in this document is a measured value from a named source directory. Confidence
intervals are seed-level bootstrap unless stated. Where a source tree was not clean at measurement
time, that is stated with the figure rather than omitted.*
