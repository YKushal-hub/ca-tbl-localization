# REALISTIC-7 — pre-registration (committed before the first cell)

**Why.** On the replays (DD 41 on) the filter's 95 % region covered 45-85 % of the crew's emergency cycles and 18 % of the
office site's quiet shift. Traces (development seeds 701-704 only) showed one cause: a cycle with exactly two ranges fuses the
circle crossing as an isotropic position of variance 2(s0^2 + s1^2) (Case A/B). With the gateways close together the crossing is
good along the range and hundreds of metres loose across it, so the filter took 100-360 m jumps while claiming about 10 m.
DD 54 `--sparseRangeGeometry` (default off; off is byte-identical on 3 development seeds) applies the two ranges as ranges. It is in the
filter every design shares, so every design gets it.

**Binary.** Frozen at launch by `scripts/realistic7_run.sh` (SHA-256 checked before every cell); source = the commit adding this
file; `dirty 0` on every cell.

**Design.** Seeds 621-640 (emergency) and 641-650 (8 h quiet shift), never used. REALISTIC-6's cells and flags with the flag on for
every design; the five arms of the main cells with it off, same seeds; the quiet shift on (ARMS, TII2) and off (ARMS). 500 runs.
Measures and statistics: `scripts/realistic7_analyse.py`, committed with this file.

## Predictions (fixed now)

- **H1-H3.** REALISTIC-6's rules (searcher arrival, time to 25 m, A250 against single-radio designs), every design with the flag on.
- **K1.** CLUS3: our crew coverage rises by more than 0.10 (paired lower bound).
- **K2.** Every other main cell: our coverage does not fall by more than 0.05.
- **K3.** Every main cell: our crew A(25 m) does not fall by more than 0.02.
- **K4.** Every main cell: our energy does not rise by more than 1.0 J per worker-hour.
- **T1 (reported, no prediction).** Ours minus the flat threshold, both with the flag on, on arrival, time to 25 m, A25, A250, J/h and coverage.
- **Q1 (reported).** Quiet shift: ours minus the better single radio on A250; on minus off.

**Abort / completeness.** Any rc not 0, not `dirty 0`, a manifest mismatch, a wrong DD54 line, an alarm not at 3000 s, or fewer than the
registered seeds in a cell: incomplete, nothing is evaluated. No cell dropped or chosen. The flag is not adopted as a default by this
campaign; a verdict either way is reported.

**Reporting.** `REALISTIC7.md` by the analyser; a Result section is appended here after the run, whatever it says.
