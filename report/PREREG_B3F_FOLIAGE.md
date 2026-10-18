# PREREG B3F-FOLIAGE — clamping the foliage depth to the link length (exploratory grade)

**Registered before any cell runs** (rule 13). Written 2026-10-16. Engine: this commit
(`--foliageDepthClamp`, G134) or a later head of this branch whose other new flags are off (NC1-NC9
byte-identical against the pre-change reference), built on `29dad313`. Owner decision D-16 (2026-10-16): fix the
defect recorded in `report/FOLIAGE_DEPTH_DEFECT.md`, as its own campaign, never in the same campaign as
another link-budget change. Outputs go outside the repository (`scripts/b3f_run.sh`), from a frozen
binary copy with a SHA-256 manifest.

## 1. What is disclosed
- K1's probe recorded r80 = 1515.3 m (868 MHz), 156.0 m (2.4 GHz), 0.0 m and r50 5.3 m (UWB) under the
  unified environment; `FOLIAGE_DEPTH_DEFECT.md` predicts a 12.8 dB over-charge on a 10 m UWB link.
- G134's seed-7 probe pair exercises the flag for the gate only; it is not read for these predictions.

## 2. Cells
Probe sweeps (`--probe=true`), seeds 1-5, `--foliageDepthClamp` false and true: 10 runs.

## 3. Metrics and predictions
r80 per tier (first distance at which PDR falls below 0.8, linear interpolation between probe rows),
mean over seeds 1-5, clamped − unclamped.

| # | prediction | confidence |
|---|---|---|
| **F1** | 868 MHz r80 unchanged within ±1 % (its links are longer than the 500 m cap of the draw) | high |
| **F2** | UWB r80 rises from below 10 m to at least 10 m | medium |
| **F3** | 2.4 GHz r80 rises by between 0 % and 25 % | medium |

## 4. Abort conditions
A run without an `rc` or exiting non-zero; a `run_id.txt` without `dirty 0`; a manifest mismatch.
