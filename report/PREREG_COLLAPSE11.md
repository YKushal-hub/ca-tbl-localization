# COLLAPSE-11 — pre-registration (committed before the first cell)

**Why.** The collapse over-confidence. Logged ranges (development seeds 731-732): 2.4 GHz scatter is 1.06-1.14 times the claimed sigma, but links read long by a constant (+0.7 m to stations, +1.4 m to
searchers), as rubble delay would; the badly covered devices at 5 storeys use the most 2.4 GHz station ranges. The correction for that exists: DD 49 (`--tofRubbleCorrect`, COLLAPSE-5 H17 held).

**Design.** COLLAPSE-9's cells, paired: `off` = DD 54, 56, 57, 60, 62 and the DD 63 world; `on` = the same plus `--tofRubbleCorrect` (DD 49), no new code. Seeds 1701-1710, never used; 200 runs;
frozen binary, SHA-256 before every cell, `dirty 0`. `scripts/collapse11_run.sh`, `scripts/collapse11_analyse.py`, committed with this file.

**Seen before registering.** COLLAPSE-5 H17, COLLAPSE-9 in full, part of COLLAPSE-10's runs finished but NOT analysed. Development seeds 731-733, crew coverage in the second half of the run: without DD 49 and DD 60
0.787 (5 storeys), 0.868 (8), 0.405 (two stations); with DD 49 0.907, 0.864, 0.519; with DD 49 and DD 60 0.941, 0.896, 0.647. DD 60 without DD 49 was not run, so part of the last step may be DD 60's. DD 49 on
top of the rejected DD 61 helped little. The correction's priors span the published range that the World's bracket is drawn from; the cells use the central bracket only, so a pass says nothing about the
low and high brackets.

## Hypotheses (fixed now; paired 95 % bootstrap over seeds; coverage here is over the whole run, as COLLAPSE-9's)

- **H44:** every crew cell: the lower end of ours' L2, on minus off, is at least -0.05.
- **H45:** off and on: every worker's and every searcher's device's audited 868 MHz airtime is at most 2.5 %.
- **H46:** on, four stations, 20 workers: ours minus the best single radio, lower end at least -0.05, at both storey counts.
- **H47:** coverage on minus off: lower end at least -0.02 in every crew cell, and above 0 at 5 storeys with four and with two stations.
- **H48:** on, four stations, 20 workers: mean coverage at least 0.85 at both storey counts.

**Abort / completeness.** As COLLAPSE-9, with this design's announcement lines. No cell dropped. Nothing is adopted as a default by this campaign. A Result section is appended after the run.

## Result (run 2026-11-25 17:00-21:22 IST; source 2047079e4; 200/200 cells rc 0, dirty 0, problems 2)

**By the registered abort clause this campaign is INCOMPLETE and its verdicts are not evidence.** In `s8_n1_on` seeds 1702 and 1703 the analyser's check asked for at least one corrected range; `--tofRubbleCorrect` was
on (it is on every `on` command line in `run_id.txt`, and on no `off` one), but these single-worker runs had no 2.4 GHz or UWB range to correct (the line reads 0). The check was wrong, not the cells. The clause is not
rewritten after the fact: COLLAPSE-12 (`report/PREREG_COLLAPSE12.md`) repeats the design on new seeds with the check reading the command line. Full tables: `results/collapse11_2026-11-25/COLLAPSE11.md`.

What the analyser printed, for information only, on minus off (`--tofRubbleCorrect`): H44 (L2 non-inferior) and H45 (no device above 2.5 %) as predicted; H46 as COLLAPSE-9 and -10 (ours minus 868 MHz alone, 5 storeys -0.030
[-0.075, 0.015], 8 storeys -0.035 [-0.075, 0.010]); coverage, four stations, 20 workers, 5 storeys 0.796 to 0.859 (+0.063 [0.044, 0.080]), 8 storeys 0.825 to 0.853 (+0.028 [0.002, 0.052]); two stations at 5 storeys 0.518
to 0.655 (+0.138 [0.086, 0.186]). None of the hypotheses uses the single-worker cells.
