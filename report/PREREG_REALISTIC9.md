# REALISTIC-9 — pre-registration (committed before the first cell)

**Why.** REALISTIC-8's L1 held as registered but its peak missed carried range bytes; the audit found 218 of 1200 devices above 2.5 % in some hour. DD 56 now counts
those bytes in the ledger (`--dutyCountCarry`, default true) and reports a peak sampled after every exchange, carried byte and MAC frame.

**Binary and design.** As REALISTIC-8 (frozen binary, SHA-256 before every cell, `dirty 0`), same cells and flags, with the corrected ledger. Seeds 1201-1220 (emergency)
and 1221-1230 (quiet shift), never used. 360 runs. `scripts/realistic9_run.sh`, `scripts/realistic9_analyse.py`, committed with this file.

**Seen before registering.** REALISTIC-8 in full (other seeds): H1 and H2 failed, so they may fail again. Development seeds 721-722: largest audited peak 2.488 %.

## Predictions (fixed now)

- **H1-H3.** REALISTIC-6's rules, unchanged.
- **L1.** Every worker device of ours in every cell, the quiet shift included: mean and audited trailing-hour peak at most 2.5 %.
- **C1.** Every main emergency cell: ours, mean crew coverage at least 0.85.
- **T1, Q1 (reported, no prediction).**

**Not covered.** Searchers' devices, gateways, 2.4 GHz and UWB airtime.

**Abort / completeness.** As REALISTIC-8, plus: a missing device line. No cell dropped or chosen. Nothing is adopted as a default by this campaign.
A Result section is appended here after the run, whatever it says.
