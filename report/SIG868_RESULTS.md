# SIG868 results: honest 868 MHz RSSI range σ on the final preset (exploratory)

Registration: `report/PREREG_SIG868.md` (`bc38069a`, before any cell). 120 cells, all `rc 0` and
`source bc38069a dirty 0`. Launched 2026-10-17T03:07Z, done 03:18Z, from `~/catbl_c4` (binary copied from
`~/catbl_v3/build`, source-identical). Tracked: `results/sig868_2026-10-17/`. Analyser: `scripts/sig868_analyse.py`.

**Disclosed registration error:** S2 needs pooled coverage, which reads `--traceCovariance` traces. The runner did not
pass that flag, so S2 is **not evaluable**. It is not counted as failed or held. The adoption rule does not use S2.

## 1. Predictions
| # | prediction | outcome | figure |
|---|---|---|---|
| S1 | W: `ca_tbl_full` CEP95 interval below 0 | **held** | −18.6 [−25.8, −11.6] m (53.4 → 34.7) |
| S2 | A: `static_868` coverage higher | **not evaluable** | no covariance traces |
| S3 | B: `ca_tbl_full` ΔCEP95 ≤ +15 % | **held** | +0.4 [−0.2, +1.1] m on 4.6 |
| S4 | A: `ca_tbl_full` ΔCEP95 ≤ +15 % | **held** | +19.8 [−105.3, +116.9] m on 324 (+6 %) |

**Adoption rule** (S1, S3 and S4 all hold): **ADOPT**. The preset now sets `--rssiSigmaLogNormal=true`
(DESIGN_DECISIONS 14).

## 2. Reading
- **O.7 was B2 on 868 MHz.** Giving the filter the exact log-normal SD of the 868 MHz RSSI range (1.01 d instead of
  0.70 d at 7 dB) cuts the hybrid's calibrated-Wolf endgame CEP95 by a third, 53.4 → 34.7 m.
  - It is still above 2.4 GHz alone (28.3 m).
  - The energy (about 90 J) does not move: the UWB escalation is driven by Wolf's large 2.4 GHz σ, not by 868 MHz.
- **868 MHz alone improves most:**
  - endgame 52.4 → 25.1 m;
  - 5 km 494 → 452 m (after acquisition 465 → 418 m).

  An honest σ stops the filter from over-trusting the short-biased ranges.
- **The engine-ToF hybrid is unaffected in the endgame** (4.6 → 5.0 m, interval spans 0). At 5 km CEP95 is
  +20 m (spans 0), while post-acquisition CEP95 improves, 257 → 213 m.
- `static_2p4` is byte-identical across variants, as it must be (no 868 MHz range enters it).

## 3. What remains of O.7
**Open.**
- Under calibrated Wolf errors the hybrid is still 6 m behind 2.4 alone, at 10× its energy.
- Diagnostic runs (PREREG disclosure) put the rest on the dual-base 868 MHz slot (`--loraDualBase=false`: 33 m,
  51 J).
- The UWB share stays at about 23 %.

## 4. Table
| config | variant | arm | CEP95 (m) | CEP95 after 300 s (m) | coverage | fix share | tag-0 J |
|---|---|---|---|---|---|---|---|
| A | BASE | static_868 | 493.5 | 464.7 | nan | 0.794 | 8.38 |
| A | BASE | static_2p4 | 396.8 | 96.6 | nan | 0.057 | 13.36 |
| A | BASE | ca_tbl_full | 324.3 | 256.6 | nan | 0.853 | 16.62 |
| A | BASE | fixed_threshold | 339.2 | 227.2 | nan | 0.859 | 17.73 |
| A | SIG | static_868 | 451.9 | 417.6 | nan | 0.791 | 8.18 |
| A | SIG | static_2p4 | 396.8 | 96.6 | nan | 0.057 | 13.36 |
| A | SIG | ca_tbl_full | 344.1 | 213.3 | nan | 0.857 | 18.20 |
| A | SIG | fixed_threshold | 359.8 | 232.9 | nan | 0.852 | 18.23 |
| B | BASE | static_868 | 52.4 | 42.1 | nan | 0.966 | 9.52 |
| B | BASE | static_2p4 | 8.7 | 7.7 | nan | 0.838 | 8.96 |
| B | BASE | ca_tbl_full | 4.6 | 4.3 | nan | 0.994 | 25.98 |
| B | BASE | fixed_threshold | 5.7 | 5.4 | nan | 0.994 | 28.07 |
| B | SIG | static_868 | 25.1 | 21.9 | nan | 0.963 | 9.33 |
| B | SIG | static_2p4 | 8.7 | 7.7 | nan | 0.838 | 8.96 |
| B | SIG | ca_tbl_full | 5.0 | 4.4 | nan | 0.995 | 27.56 |
| B | SIG | fixed_threshold | 4.7 | 4.2 | nan | 0.995 | 27.01 |
| W | BASE | static_868 | 52.4 | 42.1 | nan | 0.966 | 9.52 |
| W | BASE | static_2p4 | 28.3 | 24.9 | nan | 0.781 | 8.80 |
| W | BASE | ca_tbl_full | 53.4 | 52.6 | nan | 0.996 | 89.43 |
| W | BASE | fixed_threshold | 51.6 | 50.7 | nan | 0.996 | 91.54 |
| W | SIG | static_868 | 25.1 | 21.9 | nan | 0.963 | 9.33 |
| W | SIG | static_2p4 | 28.3 | 24.9 | nan | 0.781 | 8.80 |
| W | SIG | ca_tbl_full | 34.7 | 33.5 | nan | 0.996 | 91.76 |
| W | SIG | fixed_threshold | 34.2 | 32.6 | nan | 0.995 | 94.19 |

S1 W ca_tbl_full dCEP95 -18.6 [-25.8, -11.6] m on 53.4
S2 A static_868 coverage nan -> nan
S3 B ca_tbl_full dCEP95 +0.4 [-0.2, +1.1] m on 4.6
S4 A ca_tbl_full dCEP95 +19.8 [-105.3, +116.9] m on 324.3

## Predictions

- **S1**: held
- **S2**: not evaluable (the cells carry no --traceCovariance trace)
- **S3**: held
- **S4**: held

Adoption rule (S1, S3, S4 all hold): ADOPT
