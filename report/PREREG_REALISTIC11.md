# REALISTIC-11 — pre-registration (committed before the first cell)

**Why.** The quiet-shift over-confidence (REALISTIC-9 Q1: coverage 0.677 at CLUS3, 0.745 at RAND3) has a cause and a repair, DD 59 (`--ekf5WindowThetaWalk`): the smoother
left out the heading random walk. Searchers' devices were above the 868 MHz cap (DD 60). This repeats REALISTIC-9's cells with both switches on for every design.

**Binary and design.** As REALISTIC-9 (frozen binary, SHA-256 before every cell, `dirty 0`), its cells and flags, plus `--ekf5WindowThetaWalk=true
--sharedDuty868Finders=true`. `--dualBase868OnNeed` (REALISTIC-10) is NOT used. Seeds 1501-1520 (emergency) and 1521-1530 (quiet shift), never used.
360 runs. `scripts/realistic11_run.sh`, `scripts/realistic11_analyse.py`, committed with this file.

**Seen before registering.** REALISTIC-9 in full. Development seeds 731-738 (DD 59, DD 60 in `docs/DESIGN_DECISIONS.md`): quiet coverage with DD 59 0.908-0.966 at CLUS3 and 0.836-0.994 at
RAND3 (4 seeds each; RAND3 734 at 0.836); ROAD3F4 arrival ours minus 2.4 GHz only +499 s [102, 1020] with DD 59, +689 s [77, 1453] with DD 60 (not run together). Ours is
still later there, so H1 and H2 may fail again. The two switches have not been run together before this campaign.

## Predictions (fixed now)

- **H1-H3.** REALISTIC-6's rules, unchanged.
- **L1.** Every worker device of ours, every cell: mean and audited trailing-hour peak at most 2.5 %.
- **L2.** Every searcher's device of ours, every emergency cell: the same.
- **C1.** Every main emergency cell: ours, mean crew coverage at least 0.85.
- **C3.** Every quiet-shift site: ours, mean crew coverage at least 0.85.
- **T1, Q1, within 25 m in the quiet shift (reported, no prediction).**

**Not covered.** Gateways, 2.4 GHz and UWB airtime; collapse; a paired comparison with REALISTIC-9 or -10 (other seeds).

**Abort / completeness.** As REALISTIC-9, plus: the DD 59 and DD 60 lines in every log, and the same number of searcher lines in every run of a cell. No cell dropped or
chosen. DD 59 is adopted as a preset default only if C3 and C1 hold (owner, 2026-11-25). A Result section is appended here after the run, whatever it says.
