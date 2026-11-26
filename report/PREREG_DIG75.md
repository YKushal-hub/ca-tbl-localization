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

## Result (computed 2026-11-27 after registration adb0250dd; 100/100 runs, every trace's hash matches COLLAPSE-13's manifest; problems 0)

Full table: `results/dig75_2026-11-27/DIG75.md`.

- **D1: HOLDS.** With DD 75 on, at teams 2, t_dig 2 h, r 2 m, `spt` - `id`: K5 +0.26, K8 +3.01, H5 +8.59, H8 +4.87, K2 +6.16 survivors of 20.
- **D2: HOLDS.** `spt` >= `id` in 54 of 60 combinations (0.90); largest gain +10.20. The six losses are all K5 (-0.01 to -0.49), where
  "never worse" no longer holds with DD 75.
- **D3: FAILS.** `spt` with DD 75 on minus off: mean -0.380 survivors over the 60 combinations (need >= -0.10); worst -2.92 (K8, 1 team,
  t_dig 0.5 h, r 2 m); best +1.22. By cell: K8 -1.36, K5 -0.76, H5 -0.01, H8 +0.16, K2 +0.07.
- **Reading.** In central rubble the filter's own sigma ranks the workers close to the oracle (K8, teams 1, 0.5 h: off 15.06, oracle 15.56);
  DD 75's wider bound ranks them worse (12.23). DD 75 is the honest published bound but a worse ranking key there. With DD 75 on, the paper
  cannot say "never worse; up to 6.3": it can say spt >= id in 54 of 60, by up to +10.2, losing up to 0.49 at K5.
- **Candidate, not tested here.** Publish DD 75's bound but rank `spt` by the filter's own covariance (system-side, unchanged by DD 75 by
  construction, so the D3 difference would be zero and the off-state ranking returns). COLLAPSE-13's traces lack the covariance columns
  (`--traceCovariance` was off), so this needs new runs and its own registration. It is an owner decision whether to pursue it.
