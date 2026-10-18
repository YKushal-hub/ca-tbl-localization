# C4-P833 results: ITU-R P.833-10 forest foliage on the final preset (exploratory)

Registration: `report/PREREG_C4_P833.md` (`c02c050c`, before any cell). 80 cells, all `rc 0` and
`source c02c050c dirty 0`, launched 2026-10-17T02:57Z, done 03:04Z. Tracked: `results/c4_p833_2026-10-17/`.
Analyser: `scripts/c4_analyse.py`.

**Disclosure:**
- The campaign ran from worktree `~/catbl_c4`.
- Its binary and libraries were copied from `~/catbl_v3/build`, built at `06ea4386`. The source is identical to
  `c02c050c`, which adds only the prereg, runner and analyser.
- **Cross-check:** every BASE arm row (160 of 160) is byte-identical to the matching CAMP-FINAL2 cell (A_main, B_end),
  which ran the same binary from the other worktree.

## 1. Predictions
| # | prediction | outcome | figure |
|---|---|---|---|
| F1 | B: `static_2p4` CEP95 rises | **held** | +2.3 [−1.1, +6.2] m (8.7 → 11.0) |
| F2 | B: `static_2p4` fix share falls | **held** | −0.197 [−0.217, −0.178] (0.838 → 0.641) |
| F3 | A: `static_868` CEP95 not higher | **not held** | +43.3 [−1.7, +98.2] m (494 → 537) |
| F4 | `ca_tbl_full` \|ΔCEP95\| < 15 % in A and B | **not held** | A +89.5 [−28.9, +228.2] m on 324 (+28 %); B +2.2 [+1.4, +3.1] m on 4.6 (+48 %) |

## 2. Reading
- **What P.833-10's frequency dependence does:**
  - About 4.5 dB more forest loss at 2.4 GHz: the 2.4 GHz fix share falls from 0.84 to 0.64 in the endgame and from
    0.057 to 0.022 at 5 km.
  - About 1.5 dB less at 868 MHz.
- **The hybrid gets worse in both configs.** In the endgame the interval excludes 0 (+2.2 m on 4.6 m). Its accuracy
  rests on 2.4 GHz reach, and that is what P.833 shortens.
- **868 MHz alone gets worse, not better (F3)**, although its links lose less. One plausible mechanism, not
  diagnosed:
  - RSSI ranging inverts a path-loss model whose foliage term is the vegetation-type constant;
  - with P.833 the realised loss is lower than the model assumes, so ranges are biased long.
  - Its 5 km fix share rises (0.794 → 0.832), so this is an accuracy effect, not an availability one.
- **2.4 GHz alone after acquisition at 5 km improves** (96.6 → 72.1 m) on fewer fixes. The fixes that remain are
  the short, strong links.

## 3. Decision
**Not adopted into the preset.** P.833-10's Table 1 is one forest (St. Petersburg, mixed, 16 m trees), and the
Recommendation calls its values "only typical" (TECH_DEBT O.8).
- **What the campaign shows:** the system's endgame accuracy is sensitive to the 2.4 GHz foliage assumption, at
  about +50 % CEP95 between two defensible models. That sensitivity belongs in the threats-to-validity of any
  forest claim.
- **Which model to use:** an owner decision, best settled by the field test (E1).
- **The 868 MHz model mismatch (F3)** is recorded as TECH_DEBT O.9.

## 4. Table
| config | variant | arm | CEP95 (m) | CEP95 after 300 s (m) | fix share | tag-0 J |
|---|---|---|---|---|---|---|
| A | BASE | static_868 | 493.5 | 464.7 | 0.794 | 8.38 |
| A | BASE | static_2p4 | 396.8 | 96.6 | 0.057 | 13.36 |
| A | BASE | ca_tbl_full | 324.3 | 256.6 | 0.853 | 16.62 |
| A | BASE | fixed_threshold | 339.2 | 227.2 | 0.859 | 17.73 |
| A | P833 | static_868 | 536.8 | 470.2 | 0.832 | 8.38 |
| A | P833 | static_2p4 | 459.7 | 72.1 | 0.022 | 16.72 |
| A | P833 | ca_tbl_full | 413.8 | 341.9 | 0.862 | 20.40 |
| A | P833 | fixed_threshold | 510.1 | 443.2 | 0.862 | 21.76 |
| B | BASE | static_868 | 52.4 | 42.1 | 0.966 | 9.52 |
| B | BASE | static_2p4 | 8.7 | 7.7 | 0.838 | 8.96 |
| B | BASE | ca_tbl_full | 4.6 | 4.3 | 0.994 | 25.98 |
| B | BASE | fixed_threshold | 5.7 | 5.4 | 0.994 | 28.07 |
| B | P833 | static_868 | 48.3 | 38.3 | 0.966 | 9.51 |
| B | P833 | static_2p4 | 11.0 | 8.8 | 0.641 | 8.01 |
| B | P833 | ca_tbl_full | 6.8 | 6.1 | 0.990 | 24.98 |
| B | P833 | fixed_threshold | 8.4 | 6.2 | 0.992 | 26.07 |

F1 B static_2p4 dCEP95 +2.3 [-1.1, +6.2] m
F2 B static_2p4 dfix -0.197 [-0.217, -0.178]
F3 A static_868 dCEP95 +43.3 [-1.7, +98.2] m
F4 ca_tbl_full dCEP95 A +89.5 [-28.9, +228.2] on 324.3, B +2.2 [+1.4, +3.1] on 4.6 m

## Predictions

- **F1**: held
- **F2**: held
- **F3**: not held
- **F4**: not held
