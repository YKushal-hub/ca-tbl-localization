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
