# Campaign results — 10-hour unattended run, 2026-08-11

**PROVISIONAL.** Base `a059203` + uncommitted source; every directory carries
`ALLOW_DIRTY`; nothing committed, nothing pushed. Snapshot
`~/catbl-snapshots/20260811-101028-campaign`.

Every block's prediction was written into its own output directory before its first
run. 300 result directories produced; **completeness audit: 300 complete, 0
incomplete, 0 non-zero exit codes.** No directory excluded.

---

## Blocks completed

| block | status | outcome |
|---|---|---|
| 0 setup + gate suite | done | **ALL GATES PASS** (G1–G5) |
| 1 P2a — PCS v2 | done | **PARTIAL** |
| 2 F14 — tier-1 sigma | done | **BENIGN** — no separate defect |
| 3 S7 injector | done | built, null byte-identical, forced-failure verified |
| 4 P1 full envelope | done | falsifier CLEAR; 2 of 3 predictions MISSED |
| 6 S7-lite | done ×2 | first run **VOID** (my error); corrected run **confounded** |
| 5 P3 — complementary siting | **not reached** | budget |
| 7 P4 bracket + K=3 | **not reached** | budget |

---

## Block 1 / P2a — PCS v2. **PARTIAL**

| sp/r80 | rho(v2) | rho(legacy) | rho(control) | separation |
|---|---|---|---|---|
| 0.78 | −0.299 | −0.174 | −0.301 | −0.002 |
| 1.29 | −0.613 | −0.153 | −0.639 | −0.027 |
| 2.58 | −0.799 | **+0.114** | −0.878 | −0.079 |

v2 repairs the sign inversion — legacy was *positively* correlated with error at
2.58 — and reaches −0.799. **But it never beats the monotone-of-`traceSigma`
control at any density**, and that is exactly what `fixed_threshold` already
thresholds.

Per-term: `term_post` carries only **12.8–23.1%** of variance, so v2 is *not* a
one-term score wearing decoration — the geometry term dominates. Yet the sum still
fails to beat a statistic built from `term_post`'s input alone. **The geometry term
adds variance without adding predictive power.**

Predictions: rho negative everywhere HIT; |rho| ≥ 0.4 everywhere MISS (0.78 gives
−0.299); separation from control MISS at all three; cep50FixedM not degraded HIT.

Confound stated: `--nlosSource=propagation` and `--pcsForm=v2` moved together
deliberately (v2's `sigma_geo` needs a correctly-keyed R). The control comparison is
computed on the same runs and is immune to it, which is why it carries the verdict.

---

## Block 2 / F14 — tier-1 sigma. **BENIGN**

Three tests, two of which overturned the framing I proposed:

- **Skew hypothesis FALSIFIED.** mean/median = **1.01** — the sigma distribution is
  tight and symmetric, not right-skewed. "Distant links inflate the mean" is dead.
- **Filter is internally consistent.** `errM/sqrt(traceSigma)` median **0.612**,
  mildly conservative, not 35× wrong. The apparent paradox was comparing a
  per-measurement RANGE sigma against a POSITION error.
- **Input sigma is calibrated.** Probe measures tier-1 range error at **47–51% of
  distance**; `meanSigmaRawM` = 1019.5 m matches. `meanSigmaM` = 2734 m is **2.68×**
  larger, and that entire overstatement is the already-known legacy NLOS defect.

**No separate sigma-model defect exists.** F14 and P2 are one question, not two.

---

## Block 4 / P1 full envelope — falsifier CLEAR, 2 of 3 predictions MISSED

| sp/r80 | avail leg → P1 | totalJ leg → P1 | spec leg → P1 | floor duty |
|---|---|---|---|---|
| 0.78 | 0.9993 → 0.9997 | 28.15 → 16.09 | 20 → 20 | 1.2% |
| 1.03 | 0.9917 → 0.9893 | 24.26 → 12.87 | 19 → **18** | 6.1% |
| 1.29 | 0.9351 → 0.9470 | 21.47 → 12.54 | 12 → 12 | 21.8% |
| 1.55 | 0.7666 → 0.7759 | 20.46 → 14.32 | 1 → 1 | 36.7% |
| 2.07 | 0.5419 → **0.5313** | 23.92 → 19.24 | 0 → 0 | 43.3% |
| 2.58 | 0.4126 → 0.4231 | 31.99 → 27.91 | 0 → 0 | 45.6% |
| 3.10 | 0.2564 → 0.2682 | 38.86 → 36.29 | 0 → 0 | 49.7% |

**Energy falls at every density.** Floor duty rises 1.2% → 49.7% with sparsity, as
the mechanism requires: the floor does more real work as tier 2 thins out.

**Falsifier (spec drop ≥ 2 anywhere): CLEAR.**

**Prediction 1 (energy falls everywhere): HIT.**
**Prediction 2 (availability falls nowhere by >0.005): MISS** — 2.07 fell 0.0106.
**Prediction 3 (spec unchanged or better everywhere): MISS** — 1.03 went 19 → 18.

Both misses are small and in the same direction: I was optimistic. Recorded as
misses.

---

## Block 6 / S7-lite — TWO RUNS, NEITHER ANSWERS THE QUESTION

### First run: VOID — my design error

Run with the **default `--nlosSource=legacy`**, where ~75% of links are falsely
NLOS-tagged and `suspect` requires all-LOS. Measured gate eligibility: **0.595% of
fix cycles.** The fault bit hard (availability 0.9903 → 0.8062 at 40 m) and the gate
showed nothing (p = 0.375–1.000 across six cells) — but that null is worthless
because the gate was structurally disabled by a known defect. **Reported as void, not
as a finding.** My error in block design, caught in analysis.

### Corrected run: `--nlosSource=propagation`, all-LOS satisfiable (77.6–80.8%)

| bias | suspects/run | avail full | avail no_integrity | cep full | cep no_integrity |
|---|---|---|---|---|---|
| 0 m | 159.4 | 0.9629 | **0.1236** | 3.25 | 78.59 |
| 40 m | 354.2 | 0.7619 | **0.1018** | 4.57 | 81.56 |

Paired sign test, 20/20 seeds, p < 0.0001 both cells. An enormous apparent benefit.

**It is not integrity checking, and the diagnostic is decisive:**

    bias  0 m (NO FAULT): benefit = +0.839 availability
    bias 40 m (FAULT)   : benefit = +0.660 availability

**The benefit is LARGER with no fault at all, and shrinks when the fault is
introduced.** A fault-catching mechanism cannot behave that way.

Mechanism: `suspect` sets `wantEscalate = true` unconditionally. Under corrected
NLOS, legacy PCS is pinned high and stops escalating (Block 1 / root-cause Q2), so
`ca_tbl_no_integrity` latches on tier 1 at 0.12 availability. `ca_tbl_full` is
rescued because `suspect` fires 159–354 times per run and forces the escalation PCS
failed to make.

**The measured effect is the escalation side-effect, not integrity checking** — the
exact confound `--suspectForcesEscalation` was built to isolate, and which has still
never been run.

**The integrity gate remains UNEXERCISED.** Two attempts, two different reasons.

---

## Claim ledger

| claim | status | evidence |
|---|---|---|
| **Claim 3** — handoff triggers correctly | **PROVEN** | precision 0.078 → 0.934, F1 0.803 with `--escalationRequiresEvidence` |
| **Claim 1** — architectural advantage | **SCOPED** | window confirmed 1.0 ≤ spacing/r80 ≤ 1.3; lower edge measured, upper edge is a coverage limit (96–99.6% of bad fixes have `vis2p4 < 3`) |
| **Claim 2** — decision layer beats flat threshold | **NOT SUPPORTED** | v2 repairs the sign inversion but never separates from the `traceSigma` control the baseline already thresholds |
| **Integrity gate** — central novelty | **UNEXERCISED** | 2 attempts; run 1 void (gate disabled), run 2 confounded (escalation side-effect) |
| **Physics layer** | **PROVEN** | probe gates, CRLB, capture; tier-1 range error 47–51% of distance matches the sigma model |
| **Handshake / packets** | **PROVEN as substrate** | MAC executes; duty-cycle governor now enforced at every call site |
| **Tier-1 floor** | **CHARACTERISED** | 54.5% of arm energy for 1.16% of fixes at 0.78; floor-ranging-on-demand cuts energy at every density with falsifier clear |

---

## The one run that matters most next

`--suspectForcesEscalation=false`, `--nlosSource=propagation`, bias ∈ {0, 40} m,
boxHalf 2000, 20 seeds, `{ca_tbl_full, ca_tbl_no_integrity}`. ~10 minutes.

With the escalation side-effect removed, any remaining `ca_tbl_full` advantage is
attributable to rejection — i.e. to integrity checking — and the benefit should then
be **larger with the fault than without it**. That sign reversal is the test the
gate has never had.

Everything needed for it exists and is verified: the injector (null byte-identical,
forced-failure confirmed), the flag, and the analysis.

---

## Not reached

**P3 complementary siting** — needs new deployment code; not safe to write and run
unattended in the remaining window. Still justified by Block 2's BENIGN verdict and
by the Q3 measurement that 96–99.6% of bad fixes occur with `vis2p4 < 3`.

**P4 `searchDutyFrac` bracket and K=3 sensitivity** — both cheap, neither
claim-decisive.
