# COLLAPSE-13 — pre-registration (committed before the first cell)

**Why.** Collapse over-confidence with four stations (COLLAPSE-12 coverage 0.874 / 0.858 with DD 49; DD 61, 68, 69, 73, 74 rejected). The cause found on development seeds (DESIGN_DECISIONS 73, 74): constant
offsets on precise workers, because a still worker's repeated 868 MHz ranges and TDoA fixes are fused as independent. Weighting them as correlated inside the filter (DD 74) raised coverage but cost accuracy.
DD 75 (`--sigmaShadow`) leaves the filter and its estimate alone and publishes the larger of the filter's traceSigma and a shadow information sum that weights those repeated errors with B2's AR(1) factor
(`--rssiCorrInflate`'s own d_corr and cap: no new constant). Gate G190 (with selftest) checks the estimate is unchanged and the published sigma never lower.

**Seen before registering.** Development seeds 801-803 (rule written before they ran, ~/catbl_work/campaigns/DEV_PLAN_741.md): coverage off -> on K5 .760 -> .849, K8 .849 -> .978, H5 .591 -> .977,
H8 .638 -> .987, K2 .513 -> .692; share within 2 m identical; median bound K5 1.44 -> 5.19 m, K8 1.66 -> 8.57 m (median error 0.8 m), so the honest bound is several times wider.

**Design.** The DD 67 preset with COLLAPSE-12's collapse flags, 20 workers, arm ca_tbl_full, 1800 s, second half of the run. Cells: K5, K8 (central rubble, 5 and 8 storeys, four stations), H5, H8 (high rubble),
K2 (central, 5 storeys, two stations, reported). DD 75 off and on. Seeds 2101-2110, never used. 100 runs. Frozen binary, SHA-256 before every cell, `dirty 0`. `scripts/collapse13_run.sh`,
`scripts/collapse13_analyse.py`, committed with this file.

## Predictions (paired 95 % bootstrap over seeds; four-station cells F = K5, K8, H5, H8)

- **H60.** Mean over F of the coverage gain (on minus off): point at least 0.03 and lower end above 0.
- **H61.** No F cell's coverage falls by more than 0.02.
- **H62.** No F cell's share within 2 m changes by more than 0.03 (the estimate is unchanged by design).
- **H63.** Every F cell's coverage with DD 75 at least 0.85.

The median published bound is reported beside coverage in every cell: a wider honest bound is the price, and the paper states it. If H60-H62 hold, DD 75 may be proposed to the owner as the collapse
preset's uncertainty; H63 decides whether the paper may say the four-station bound reaches 0.85. The open site is not covered: DD 75's effect there is untested, so no open-site default changes.

**Not covered.** The open site and quiet shift; digging cost of the wider bound (B6 search area); other station counts.

**Abort / completeness.** Any rc not 0, not `dirty 0`, a manifest mismatch, a DD75 line that does not match the cell, fewer than 20 crew traces: incomplete, nothing evaluated. Nothing is adopted as a default by
this campaign. A Result section is appended after the run, whatever it says.

## Result (run 2026-11-26 13:32-18:03 UTC, paused 71/100 for disk space, resumed with the same frozen binary after the manifest check; source 256dd2fe1; 100/100 cells rc 0, dirty 0, problems 0)

Full table: `results/collapse13_2026-11-26/COLLAPSE13.md` (raw traces outside the repository, hashed in `RAW_MANIFEST.sha256`).

- **H60: HOLDS.** Mean four-station coverage gain +0.207 [0.179, 0.235]. Per cell: K5 0.817 -> 0.904, K8 0.790 -> 0.942, H5 0.629 -> 0.989, H8 0.753 -> 0.982.
- **H61: HOLDS.** No four-station cell falls; the smallest gain is K5 +0.087 [0.059, 0.115].
- **H62: HOLDS.** Share within 2 m identical in every cell (K5 0.900, K8 0.737, H5 0.444, H8 0.283), as designed: the estimate is unchanged.
- **H63: HOLDS.** Every four-station cell reaches at least 0.85 with DD 75 (lowest K5 0.904).
- K2 (two stations, reported): 0.525 -> 0.697, below 0.85; two stations stay a stated limitation.
- **Price.** The median published bound grows: K5 1.45 -> 4.72 m, K8 1.75 -> 9.03 m, H5 3.09 -> 21.43 m, K2 5.98 -> 8.04 m (median errors 0.72, 1.01, 3.52, 5.51 m). At H8 the median
  bound is already 133 m without DD 75 (median error 14.0 m); the gain there comes from the upper tail.
- DD 75 may now be proposed to the owner as the collapse preset's uncertainty; no default changes without that decision. The paper may say the four-station bound reaches 0.85 in the collapse cells.
