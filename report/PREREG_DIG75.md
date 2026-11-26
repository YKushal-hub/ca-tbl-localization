# DIG-75: the digging order with DD 75's uncertainty (registered 2026-11-27, before any digging score was computed)

**Why.** The owner approved DD 75 (`--sigmaShadow`) as the collapse preset's uncertainty (2026-11-27, relayed by the paper session) on the
condition that the digging-order result (COLLAPSE-1 H5, COLLAPSE-2 H8; paper Table V "never worse; up to 6.3 survivors of 20") is
re-run with DD 75 on: `spt` ranks by the published uncertainty, and COLLAPSE-1 used the old sigma (binary c93ee19f2, before DD 67 and DD 75).

**Data, and what was seen.** COLLAPSE-13's 100 runs (seeds 2101-2110; cells K5 K8 H5 H8 K2; DD 75 off and on; DD 67 preset; 20 workers;
1800 s; source 256dd2fe1; raw traces hashed in `results/collapse13_2026-11-26/RAW_MANIFEST.sha256`). Their coverage, share within 2 m,
median error and median bound are known (COLLAPSE-13 Result). No digging score has been computed on them. No fresh runs: C: has
~3.6 GB free while the gate suite runs; a fresh-seed repeat is possible later and would be a separate registration.

**Method.** `scripts/collapse/score_dig.py` as gated by G191 (`load`, `policies`, `schedule`; decision at the run's end; base 0,0),
called by `scripts/dig75_analyse.py`, committed with this file. Expected survivors out of 20, mean over the 10 seeds, paired by seed.
Grid: teams 1 / 2 (INSARAG-MED / INSARAG-HEAVY) x t_dig 0.5 / 2 / 8 h (DEBRUY-TIMES span) x radius 1 / 2 m = 12 combinations per cell, 60 over the five cells.

## Predictions
- **D1 (COLLAPSE-1 H5 with DD 75 on).** At teams 2, t_dig 2 h, r 2 m: `spt` - `id` >= 0 (mean over seeds) in every cell.
- **D2 (COLLAPSE-2 H8 with DD 75 on).** `spt` >= `id` in at least 80 % of the 60 combinations.
- **D3 (DD 75 does not spoil the ordering).** `spt` with DD 75 on minus `spt` with it off, mean over the 60 combinations, at least -0.10
  survivors; the worst single combination is reported.

The estimate is identical with DD 75 on and off (COLLAPSE-13 H62), so `id` and the oracle are identical too; only `spt`'s order changes.
If D1-D3 hold, the paper's digging claim stands with DD 75 and is re-stated with these numbers. If any fails, it is reported as failed
and the paper's claim is restated or withdrawn; nothing is rescued by a new cell or setting.

**Abort.** A missing run, rc not 0, or a manifest hash mismatch on any trace read: incomplete, nothing evaluated.
