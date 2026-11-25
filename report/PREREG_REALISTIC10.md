# REALISTIC-10 — pre-registration (committed before the first cell)

**Why.** REALISTIC-9's H1 and H2 failed. Development runs on seeds 731–738 (ROAD3F4) point to the 868 MHz ledger as one cause of the late arrival. This repeats
REALISTIC-9 with the existing switch `--dualBase868OnNeed`: a hybrid design does not make the 868 MHz slot of its dual base on a cycle where the active tier
gave at least three ranges. It is one new flag, on the main cells (`ARMS`) of every design, with no new code. The TII-2 cells stay as in REALISTIC-9:
with the switch on, TII-2's own skip considers no slot (600 s check, seed 731), so it would no longer be TII-2.

**Binary and design.** As REALISTIC-9 (frozen binary, SHA-256 before every cell, `dirty 0`), same cells and flags plus that switch. Seeds 1401–1420 (emergency) and
1421–1430 (quiet shift), never used. 360 runs. `scripts/realistic10_run.sh`, `scripts/realistic10_analyse.py`, committed with this file.

**Seen before registering.** REALISTIC-9 in full. Development seeds 731–738, ROAD3F4, ours minus 2.4 GHz only, arrival: without the switch +805 s [184, 1562], with it
+306 s [−3, 833]; with against without, −499 s [−1207, 44], not clear. Also tried there, not selected: `--dutyRangeShare` 0.25 (+227 s [−102, 775], A250 0.404
against 0.521), 0.75 (+1555 s), both together (+203 s), smoother off (coverage 0.732 against 0.818) and DD 58 (a window without its marginal prior; rejected,
quiet-shift A25 0.02–0.24). The switch was chosen after seeing these, for its arrival, coverage and A250 together, so this campaign is the only evidence for it.
No variant was ahead of 2.4 GHz only, so H1 and H2 may fail again.

## Predictions (fixed now)

- **H1–H3.** REALISTIC-6's rules, unchanged.
- **L1.** Every worker device of ours in every cell, the quiet shift included: mean and audited trailing-hour peak at most 2.5 %.
- **C1.** Every main emergency cell: ours, mean crew coverage at least 0.85.
- **T1, Q1 (reported, no prediction).** The quiet-shift over-confidence is not repaired by this switch and is not predicted to change.

**Not covered.** Searchers' devices, gateways, 2.4 GHz and UWB airtime; any comparison with REALISTIC-9's seeds (other seeds, so not paired).

**Abort / completeness.** As REALISTIC-9, plus: the switch's summary line must be present in every analysed `ARMS` log (the analyser checks). No cell dropped or chosen. Nothing is adopted as a
default by this campaign. A Result section is appended here after the run, whatever it says.

## Result (run 2026-11-25 12:38-16:26 IST; source f9e23eadd; 360/360 cells rc 0, dirty 0, problems 0)

Full tables: `results/realistic10_2026-11-25/REALISTIC10.md`. "Ours" = `ca_tbl_full`. Intervals are paired 95 % bootstrap over seeds.

- **H1: FAILS, at ROAD3F4 only.** Ours minus 2.4 GHz only, searcher within 10 m: ROAD1 -689 s [-1200, -126], ROAD3 -584 [-1256, -1], CLUS3 +77 [40, 115], RAND3 -1266 [-1746, -798],
  ROAD3F4 -26 [-454, 384]. The clearly-better rule (upper bound below -300 s) is met at RAND3. Non-inferiority (upper bound below +120 s) fails at ROAD3F4, where no difference is
  detected either way; at CLUS3 ours is later by 40-115 s.
- **H2: FAILS** at ROAD3 (t25 -358 s [-988, 155]) and ROAD3F4 (-124 [-754, 506]): both means are earlier, the intervals are too wide to show non-inferiority by 120 s. Holds elsewhere.
- **H3: HOLDS.** A250 +0.194 to +0.417, every lower bound at least 0.119.
- **L1: HOLDS.** 0 of 1400 devices above 2.5 %, mean or audited peak; largest 2.488 %. Searchers' devices are not covered, and later counting (DD 60) found them above the cap.
- **C1: HOLDS.** Coverage 0.857 (RAND3) to 0.933 (ROAD3).
- **T1 (reported):** against the flat threshold, arrival and t25 differ in no cell; A250 +0.007 [0.002, 0.014] at CLUS3 and -0.005 [-0.012, -0.001] at RAND3. No demonstrated benefit.
- **Q1 (reported):** quiet-shift coverage ROAD1 1.000, ROAD3 0.878, RAND3 0.795, **CLUS3 0.711**: over-confidence remains, as stated. A250 over the better single radio -0.001 (one station) to +0.405.
- **H4 (reported):** arrival CREW10 -616 [-1146, -160], CREW15 -268 [-643, 15], ROAD3MUL -318 [-896, 294], CLUS3MUL -345 [-634, -84].
- **Against REALISTIC-9:** ROAD3F4 there was +832 [403, 1321]. Other seeds, so the runs do not separate the switch from the seeds; no claim is made that the switch caused the change.
