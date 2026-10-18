# V4-ACQUIRE results — particle acquisition makes the filters far more consistent; accuracy gains are partial (registered, exploratory)

Registration: `report/PREREG_V4_ACQUIRE.md` at `655fac7f`, committed before any cell. 80 runs, all `rc 0`,
`source 655fac7f dirty 0`, one compile stamp, frozen binary with a SHA-256 manifest. Tracked per cell:
`arm_summary.csv`, `run_id.txt` (`results/v4_2026-10-16/`).

## 1. Verdicts (as registered)
| # | prediction | outcome |
|---|---|---|
| V4-1 | A: `ca_tbl_full` CEP95 ON − OFF interval below 0 | **not held**: −138.5 m [−401.7, +70.7] |
| V4-2 | A: `static_2p4` median error ON < OFF | **not held as computed**: the registered analyser's mean of seed medians is NaN, because seeds with no fix rows after 120 s give an empty median (§2) |
| V4-3 | A: `ca_tbl_full` coverage up and median NEES down | **held**: 0.335 → 0.517, 41.3 → 9.7 |
| V4-4 | A: `static_868` CEP95 ON < OFF | **not held**: +143 m [−159, +534] |
| V4-5 | B: `static_2p4` coverage >= 0.90 | **held**: 0.942 (from 0.824); CEP95 34.0 → 16.5 m |
| V4-6 | B: `ca_tbl_full` CEP95 within +30 % | **not held**: +12.0 m [−2.3, +32.5] on 14.9 m |

## 2. Disclosed post-hoc computations (not verdicts)
**V4-2 recomputed** over seeds with fix rows in both variants (NaN-aware):

| config | arm | median error OFF → ON | paired seeds |
|---|---|---|---|
| A | `static_2p4` | 231 → 75 m (−156 m [−268, −75]) | 15 |
| A | `ca_tbl_full` | 285 → 312 m (+26 [−143, +184]) | 20 |
| B | `static_2p4` | 3.7 → 2.6 m (−1.2 [−1.6, −0.8]) | 20 |

**Availability falls where the particle filter holds on**, because cycles whose estimate is the particle cloud do
not count as fix cycles (`hadFix` marks an EKF range update):

| config | arm | fix-cycle share OFF → ON | first fix OFF → ON |
|---|---|---|---|
| A | `static_2p4` | 0.064 → 0.034 | 204 → 393 s |
| A | `ca_tbl_full` | 0.648 → 0.608 | 62 → 92 s |
| B | `ca_tbl_full` | 0.988 → 0.989 | unchanged |

Part of 2.4-alone's per-fix gain at 5 km is therefore selection: fewer, later fixes. Whether particle-filter cycles
should count as fixes is a definition the owner should set; the cloud is an estimate with an honest covariance.

## 3. Reading
- **Consistency is where V4 clearly works.**

  | config | arm | median NEES | coverage |
  |---|---|---|---|
  | A | `static_2p4` | 4.7 → 0.24 | 0.56 → 0.92 |
  | A | `ca_tbl_full` | 41 → 9.7 | 0.34 → 0.52 |
  | A | `fixed_threshold` | 59 → 9.4 | |
  | A | `static_868` | 84 → 37 | 0.19 → 0.39 |
  | B | `static_2p4` | | 0.94 |

  No arm reaches the S3 band in A.
- **Accuracy:**
  - better for 2.4 alone in both configs;
  - not significantly better for the hybrid arms at 5 km;
  - worse for the hybrid arms' CEP95 in the endgame (14.9 → 26.9 m), while their median error is unchanged
    (3.4 → 3.5 m). The cost sits in the tail, consistent with the cloud's coarser estimate during a longer
    acquisition.
- **The same endgame tail worsening appeared under V3's consider biases** (14.9 → 27.3 m). Two different mechanisms
  that both stop the filter committing early cost the hybrid arms' endgame tail. Committing early to an
  overconfident fix, it seems, was helping the endgame CEP95.
- **Decision:**
  - `--acquireParticles` stays default off.
  - It is recommended for 2.4-only operation and wherever an honest covariance matters (relays, V5's decision rule).
  - For the hybrid arms' endgame tail, a faster handover in small boxes is the next lever. That is a new
    registration, not a re-reading.

## 4. Analysis output (`scripts/v4_analyse.py`)
# V4-ACQUIRE results (exploratory)

| config | arm | variant | median NEES | R95 coverage | mean CEP95 (m) | mean of seed median errors (m) |
|---|---|---|---|---|---|---|
| A | static_868 | OFF | 84.37 | 0.189 | 1777.4 | 992.1 |
| A | static_868 | ON | 36.85 | 0.386 | 1920.8 | nan |
| A | static_2p4 | OFF | 4.70 | 0.560 | 634.5 | nan |
| A | static_2p4 | ON | 0.24 | 0.924 | 525.7 | nan |
| A | ca_tbl_full | OFF | 41.26 | 0.335 | 841.7 | 285.4 |
| A | ca_tbl_full | ON | 9.67 | 0.517 | 703.1 | 311.5 |
| A | fixed_threshold | OFF | 59.38 | 0.287 | 847.9 | 271.9 |
| A | fixed_threshold | ON | 9.39 | 0.517 | 696.5 | 240.8 |
| B | static_868 | OFF | 14.51 | 0.440 | 46.2 | 8.5 |
| B | static_868 | ON | 13.50 | 0.460 | 82.1 | 10.4 |
| B | static_2p4 | OFF | 0.94 | 0.824 | 34.0 | 3.7 |
| B | static_2p4 | ON | 0.62 | 0.942 | 16.5 | 2.6 |
| B | ca_tbl_full | OFF | 3.14 | 0.738 | 14.9 | 3.4 |
| B | ca_tbl_full | ON | 2.92 | 0.761 | 26.9 | 3.5 |
| B | fixed_threshold | OFF | 3.47 | 0.720 | 12.7 | 3.5 |
| B | fixed_threshold | ON | 2.92 | 0.758 | 22.6 | 3.4 |

A ca_tbl_full CEP95 ON-OFF -138.5 [-401.7, +70.7] m
A static_2p4 median error ON-OFF +nan [+nan, +nan] m
A static_868 CEP95 ON-OFF +143.4 [-158.6, +534.0] m
B ca_tbl_full CEP95 ON-OFF +12.0 [-2.3, +32.5] m (OFF 14.9 m)

## Predictions

- **V4-1**: not held
- **V4-2**: not held
- **V4-3**: held
- **V4-4**: not held
- **V4-5**: held
- **V4-6**: not held
