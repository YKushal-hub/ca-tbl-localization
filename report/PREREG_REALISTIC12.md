# REALISTIC-12 — pre-registration (committed before the first cell)

**Why.** The quiet-shift position loss (REALISTIC-11: within 25 m 0.592 at RAND3, 0.441 at ROAD3). A cause test on development seeds (RAND3 732-734, a throwaway binary multiplying the particle count by ten): within 25 m
0.390 to 0.489, 0.200 to 0.727, 0.351 to 0.357, re-acquisitions 63 to 14 (seed 733). The particle cloud running out is part of the cause, not all of it. V4's 2000 particles is OURS, not sourced, so this is a
sensitivity sweep of an unsourced value, reported both ways, not a tuned value: DD 70 exposes the count as `--acquireParticleCount` (default 2000, off state byte-identical, 56 files).

**Design.** The DD 67 preset with REALISTIC-11's quiet-shift flags, arms ca_tbl_full and static_2p4; sites RAND3, CLUS3, ROAD3 (ROAD1 left out: one gateway, A25 0 in every earlier campaign); particle count 2000 and 20000;
seeds 1901-1910, never used. 60 runs. Frozen binary, SHA-256 before every cell, `dirty 0`. `scripts/realistic12_run.sh`, `scripts/realistic12_analyse.py`, committed with this file.

**Seen before registering.** REALISTIC-11 in full; the cause test above; seeds 751-754 at RAND3 gave 0.893 within 25 m with 2000 particles, so the loss varies a lot with the seed.

## Predictions (paired 95 % bootstrap over seeds)

- **S1.** RAND3, ours, share within 25 m, 20000 minus 2000: lower end above 0.
- **S2.** Every site, 20000: ours' mean crew coverage at least 0.85.
- **S3.** Every site: ours' share within 25 m, 20000 minus 2000, lower end at least -0.02.

**Not covered.** The emergency cells and collapse (a higher count there is not tested); run time per filter (about ten times the particle work).

**Abort / completeness.** Any rc not 0, not `dirty 0`, a manifest mismatch, a command line or DD70 line that does not match the cell, a missing crew trace: incomplete, nothing evaluated. Nothing is adopted as a default by
this campaign. A Result section is appended after the run, whatever it says.

## Result (run 2026-11-26 14:20-15:01 IST; source 407d6f368; 60/60 cells rc 0, dirty 0, problems 0)

Full table: `results/realistic12_2026-11-26/REALISTIC12.md`. "Ours" = `ca_tbl_full`.

- **S1: FAILS.** RAND3, within 25 m, 20000 minus 2000 particles: +0.071 [-0.070, 0.228] (0.511 to 0.583). The cause test's gain is not confirmed on new seeds.
- **S2: HOLDS.** Coverage with 20000: RAND3 0.920, CLUS3 0.909, ROAD3 0.975.
- **S3: FAILS** at CLUS3 (-0.006 [-0.037, 0.034]); ROAD3 -0.002 [-0.069, 0.064]. No detectable change at either.
- **Reported.** 2.4 GHz only, within 25 m: RAND3 0.020 / 0.077, CLUS3 0.695 / 0.708, ROAD3 0.098 / 0.098. Ours stays ahead of it at every site with either count.
- The particle count is not the main cause of the quiet-shift position loss. It stays a limitation. Nothing is adopted.
