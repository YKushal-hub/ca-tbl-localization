# ACC-A-OPPOINT — results: one lowered variance threshold matches or beats `ca_tbl_full`; at T = 25 m² `fixed_threshold` matches it on availability and accuracy for 4.2 J less per node

**Registered in `report/PREREG_ACC_A_OPPOINT.md` (`5c28eb75`) before any cell ran.** Grade:
**exploratory**. All 100 runs exit 0, on ACC-A's own binary.

The registered analysis was applied with one disclosed correction (`f42e9ec4`). The provenance
check had expected the wrong `source` string, and the tables are byte-identical before and after.
Provenance: `results/acc_a_oppoint_2026-10-09/RUN.md`. Tables: `OPPOINT.md` there.

Every comparison is **`fixed_threshold`@T − `ca_tbl_full`** (ACC-A M_k10), paired over seeds 1-20,
as mean [95 % bootstrap]. k = 10 throughout.

## 1. Endgame (B)

| T (m²) | availability | J per node | tag 0 CEP50 / CEP95 | within 25 m | UWB-solved fixes |
|---|---|---|---|---|---|
| 8 | **+0.011 [0.002, 0.021]** | **+4.03 [0.84, 7.24]** | −0.06 / −0.33 | −0.002 | +0.95 |
| 12 | **+0.011 [0.003, 0.020]** | +2.13 [−0.42, 4.58] | +0.03 / −0.23 | −0.004 | −0.45 |
| 25 | −0.002 [−0.011, 0.008] | **−4.19 [−6.33, −1.68]** | +0.19 [−0.03, 0.42] / +0.52 [−0.92, 2.04] | +0.002 | **−2.10** |
| 100 (control) | **−0.031 [−0.042, −0.021]** | **−6.11 [−8.23, −4.02]** | +0.15 / +0.06 | −0.002 | **−2.95** |

All CEP and within-25 m intervals contain 0.

- **R1 held.** The control is byte-identical to ACC-A's `fixed_threshold` on 20 of 20 seeds. So
  T = 100 reproduces ACC-A §4 exactly, and pairing with ACC-A's `ca_tbl_full` is valid.
- **R5 held.** Lowering one threshold moves `fixed_threshold` monotonically: availability and
  energy rise together.
- **R6, the registered falsifier, fired, but not in the direction it was written to detect.**
  - No T matches `ca_tbl_full` on availability *and* energy at once.
  - **At T = 25**, `fixed_threshold` matches `ca_tbl_full`'s availability, CEP and within 25 m for
    **4.19 J less per node**.
  - **At T = 12** it has **more** availability (+0.011) for an energy difference that is not
    significant.
  - So the per-tier thresholds do not do something a single threshold cannot. In the endgame
    `ca_tbl_full` sits **inside** the single-threshold curve: it pays more energy for the same
    availability.
  - Diagnosis reading 4 ("only an operating point") is wrong as stated. The point is a worse one.
- **R2 failed**, by the same result: availability +0.011 at T = 12.

## 2. 5 km (A), T = 12

- Availability +0.007 [0.000, 0.013] (lower bound 0.0004, so R7 failed).
- CEP50 −1.07 [−3.46, 1.09].
- CEP95 **−9.13 [−22.70, −0.04]**.
- J −0.43 [−1.01, 0.14].
- Within 25 m +0.011 [−0.012, 0.035].

`fixed_threshold` at T = 12 is at least as good as `ca_tbl_full` at 5 km on every metric, with
CEP95 marginally better. That holds for this point; no other T was run at 5 km.

## 3. Registered predictions

| # | prediction | verdict |
|---|---|---|
| R1 | control byte-identical, 20/20 | **held** |
| R2 | B T12 availability contains 0 | **failed** (+0.011, `fixed_threshold` higher) |
| R3 | B T12 J contains 0 | **held** |
| R4 | B T12 CEP50 and CEP95 contain 0 | **held** |
| R5 | B availability and J non-increasing in T | **held** |
| R6 | some T in {8, 12, 25} matches availability and J | **failed** (T = 25 matches availability at −4.19 J) |
| R7 | A T12 availability, CEP50, J contain 0 | **failed** (availability +0.007 [0.0004, 0.013]) |
| R8 | B T12 UWB cycles contain 0 | **held** |

## 4. What this changes

- **ACC-A §4's verdict.** "The CA-TBL rule buys endgame availability with energy" was measured
  against `fixed_threshold` at its default 100 m². Against `fixed_threshold` swept over its own
  threshold, the CA-TBL rule buys nothing that a single variance threshold does not buy more
  cheaply.
- **Why** (`ACC_A_DECISION_DIAGNOSIS.md`):
  - in this preset PCS is a function of trace(P);
  - the URGENT-query hook makes most endgame escalations;
  - the dual base removes most of the 868 ↔ 2.4 GHz difference.

  What is left is a threshold. Per-tier tuning did not land on the efficient side.

## 5. What this cannot establish

- **One machine, one k (10), three thresholds in B and one in A.** No tuning seeds.
- **This is not a Pareto-front domination claim.** `ca_tbl_full` was not swept, only
  `fixed_threshold`. A `ca_tbl_full` with other PCS thresholds may reach the same curve.
  This result says its registered operating point is not on it.
- **The CEP and within-25 m comparisons sit at the 3 m tier-2 floor** (N.40). Under field ToF error
  (TOF-FIELD), the two rules have not been compared at matched thresholds.
