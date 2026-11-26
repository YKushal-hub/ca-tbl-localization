# DIG-77: rank the digging order by the filter's own covariance while publishing DD 75's bound (registered 2026-11-27, before any run)

**Why.** Owner decision 2026-11-27, option (b): DD 75 (`--sigmaShadow`) becomes the collapse preset's published uncertainty (DD 77), and
this study tests ranking the dig order by the filter's own covariance. DIG-75 D3 failed: ranking by DD 75's wider bound lost 0.38 expected
survivors on average (worst -2.92, K8), because in central rubble the filter's own sigma ranks close to the oracle.

**What is known before the run.** DD 75 changes only the published traceSigma: estimate and filter covariance (pxx, pxy, pyy) identical
on and off, and with DD 75 off traceSigma = pxx + pyy to the printed precision (development check, seed 900, 5 workers, 300 s, not
evidence). So ranking by pxx + pyy with DD 75 on reproduces the off-state ranking by construction; this study measures it on fresh seeds
rather than assuming it, and repeats COLLAPSE-13's coverage claim (H63) on them.

**Design.** The DD 77 preset (DD 75 on under `--collapse`), COLLAPSE-13's collapse flags and cells (K5, K8, H5, H8 four stations; K2 two
stations), 20 workers, arm ca_tbl_full, 1800 s, with `--traceCovariance=true --traceSubjectConsistency=true` (output only; checked
byte-identical in the common columns). Seeds 2201-2210, never used. 50 runs. Frozen binary, SHA-256 before every cell, `dirty 0`.
`scripts/dig77_run.sh` and `scripts/dig77_analyse.py`, committed before the first cell. Disk (amended before the first cell): each finished run's files are hashed into its `files.sha256` and the run is pruned to the traces, summaries, logs, `run_id.txt` and `spec_profile.txt`; `rc` is written after the prune. Scoring: `scripts/collapse/score_dig.py` (G191),
decision at the run's end, base 0,0; grid teams 1 / 2 x t_dig 0.5 / 2 / 8 h x radius 1 / 2 m (DIG-75's 60 combinations).
Policies: `id`; `spt` (ranks by the published traceSigma = DD 75's bound); `sptf` (ranks by the filter's pxx + pyy); `oracle` (bound).

## Predictions
- **E1.** `sptf` - `spt`, mean over the 60 combinations of the mean over seeds: at least 0 survivors, with the worst combination at least -0.10.
- **E2.** `sptf` >= `id` (mean over seeds) in at least 80 % of the 60 combinations, and at teams 2, t_dig 2 h, r 2 m in every cell.
- **E3.** (COLLAPSE-13 H63 repeated) every four-station cell's coverage of the published bound (errM <= 1.73 sqrt(traceSigma), second half) at least 0.85.

If E1 and E2 hold, the dig order ranks by the filter's covariance (`sptf`) and the paper's digging table reports it, with DD 75 as the
published bound. If either fails, that is reported and the paper restates the digging claim from DIG-75. E3 failing withdraws the "0.85"
sentence. Nothing else changes a default.

**Abort.** Any rc not 0, not `dirty 0`, a manifest mismatch, a trace without pxx/pyy, fewer than 20 workers in a run: incomplete, nothing evaluated.
