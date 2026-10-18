# V3-CONSIDER results — consider biases buy a little consistency and cost accuracy (registered, exploratory)

Registration: `report/PREREG_V3_CONSIDER.md` at `a80b202f`, committed before any cell. 80 runs, all `rc 0`,
`source a80b202f dirty 0`, one compile stamp, frozen binary with a SHA-256 manifest. Tracked per cell:
`arm_summary.csv`, `run_id.txt` (`results/v3_2026-10-16/`).

## 1. Verdicts
| # | prediction | outcome |
|---|---|---|
| V3-1 | B: `ca_tbl_full` coverage ON > OFF, median NEES in [1, 3] | **held** (0.738 → 0.759; 2.47) |
| V3-2 | B: `static_868` coverage up by >= 0.08 | **not held** (0.440 → 0.518, +0.078) |
| V3-3 | B: `ca_tbl_full` CEP95 not worse by more than 10 % | **not held**: +12.4 m [−4.9, +42.1] on 14.9 m |
| V3-4 | A: `static_868` coverage ON > OFF | **not held** (0.189 → 0.142; CEP95 1777 → 2370 m) |
| V3-5 | no arm in the S3 band in B | **held** |
| V3-6 | `static_2p4` identical ON and OFF | **held** (40 of 40 tag-0 traces) |

## 2. Reading
- **The dev runs (6 seeds, 900 s) reversed at 20 seeds and 1800 s.** They showed CEP95 improving
  (11.4 → 8.4 m). The registered campaign shows it worsening for every hybrid arm in B (`ca_tbl_full` 14.9 → 27.3 m,
  `fixed_threshold` 12.7 → 26.1 m) and for `static_868` in A and B. This is why the registration exists.
- **Consistency improves only modestly** (median NEES 3.1 → 2.5 for `ca_tbl_full` in B) and stays far from the
  S3 band.
- **Likely mechanism (not tested here):** with its own 868 MHz ranges discounted as correlated, each filter leans
  on the measurement classes the per-range log shows are overconfident, the relayed ranges (E[z^2] 12–37).
  Correcting one source of overconfidence while another stays shifts the weight toward the worse one.
- **Decision:** `--rangeBiasStates` stays in the engine, default off, a recorded mixed result. It is worth
  revisiting only after relayed-range consistency is fixed (V4's acquisition fix addresses the early-relay part).

## 3. Analysis output (`scripts/v3_analyse.py`)
# V3-CONSIDER results (exploratory)

| config | arm | variant | rows | median NEES | R95 coverage | mean CEP95 (m) |
|---|---|---|---|---|---|---|
| A | static_868 | OFF | 49914 | 84.37 | 0.189 | 1777.4 |
| A | static_868 | ON | 49618 | 66.58 | 0.142 | 2370.0 |
| A | static_2p4 | OFF | 29784 | 4.70 | 0.560 | 634.5 |
| A | static_2p4 | ON | 29784 | 4.70 | 0.560 | 634.5 |
| A | ca_tbl_full | OFF | 55644 | 41.26 | 0.335 | 841.7 |
| A | ca_tbl_full | ON | 54252 | 50.03 | 0.316 | 849.2 |
| A | fixed_threshold | OFF | 54835 | 59.38 | 0.287 | 847.9 |
| A | fixed_threshold | ON | 54335 | 48.65 | 0.305 | 817.7 |
| B | static_868 | OFF | 57036 | 14.51 | 0.440 | 46.2 |
| B | static_868 | ON | 56616 | 9.12 | 0.518 | 65.8 |
| B | static_2p4 | OFF | 62639 | 0.94 | 0.824 | 34.0 |
| B | static_2p4 | ON | 62639 | 0.94 | 0.824 | 34.0 |
| B | ca_tbl_full | OFF | 66599 | 3.14 | 0.738 | 14.9 |
| B | ca_tbl_full | ON | 66718 | 2.47 | 0.759 | 27.3 |
| B | fixed_threshold | OFF | 66350 | 3.47 | 0.720 | 12.7 |
| B | fixed_threshold | ON | 66236 | 2.46 | 0.775 | 26.1 |

B ca_tbl_full CEP95 ON - OFF: +12.38 [-4.85, +42.10] m (OFF mean 14.9 m)

## Predictions

- **V3-1**: held (coverage 0.738 -> 0.759, median NEES 2.47)
- **V3-2**: not held (0.440 -> 0.518)
- **V3-3**: not held
- **V3-4**: not held (0.189 -> 0.142)
- **V3-5**: held
- **V3-6**: held (static_2p4 tag-0 trace identical ON/OFF, 40 pairs)
