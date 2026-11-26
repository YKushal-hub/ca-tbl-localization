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
