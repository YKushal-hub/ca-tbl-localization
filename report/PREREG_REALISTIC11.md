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

## Result (run 2026-11-25 20:00-23:49 IST, paused for disk space 23:15-23:26; source d9a75c3f5; 360/360 cells rc 0, dirty 0, problems 0)

Full tables: `results/realistic11_2026-11-25/REALISTIC11.md`. "Ours" = `ca_tbl_full`. Intervals are paired 95 % bootstrap over seeds. The pause froze running processes (SIGSTOP, SIGCONT); no cell was restarted.

- **H1: FAILS.** Ours minus 2.4 GHz only, searcher within 10 m: ROAD1 -438 s [-809, -131], ROAD3 -518 [-1110, 49], CLUS3 +121 [56, 201], RAND3 -1134 [-1623, -672], ROAD3F4 +542 [189, 973]. Non-inferiority (upper
  bound below +120 s) fails at CLUS3 and ROAD3F4, where ours is later and the interval excludes zero. The clearly-better rule is met at RAND3.
- **H2: FAILS** (ROAD3 t25 -481 s [-1470, 521], ROAD3F4 -250 [-930, 295]; intervals too wide). Holds elsewhere.
- **H3: HOLDS.** A250 +0.165 to +0.369, every lower bound at least 0.121.
- **L1: HOLDS.** 0 of 1400 workers' devices above 2.5 %; largest 2.489 %.
- **L2: HOLDS.** 0 of 400 searchers' devices above 2.5 %; largest 2.488 %.
- **C1: HOLDS.** Emergency coverage 0.884 (ROAD3F4) to 0.938 (ROAD1).
- **C3: HOLDS.** Quiet-shift coverage ROAD1 1.000, ROAD3 0.958, **CLUS3 0.941, RAND3 0.912** (REALISTIC-9, without DD 59: 0.677 and 0.745; REALISTIC-10: 0.711 and 0.795; other seeds, so not paired).
- **Quiet shift, within 25 m (reported), ours / 2.4 GHz only:** ROAD3 0.441 / 0.098, CLUS3 0.760 / 0.624, RAND3 0.592 / 0.169, ROAD1 0.000 / 0.000. Position loss at RAND3 and ROAD3 remains.
- **T1 (reported):** against the flat threshold, arrival differs at ROAD3 only (-279 s [-598, -32]), one interval of many and not seen in REALISTIC-9 or -10; no benefit is claimed.
- **Arrival at ROAD3F4 across campaigns (different seeds and flags):** +832 [403, 1321] (REALISTIC-9), -26 [-454, 384] (-10, with the 868 MHz skip), +542 [189, 973] (-11, without it).
