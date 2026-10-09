# S2-C results — unknown start, subjects' measurement weighting, NLOS, and an acquisition rule

**Three registered campaigns, exploratory grade, one machine (cloud, 4 cores):**
- **S2-C** (`PREREG_S2C`, `8d441fe4`): unknown start, and the weighting train/held-out;
- **S2-C-NLOS** (`PREREG_S2C_NLOS`, `ddc0173b`);
- **S2-C-ACQ** (`PREREG_S2C_ACQ`, `03473f3c`).

Each pre-registration was committed before its first cell. Tables:
`results/s2c_2026-10-09/S2C.md`, `results/s2c_nlos_2026-10-09/S2C_NLOS.md`,
`results/s2c_acq_2026-10-09/S2C_ACQ.md`. Provenance: each directory's `RUN.md`.

**Conventions:**
- Every difference is paired over seeds, as mean [95 % bootstrap].
- "Five-node" means tag 0 plus subjects 2-5.
- w25 is the share of cycles within 25 m, all cycles. w25acq is the same share after the node's
  first fix.
- Known-start controls are ACC-A's and ACC-A-OPPOINT's cells. The registered byte-identity stage
  shows the S2-C binary reproduces them: **18 of 18 seed-arm pairs identical (P6)**.

## 1. Headline

1. **With the start unknown, static 2.4 GHz's 5 km lead reverses, and every arm localises badly.**
   - The 5 km within-25 m shares of 0.64-0.71 were mostly dead reckoning from a known start.
   - Told only the search box, `ca_tbl_full` is within 25 m on **0.229** of five-node cycles,
     `fixed_threshold`@12 on 0.206 and static 2.4 GHz on **0.161**. `ca_tbl_full` − static is
     **+0.068 [0.003, 0.135]** (P1 held).
   - Static 2.4 GHz never fixes on **46 %** of node-runs, against 11 % (P2 held).
   - `ca_tbl_full`'s first fix comes **645 s [509, 786] earlier** (P3 held, in both
     configurations).
   - Median five-node error is about 300 m for every arm. Over the run, `ca_tbl_full` converges
     from 718 m to 66 m and static 2.4 GHz stalls at 154 m (§2.2).
2. **The endgame survives an unknown start.**
   - Every arm converges within about 600 s. w25 is 0.86-0.88 against 0.97 at a known start.
   - `ca_tbl_full` fixes first 165 s [96, 243] sooner than static 2.4 GHz and has 2.3 m
     [0.8, 4.3] lower median error, at 4.5× the energy.
3. **CA-TBL and `fixed_threshold`@12 do not separate under unknown start** on any accuracy or
   availability metric (P5). This repeats ACC-A-OPPOINT.
4. **The subjects' filter is overconfident after fixes, not while coasting.**
   - The inconsistency is concentrated in three-range 2.4 GHz fixes and mixed dual-base fixes.
   - A uniform R multiplier (the registered training selected ×10) is the wrong shape (§3).
5. **NLOS handling, oracle or detector, has no measurable value in this engine.** The engine's
   NLOS removes links rather than biasing ranges (§4).
6. **The acquisition rule does not rescue the unknown-start filter** (§5). The lock-in is not
   only the two-range solves.

## 2. Unknown start (S2-C, U cells, seeds 1-20)

### 2.1 Means per arm and paired differences

| | 5 km (A): `ca_tbl_full` / `fixed_threshold`@12 / static 2.4 GHz | endgame (B): same order |
|---|---|---|
| first fix, five-node mean (s) | 418 / 389 / 1 063 | 65 / 78 / 230 |
| node-runs never fixing | 0.11 / 0.08 / 0.46 | 0 / 0 / 0.02 |
| availability | 0.278 / 0.274 / 0.019 | 0.576 / 0.577 / 0.269 |
| w25 | 0.229 / 0.206 / 0.161 | 0.868 / 0.876 / 0.859 |
| w25acq | 0.274 / 0.241 / 0.308 | 0.893 / 0.910 / 0.913 |
| five-node median error (m) | 299 / 323 / 373 | 6.2 / 6.7 / 8.6 |
| J per node | 18.5 / 18.4 / 14.2 | 25.4 / 27.3 / 5.7 |
| **known start** w25 (control) | 0.638 / 0.649 / 0.705 | 0.977 / 0.973 / 0.969 |

| paired difference | A | B |
|---|---|---|
| `ca_tbl_full` − static 2.4 GHz, w25 | **+0.068 [0.003, 0.135]** | +0.009 [−0.029, 0.047] |
| … first fix (s) | **−645 [−786, −509]** | **−165 [−243, −96]** |
| … median error (m) | −75 [−159, 11] | **−2.3 [−4.3, −0.8]** |
| … J per node | +4.2 [3.1, 5.4] | +19.6 [17.4, 22.0] |
| `ca_tbl_full` − `fixed_threshold`@12, w25 | +0.023 [−0.016, 0.068] | −0.008 [−0.039, 0.020] |
| … J per node | +0.05 [−0.88, 1.01] | **−1.89 [−3.88, −0.03]** |

**P4 failed.** At 5 km `ca_tbl_full`'s w25acq is 0.274 against 0.643 at a known start: a radio
first fix does not localise the node to 25 m. The filter starts from a prior kilometres wide and
often acquires on the wrong root (§5).

### 2.2 Error over time (post-hoc, descriptive; `scripts/s2c_error_over_time.py`)

Five-node median error (m) / within-25 m share:

| 5 km | t = 30 s | 300 s | 900 s | 1 500 s | 1 795 s |
|---|---|---|---|---|---|
| `ca_tbl_full`, unknown | 718 / 0.03 | 340 / 0.10 | 140 / 0.28 | 81 / 0.29 | 66 / 0.35 |
| static 2.4 GHz, unknown | 671 / 0.02 | 343 / 0.03 | 246 / 0.23 | 161 / 0.25 | 154 / 0.15 |
| `ca_tbl_full`, known | 2 / 0.88 | 11 / 0.74 | 15 / 0.66 | 27 / 0.48 | 28 / 0.47 |
| static 2.4 GHz, known | 1 / 0.97 | 6 / 0.93 | 14 / 0.80 | 31 / 0.44 | 38 / 0.32 |

**At a known start,** static 2.4 GHz's lead at 5 km is its first ten minutes of dead reckoning. By
25 minutes it has decayed below `ca_tbl_full`.
## 3. Subjects' measurement weighting (H-W)

### 3.1 Training (seeds 21-30, 5 km, `ca_tbl_full`; registered selection)

| `--measVarScale` | subjects' NIS > 9.21 share | subjects' ANEES | w25 | median error (m) | J/node |
|---|---|---|---|---|---|
| 1 | 0.421 | 12.7 | 0.595 | 26.7 | 19.28 |
| 3 | 0.314 | 9.0 | 0.601 | 23.7 | 19.10 |
| 10 | 0.208 | 6.8 | 0.642 | 21.4 | 18.59 |

- **No scale reached the 0.05 criterion,** so the registered rule chose s\* = 10, the smallest
  share.
- **P7 held:** the share at s = 1 is 0.42, against 0.01 for a consistent filter.
- **P8 held:** the share decreases strictly with s.
- **P11** is evaluated in `S2C.md`.

### 3.2 Where the inconsistency is (post-hoc, training seeds; `scripts/s2c_consistency_diag.py`)

**Subjects' NEES at s = 1** (χ²₂ has mean 2, median 1.39, 5 % above 5.991):
- coasting rows: mean 2.4, median 0.17, 7 % above;
- the coasting row just before a fix: mean 11.3, 33 % above;
- fix rows: mean 15.2, median 4.85, **45 % above**.

The filter is consistent or conservative while it dead-reckons, and overconfident after it fixes.

**The raw fix's NIS by fix type, s = 1:**

| fix tier label | set | anchors | n | NIS > 9.21 | median NIS |
|---|---|---|---|---|---|
| 2.4 GHz | active tier | 3 | 1 113 | **0.53** | 11.4 |
| 2.4 GHz | 868 MHz floor set (dual base: both bands) | 4 | 355 | **0.69** | 23.4 |
| 2.4 GHz | 868 MHz floor set | 5 | 205 | 0.42 | 3.9 |
| 2.4 GHz | active tier | 4 | 249 | 0.13 | 0.35 |
| 2.4 GHz | active tier | 5-6+ | 46 | 0.12-0.15 | 0.4-0.5 |
| 868 MHz | 868 MHz floor set | 3-4 | 362 | 0.08-0.13 | 1.5-2.2 |

**What this says:**
- **The inconsistency is concentrated** in three-range 2.4 GHz fixes and in mixed-band dual-base
  fixes. Redundant 2.4 GHz fixes and 868 MHz fixes are near consistent.
- **A uniform R multiplier is the wrong shape.** ×10 inflates the consistent fixes as much as the
  inconsistent ones and leaves 21 % inconsistent.
- **Anchor-position error is not the cause.** Finder pseudo-anchors already carry their reported
  variance in sigma (N.21), and gateways are surveyed.
- **The mechanism is unidentified.** The candidates are three-range geometry (no redundancy; a
  linearised covariance or a wrong root) and the mixed-band sets. B1's per-range log is the
  instrument.

### 3.3 Held-out (seeds 1-20; s\* = 10 against s = 1; known start; control ACC-A M_k10)

| s\* − s = 1 | 5 km (A) | endgame (B) |
|---|---|---|
| `ca_tbl_full` five-node w25 | +0.042 [−0.000, 0.089] | −0.001 [−0.010, 0.010] |
| `ca_tbl_full` median error (m) | **−5.8 [−12.4, −1.2]** | **+1.59 [1.26, 1.96]** |
| `ca_tbl_full` tag 0 CEP95 (m) | +9.0 [−1.5, 17.4] | **+3.7 [2.0, 5.5]** |
| `ca_tbl_full` availability | +0.008 [−0.004, 0.020] | **+0.014 [0.004, 0.024]** |
| `ca_tbl_full` recovery after a loss (s) | **+70 [26, 123]** | **+22 [7, 42]** |
| `ca_tbl_full` J per node | +0.42 [−0.01, 0.86] | +1.5 [−1.0, 4.1] |
| static 2.4 GHz median error (m) | +0.66 [−0.26, 1.63] | **+0.65 [0.44, 0.86]** |

`ca_tbl_full`'s subjects at s\*: NIS > 9.21 share 0.166 (A) and 0.091 (B); ANEES 4.5 and 4.1.

**Verdicts:**
- **P9 failed.** The 5 km w25 gain's interval touches 0.
- **P10 failed.** The endgame CEP95 worsens by +3.7 m, more than 10 % of 7.5 m.
- **P11 held,** on training seeds.

**Reading:**
- A global ×10 R buys a little at 5 km (median error −5.8 m) and costs accuracy in the endgame.
  There most fixes are redundant and already consistent, and the inflation discards their
  information.
- That is what §3.2's stratification predicts of a correction with the wrong shape.
- **Not adopted.** `--measVarScale` stays a default-off probe.

## 4. NLOS (`S2C_NLOS_AUDIT.md`; ablation registered, `ddc0173b`)

**Ablation** (`ca_tbl_full`, seeds 1-10, paired, mean [95 % bootstrap]):

| | A: none − oracle | A: detector − oracle | B: none − oracle | B: detector − oracle |
|---|---|---|---|---|
| tag 0 CEP95 (m) | −2.37 [−11.29, 6.98] | −1.07 [−12.17, 9.21] | −0.51 [−1.47, 0.46] | −0.38 [−1.64, 1.03] |
| five-node w25 | +0.046 [−0.024, 0.140] | +0.029 [−0.045, 0.124] | −0.005 [−0.015, 0.006] | +0.002 [−0.007, 0.009] |
| five-node median error (m) | −6.60 [−18.26, 0.74] | −5.94 [−17.50, 1.09] | **+0.44 [0.01, 0.93]** | +0.28 [−0.19, 0.72] |
| availability | −0.001 | +0.003 | +0.009 | +0.005 |
| J/node | −0.93 [−2.15, 0.07] | **−0.65 [−1.21, −0.13]** | +1.70 [−1.63, 5.19] | −0.32 |
| subjects' ANEES | −42.6 (n.s.) | +12.8 (n.s.) | **+2.12 [0.23, 5.04]** | +0.78 (n.s.) |

Q1-Q3 held. Q4 failed (tag 0's NIS share does not rise without the inflation).

**Reading:**
- **Removing the truth-keyed inflation** changes no CEP95, within-25 m or availability figure.
  In the endgame it costs 0.44 m of median error and some subject consistency.
- **The detector-keyed inflation** is indistinguishable from the oracle on every metric, and
  from none. With precision 0.02-0.08, it inflates mostly LOS ranges, and with near-unbiased
  simulated NLOS ranges that does no harm either.
- **NLOS information has no measurable value beyond the covariance in this engine,** because the
  engine's NLOS hardly biases a range; it removes links. The honest conclusion is a limitation of
  the simulator's NLOS model, not a detector result.
## 5. The acquisition rule (S2-C-ACQ, `03473f3c`; 5 km, unknown start, seeds 1-10)

**What the rule does.** `--acquireMinAnchors=3` refuses two-range and one-range updates until the
first three-range cycle. Diagnosed motivation: two-thirds of 5 km first fixes were two-range solves
against the box-centre prior, with a median error of 611 m.

**Reuse check:** 4 of 4 seed-arm pairs byte-identical to S2-C's U_A (S5).

| Q3 − U_A (off) | `ca_tbl_full` | static 2.4 GHz |
|---|---|---|
| first fix (s) | +236 [144, 326] | +678 [558, 792] |
| node-runs never fixing | +0.14 [0.06, 0.22] | +0.50 [0.40, 0.60] |
| w25 | −0.015 [−0.101, 0.099] | **−0.129 [−0.180, −0.079]** |
| median error (m) | +92 [−11, 186] | +368 [290, 437] |
| recovery after a loss (s) | **−343 [−871, −33]** | n/a |
| J per node | +1.8 [0.03, 4.1] | +0.1 |

- **S1 and S2 failed.** Waiting for three ranges does not improve `ca_tbl_full`'s localisation. It
  delays acquisition and doubles the never-fix share.
- **For static 2.4 GHz it is destructive.** 94 % of node-runs never fix, because three
  simultaneous 2.4 GHz ranges are rare at 5 km.
- **So the lock-in is not only the two-range solves.** The more likely cause is the EKF itself,
  which updates from a box-centre prior kilometres off (hypothesis, untested). Re-initialising at
  the first multilateration solution is the next candidate (N.54).

**The flag stays default-off and is not recommended for adoption.**

## 6. What this changes, and what it cannot establish

**Changes:**
- **Any 5 km within-25 m comparison in the tree is a known-start number.** At a known start it is
  dominated by the first ten minutes of dead reckoning, which is why static 2.4 GHz led. With the
  start unknown, the hybrid leads, but no arm localises well at 5 km. The 5 km scenario is
  acquisition-limited.
- **"Is there any point to tiers"** gets an evidence-based answer: yes, for acquisition. At 5 km
  under an unknown start the hybrid is the only design that reliably fixes (11 % never against
  46 %), and it fixes 10.7 minutes sooner. CA-TBL's escalation rule still adds nothing over a
  variance threshold.
- **The subjects' overconfidence is real and localised** (§3.2). Fix it where it is (three-range
  and mixed-band fixes), not with a global R.

**Cannot establish:**
- **Exploratory grade.** One machine, 20 seeds (10 for NLOS and ACQ). The campaigns ran
  concurrently with each other and with the gate suite; wall time is not a metric here.
- **The unknown-start prior is ours.** A uniform box with σ = boxHalf/√3 is the least informative
  choice, and not a calibrated SAR prior. A last-known-position prior would sit between it and the
  known start.
- **The 3 m tier-2 floor (N.40) applies throughout.** TOF-FIELD's field error under unknown start
  has not been run.
- **Finder navigation was checked; it is not a truth leak.** The endgame's `--finderMode=1`
  converges finders on the system's position *estimate* of the lost node and stands them off 8 m
  (the flag's help text), so the endgame under unknown start does test acquisition. Finders'
  reported positions enter subjects' ranges as pseudo-anchors with their variance (N.21).
