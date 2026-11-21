# COLLAPSE-3 — pre-registration (committed before COLLAPSE-1's energy is looked at)

**Question.** Does a worker's device last the 72 h rescue window on one CR123A cell, including its own
computation (Phase 0, DD 43)?

**Data.** COLLAPSE-1's runs (report/PREREG_COLLAPSE1.md): each worker's `totalJ` over 1800 s.
**Battery.** One CR123A, 16,740 J (row BATT-CR123A; nominal, not derated).
**Compute.** Phase 0 measured 1.26 mJ per 2.5 s step for ca_tbl_full and 1.20 mJ for static_868 at k_dp = 1, i.e.
1.82 / 1.72 J/h. A static_2p4 step is taken as ca_tbl_full's (the same filter); stated.
**Extrapolation.** J/h = totalJ · 2 + compute; hours = 16,740 / (J/h). This assumes the first half hour's rate
holds. The first half hour is the busiest (search, escalations), so this is conservative.

**Hypothesis H7.** In every COLLAPSE-1 cell, the 95th-percentile worker (over seeds and workers) lasts ≥ 72 h for
ca_tbl_full, static_868 and static_2p4.

`scripts/collapse3_analyse.py` reads the campaign directory and prints hours per cell and arm (median, 5th
percentile) and H7.

**Disclosure.** Before registration, the analyser was dry-run on one exploratory 300 s run (seed 5; 2 storeys,
central, 20 workers; not a COLLAPSE-1 cell). Its numbers were seen: for ca_tbl_full, median 138 h and 5th
percentile 33 h, a short run dominated by the first minutes' search. H7 was not changed after seeing them. No
COLLAPSE-1 cell's energy had been looked at.
