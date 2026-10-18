# ACC-B2 — results: leaving the subjects' 868 MHz ranges out of their fused set (F2) does not help; H2 is falsified in this form, and in the endgame it costs availability and accuracy

**Registered in `report/PREREG_ACC_B2_PRECISE.md` (`66338f26`) before any cell ran.** Grade:
**exploratory**. All 80 cells exit 0 and read one source with `dirty 0`. The registered analysis,
applied unchanged, reports "Problems: 0". F2 fired in every on cell. Tables:
`results/acc_b2_2026-10-09/ACC_B2.md`. Paired over seeds 1-20, as mean [95 % bootstrap].

The off cells are byte-identical to ACC-B's off cells: `arm_summary.csv` matches in 40 of 40 seed
cells, on another binary of the same branch. That is consistent with both flags' null controls.

| `--subjectPreciseOnly`, on − off | 5 km (A) | endgame (B) |
|---|---|---|
| subjects' median error (mean over subjects 2-5) | +0.74 [−4.90, 5.59] | **+0.74 [0.28, 1.25]** (worse) |
| five-node within 25 m | −0.011 [−0.046, 0.024] | +0.005 [−0.012, 0.023] |
| five-node availability | −0.003 [−0.014, 0.008] | **−0.042 [−0.051, −0.034]** |
| tag 0 CEP95 | +1.24 [−6.32, 7.94] | −0.18 [−1.04, 0.55] |
| J per node | +1.25 [−0.07, 2.97] | **+2.30 [0.62, 4.00]** |
| within 25 m against static 2.4 GHz | −0.082 [−0.136, −0.025] (off: −0.071) | +0.006 [−0.011, 0.020] |

**Reading:**
- The dual base's 868 MHz ranges help the subjects; they do not pull them off.
- Leaving them out never improves a subject. In the endgame it removes fixes (−0.042) and
  adds error.
- **The 5 km within-25 m loss to static 2.4 GHz has a cause other than H1 (F1, ACC-B) and H2
  (F2, here). It is still open.**
- The energy rise in B is not explained here. Fewer fixes probably mean more escalations, but
  that is not measured.

| # | prediction | verdict |
|---|---|---|
| Q1 | A: subjects' median error lower | **failed** (+0.74, n.s.) |
| Q2 | A: within 25 m higher | **failed** (n.s.) |
| Q3 | A: no significant loss to static 2.4 GHz | **failed** (−0.082) |
| Q4 | A: availability lower | **failed** (n.s.; it is B that loses) |
| Q5 | tag 0 CEP95 unchanged, A and B | **held** |
| Q6 | static 2.4 GHz byte-identical on and off | **held** |

**What is left of Part B's question.** At 5 km the hybrid subjects fix 10-40× more often than
static 2.4 GHz's and are less often within 25 m. Neither the relay supply (F1) nor the tier-1
information (F2) explains it. Candidates for a next registered step, none tested:
- static 2.4 GHz's subjects coast on dead reckoning from a few good fixes, while the hybrid's
  are pulled by frequent tier-1/TDoA fixes. That is H2 in its TDoA form, which F2 does not
  touch.
- a geometry or selection difference in *where* the hybrid fixes. A per-subject range log (B1)
  is the instrument for both.
