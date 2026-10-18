# V7-REPLYPOS results — current finder positions in the ranging reply: the largest accuracy gain of the programme at 5 km (registered, exploratory)

Registration: `report/PREREG_V7_REPLYPOS.md` at `cf157648`, committed before any cell. 80 runs, all `rc 0`,
`source cf157648 dirty 0`, one compile stamp, frozen binary with a SHA-256 manifest. Tracked per cell:
`arm_summary.csv`, `run_id.txt` (`results/v7_2026-10-16/`).

## 1. Verdicts
| # | prediction | outcome |
|---|---|---|
| V7-1 | A: `ca_tbl_full` CEP95 ON − OFF interval below 0 | **held**: −570.5 m [−928.0, −274.3] (842 → 271 m) |
| V7-2 | A: `static_868` CEP95 interval below 0 | **held**: −1308.8 m [−1731.8, −901.9] (1777 → 469 m) |
| V7-3 | A: `static_2p4` CEP95 paired mean < 0 | **held**: −186.4 m [−366.7, −22.0] |
| V7-4 | B: `static_2p4` CEP95 paired mean < 0 | **held**: −5.5 m [−19.4, +11.8] |
| V7-5 | B: `ca_tbl_full` CEP95 within ±25 % | **not held**: +6.5 m [−7.8, +31.8] on 14.9 m |
| V7-6 | B: `ca_tbl_full` energy >= 1.10 × | **not held**: × 1.079 (the cost is smaller than predicted) |

## 2. Reading
- **The dominant source of 5 km error was not the radio but the staleness of where finders are.**
  - A precise range to a finder anchored at a minutes-old entry is worth little.
  - Anchored at the finder's current GNSS position it is worth a great deal.
  - Availability rises too: fix-cycle share `ca_tbl_full` 0.648 → 0.861, `static_868` 0.329 → 0.776. Finder ranges
    that were inflated into uselessness now complete fixes.
- **Energy is flat at 5 km** (`ca_tbl_full` 16.0 → 15.5 J) and +7.9 % in the endgame.
- **Consistency:** coverage rises for `ca_tbl_full` (A 0.335 → 0.413; B 0.738 → 0.828) and median NEES falls (A 41 → 23).
- **The hybrid arms' endgame CEP95 tail worsens again** (`ca_tbl_full` +6.5 m, `fixed_threshold` 12.7 → 27.6 m) while
  2.4 alone improves. V3 (14.9 → 27.3 m) and V4 (14.9 → 26.9 m) moved the same baseline the same way. Three
  unrelated interventions that improve the filter's information all worsen this one tail statistic. That makes it a
  property of the hybrid endgame dynamics, not of any one change. It is V8's first ablation target.
- **Decision:** `--rangeReplyPosition` is the first V-flag recommended for every subsequent campaign, alongside
  `--rngKeyed --rangeErrorNoLinkLeak`.

## 3. Analysis output (`scripts/v7_analyse.py`)
# V7-REPLYPOS results (exploratory)

| config | arm | variant | median NEES | coverage | CEP95 (m) | tag-0 J | fix share |
|---|---|---|---|---|---|---|---|
| A | static_868 | OFF | 84.37 | 0.189 | 1777.4 | 6.79 | 0.329 |
| A | static_868 | ON | 106.27 | 0.169 | 468.6 | 6.38 | 0.776 |
| A | static_2p4 | OFF | 4.70 | 0.560 | 634.5 | 14.25 | 0.064 |
| A | static_2p4 | ON | 0.98 | 0.635 | 448.1 | 14.46 | 0.089 |
| A | ca_tbl_full | OFF | 41.26 | 0.335 | 841.7 | 16.01 | 0.648 |
| A | ca_tbl_full | ON | 23.02 | 0.413 | 271.1 | 15.46 | 0.861 |
| A | fixed_threshold | OFF | 59.38 | 0.287 | 847.9 | 16.34 | 0.618 |
| A | fixed_threshold | ON | 33.17 | 0.389 | 334.8 | 15.91 | 0.853 |
| B | static_868 | OFF | 14.51 | 0.440 | 46.2 | 7.81 | 0.916 |
| B | static_868 | ON | 22.29 | 0.377 | 51.3 | 7.90 | 0.966 |
| B | static_2p4 | OFF | 0.94 | 0.824 | 34.0 | 9.60 | 0.815 |
| B | static_2p4 | ON | 1.06 | 0.823 | 28.5 | 9.80 | 0.850 |
| B | ca_tbl_full | OFF | 3.14 | 0.738 | 14.9 | 24.00 | 0.988 |
| B | ca_tbl_full | ON | 1.93 | 0.828 | 21.4 | 25.15 | 0.994 |
| B | fixed_threshold | OFF | 3.47 | 0.720 | 12.7 | 26.47 | 0.990 |
| B | fixed_threshold | ON | 2.11 | 0.810 | 27.6 | 24.16 | 0.994 |

A ca_tbl_full CEP95 ON-OFF -570.5 [-928.0, -274.3] m

A static_868 CEP95 ON-OFF -1308.8 [-1731.8, -901.9] m

A static_2p4 CEP95 ON-OFF -186.4 [-366.7, -22.0] m

B static_2p4 CEP95 ON-OFF -5.5 [-19.4, +11.8] m

B ca_tbl_full CEP95 ON-OFF +6.5 [-7.8, +31.8] m

B ca_tbl_full tag-0 energy ON/OFF 1.079

## Predictions

- **V7-1**: held
- **V7-2**: held
- **V7-3**: held
- **V7-4**: held
- **V7-5**: not held
- **V7-6**: not held

