# REALISTIC-13 — pre-registration (committed before the first cell)

**Why.** The quiet-shift position loss at RAND3 (REALISTIC-11 within 25 m 0.592; REALISTIC-12 0.511), which survived every filter repair tried (DD 64, 66, 70). Diagnosis on REALISTIC-12's
archived RAND3 runs: a crew device is ranged by about one anchor per cycle on average and gets a fix in 4-29 % of cycles, against 1.8-3.1 anchors and 40-73 % at CLUS3. One anchor does not fix
a position, so the loss is suspected to be coverage (how many stations a work area hears), not the filter. The remedy tested is a deployment choice, not a code change: five stations from the
same random-site generator instead of three.

**Seen before registering.** Development seeds 781-788 (diag58/Q75, rule written in ~/catbl_work/campaigns/DEV_PLAN_741.md before they ran): within 25 m RAND3 0.581, RAND5 0.883 (7 of 8 seeds up);
coverage 0.965 / 0.917. REALISTIC-11 and -12 in full.

**Design.** The DD 67 preset with REALISTIC-12's quiet-shift flags (2000 particles, the default), arm ca_tbl_full only (the 2.4 GHz-only comparison is not the question here); site RAND with
3 stations (`rand_n3_k{seed mod 10}.csv`) and 5 stations (`rand_n5_k{seed mod 10}.csv`), the layouts already in `scripts/layouts/realistic` (make_realistic_layouts.py), not made for this
campaign; seeds 2001-2020, never used, so each of the ten layouts of each count runs with two seeds. 40 runs. Frozen binary, SHA-256 before every cell, `dirty 0`.
`scripts/realistic13_run.sh`, `scripts/realistic13_analyse.py`, committed with this file.

## Predictions (paired 95 % bootstrap over seeds)

- **T1.** Share of crew cycles within 25 m, RAND5 minus RAND3: point estimate at least 0.20 and lower end above 0.
- **T2.** RAND5 mean crew coverage at least 0.85.

If both hold, the paper may state the deployment rule "five stations rather than three on a random site" as the remedy for the quiet-shift loss, with this campaign as its evidence. If either
fails, the loss stays a limitation without a remedy.

**Not covered.** The emergency cells, CLUS and ROAD sites, any other station count, cost of the extra stations.

**Abort / completeness.** Any rc not 0, not `dirty 0`, a manifest mismatch, a command line that does not match the cell, a missing crew trace: incomplete, nothing evaluated. Nothing is adopted
as a code default by this campaign. A Result section is appended after the run, whatever it says.
