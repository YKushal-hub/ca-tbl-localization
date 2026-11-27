# REPLAY-DD54-PARETO — pre-registration (committed before the first cell)

**Why.** The paper's decision-rule section rests on PARETO-1/2 replays run before DD 54 (a two-range fix keeps its geometry). REALISTIC-7 showed that
the defect penalised single-radio designs most, so the rule's comparison must be repeated with it corrected.

**Design.** PARETO-1 and -2 exactly as replayed under DD 41 (`scripts/pareto{1,2}_dd41_run.sh`: same seeds 401-420 and 421-440, cells, flags,
12-gateway pinned deployment), with `--sparseRangeGeometry=true` added to every cell (`scripts/pareto{1,2}_dd54_run.sh`, the only change). 960 runs.
Analysers `scripts/pareto1_analyse.py`, `scripts/pareto2_analyse.py` unchanged; PARETO-2's P2 contrast by `scripts/pareto2_replay_contrasts.py`'s method.
Binary frozen at launch; source = the commit adding this file; `dirty 0` on every cell.

**Hypotheses.** PARETO-1's P1-P3 and PARETO-2's P1-P3 re-evaluated by their own rules; each verdict reported beside the DD 41 replay's. None added.

**Abort / completeness.** As PARETO-1/2. A Result section is appended whatever it says.

**Stopped (2026-11-24 ~15:56 IST).** 235 of 960 cells had run. The design under test is not legal (DD 56: every device passes 2.5 %), so these numbers would be
outdated before use. The runs are kept, not analysed, and not resumed; the comparison is to be repeated with DD 54, 56 and 57 on.
