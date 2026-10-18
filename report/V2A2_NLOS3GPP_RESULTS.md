# V2A2-NLOS3GPP results — 3GPP RMa NLOS removes most gateway ranges; CA-TBL holds at 5 km through its finders (registered, exploratory)

Registration: `report/PREREG_V2A2_NLOS3GPP.md` at `8190ef1e`, committed before any cell. 120 runs, all `rc 0`,
`source 8190ef1e dirty 0`, one compile stamp, frozen binary with a SHA-256 manifest, on the adopted preset
(DESIGN_DECISIONS 10). Tracked per cell: `arm_summary.csv`, `run_id.txt` (`results/v2a2_2026-10-17/`).

## 1. Verdicts
| # | prediction | outcome |
|---|---|---|
| N1 | A: NLOS share of gateway ranges up by > 0.2 | **held**: 0.153 → 0.491; their count 8613 → 277 |
| N2 | A: `static_868` CEP95 interval above 0 | **not held**: +17.7 m [−96.4, +119.5] |
| N3 | R8: `ca_tbl_full` CEP95 ON − OFF > 0 | **held**: +94.1 m [−50.7, +268.9] (214 → 308 m) |
| N4 | A: `ca_tbl_full` fix share ON < OFF | **held**: −0.051 [−0.093, −0.015] |
| N5 | B: `static_2p4` CEP95 within ±25 % | **not held**: +2.6 m [+0.4, +5.0] on 7.1 m (+37 %) |

## 2. Reading
- **Gateways at 5 km become nearly useless under 3GPP RMa NLOS:** 97 % fewer gateway ranges. Yet `ca_tbl_full`'s
  CEP95 at 5 km does not worsen (368 → 340 m), because the finders' ranges, with their positions now current (V7),
  carry the fix.
- **The 868 MHz-dependent configurations suffer:**
  - the ring R8: `ca_tbl_full` +94 m, `static_868` 212 → 440 m;
  - the endgame `static_868`: 54 → 107 m.
- **2.4 GHz alone in the endgame loses a little** (7.1 → 9.7 m): its gateway links now see RMa NLOS too.

**Decision on adoption (scope item E3):** `--nlos3gppRMa` is **not** added to the preset.
- RMa's NLOS term is a building model (h = 5 m, W = 20 m). The deployment is forest, so the standard model is the
  wrong environment, and adopting it would trade one assumption for another.
- It stays as the sensitivity case. "If blockage were urban-rural-like": CA-TBL's 5 km result survives; 868 MHz-only
  layouts do not.
- A forest-specific model is scope item C4.

## 3. Analysis output (`scripts/v2a2_analyse.py`)
# V2A2-NLOS3GPP results (exploratory)

| config | arm | variant | CEP95 (m) | fix share | coverage | tag-0 J |
|---|---|---|---|---|---|---|
| A | static_868 | OFF | 489.9 | 0.780 | 0.028 | 6.43 |
| A | static_868 | ON | 507.6 | 0.698 | 0.036 | 6.28 |
| A | static_2p4 | OFF | 397.9 | 0.060 | 0.822 | 14.28 |
| A | static_2p4 | ON | 410.9 | 0.058 | 0.852 | 14.69 |
| A | ca_tbl_full | OFF | 368.2 | 0.843 | 0.358 | 15.42 |
| A | ca_tbl_full | ON | 340.2 | 0.792 | 0.363 | 15.37 |
| A | fixed_threshold | OFF | 391.4 | 0.844 | 0.330 | 15.33 |
| A | fixed_threshold | ON | 307.8 | 0.798 | 0.369 | 15.59 |
| B | static_868 | OFF | 54.4 | 0.963 | 0.201 | 7.91 |
| B | static_868 | ON | 107.1 | 0.962 | 0.097 | 7.68 |
| B | static_2p4 | OFF | 7.1 | 0.836 | 0.798 | 9.66 |
| B | static_2p4 | ON | 9.7 | 0.795 | 0.719 | 9.18 |
| B | ca_tbl_full | OFF | 5.3 | 0.994 | 0.814 | 25.69 |
| B | ca_tbl_full | ON | 6.8 | 0.994 | 0.789 | 26.76 |
| B | fixed_threshold | OFF | 5.9 | 0.995 | 0.791 | 26.10 |
| B | fixed_threshold | ON | 8.9 | 0.993 | 0.781 | 28.15 |
| R8 | static_868 | OFF | 211.9 | 0.922 | 0.198 | 6.76 |
| R8 | static_868 | ON | 440.2 | 0.728 | 0.080 | 6.30 |
| R8 | static_2p4 | OFF | 371.4 | 0.084 | 0.878 | 11.55 |
| R8 | static_2p4 | ON | 417.0 | 0.058 | 0.892 | 13.09 |
| R8 | ca_tbl_full | OFF | 214.5 | 0.953 | 0.474 | 13.97 |
| R8 | ca_tbl_full | ON | 308.5 | 0.831 | 0.379 | 14.40 |
| R8 | fixed_threshold | OFF | 218.6 | 0.951 | 0.471 | 13.89 |
| R8 | fixed_threshold | ON | 273.6 | 0.843 | 0.420 | 15.49 |

NLOS share of tag 0's gateway ranges (ca_tbl_full arm): A OFF 0.153 (n 8613); A ON 0.491 (n 277); B OFF 0.052 (n 40883); B ON 0.191 (n 19360); R8 OFF 0.127 (n 31477); R8 ON 0.288 (n 3732)
A static_868 CEP95 ON-OFF +17.7 [-96.4, +119.5] m
R8 ca_tbl_full CEP95 ON-OFF +94.1 [-50.7, +268.9] m
A ca_tbl_full fix share ON-OFF -0.051 [-0.093, -0.015]
B static_2p4 CEP95 ON-OFF +2.6 [+0.4, +5.0] m (OFF 7.1)

## Predictions

- **N1**: held
- **N2**: not held
- **N3**: held
- **N4**: held
- **N5**: not held

