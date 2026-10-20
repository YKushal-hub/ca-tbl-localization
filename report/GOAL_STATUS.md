# Goal status — the design requirement, measured

**The requirement** (`Architecture.md` §1.3): **A ≥ 0.95 at R ≤ 25 m**, the share of ALL decision cycles whose
position estimate is within 25 m, with a cycle without a fix counting against it. Until 2026-10-20 the recent
campaigns reported the fix share (`fixCycles/steps`) as "availability". That is a different quantity. This file
reports the requirement itself, from each run's traces, with `scripts/goal_report.py`.

Written 2026-10-20 from the two campaigns whose traces are on disk: EKF5-WINDOW (seeds 61–80) and EKF5-WINDOW-END
(seeds 81–100). Both are the realistic world under REAL-BEST's S stack (S) and with `--ekf5Window=60` (W). They are
re-read here, not re-run. **Every number below is a post-hoc reading of registered cells, not a registered result.**

## Registered, 2026-10-20: PRESET-2 (seeds 101–120, `report/PRESET2_RESULTS.md`)

CA-TBL means, S (REAL-BEST's stack) → N (S plus the PRESET-2 candidates; not adopted by its rule, DESIGN_DECISIONS
17):

| config | A(25 m) tag 0 | A(25 m) five-node | CEP95 tag 0 | subject J |
|---|---|---|---|---|
| A, 5 km | 0.386 → 0.829 | 0.268 → 0.503 | 700 → 303 m | 19.4 → 16.2 |
| B, endgame | 0.994 → 0.995 | 0.856 → 0.891 | 3.9 → 3.3 m | 21.2 → 20.3 |
| C, 8-gateway ring | 0.790 → 0.959 | 0.505 → 0.658 | 229 → 47 m | 17.8 → 14.7 |

**The requirement under N:**
- met for tag 0 in the endgame and on the 8-gateway ring;
- not met for the five-node pool anywhere;
- not met for tag 0 at the brief's 5 km three-gateway geometry.

**Against comparators under N:**
- CA-TBL beats both single-band arms on five-node A(25 m) everywhere;
- it is level with Poucet fusion and fixed_threshold (every interval spans 0);
- it uses 383–446 J less subject energy than Poucet fusion.

## Registered, 2026-10-21: PRESET-3 — the preset the owner adopted (`report/PREREG_PRESET3.md`)

| configuration | tag-0 A(25 m) | five-node A(25 m) | runs ≥ 0.95 (five-node) | subject J |
|---|---|---|---|---|
| 5 km | **0.979** | 0.811 | 0.15 | 13.4 |
| endgame | **0.997** | **0.982** | **1.00** | 16.9 |
| 8-gateway ring | **0.985** | 0.935 | 0.80 | 12.9 |

**The requirement is met:**
- for tag 0 in every geometry;
- for all five nodes in the endgame.

**It is short** for the five-node pool at 5 km and on the ring.

**Against comparators:** level with `fixed_threshold`; above Poucet on the five-node pool at about 1/30 of its
energy. Everything below this section predates the adopted preset.

## Registered, 2026-10-20: FIDELITY-1b — the corrected receiver, now the preset (`report/PREREG_FIDELITY1B.md`)

CA-TBL, five-node A(25 m), F0 → F1:
- 5 km: 0.362 → 0.436;
- endgame: 0.896 → 0.913;
- 8-gateway ring: 0.605 → 0.757; tag 0 there 0.911 → 0.975.

**Against comparators under F1:**
- at 5 km CA-TBL read below `fixed_threshold` on seeds 161–180 (−0.056). It is **level** on fresh seeds 181–200
  (INTEG3, −0.001), so the two campaigns read together say level;
- in the endgame it beats Poucet, +0.053, at about 1/20 the energy.

## Registered, 2026-10-20: ANCHOR-SUPPLY (seeds 121–140, `report/PREREG_ANCHOR_SUPPLY.md`)

S stack plus the window, CA-TBL, five-node A(25 m):
- **endgame:** four finders 0.873, **eight finders 0.970** (meets the requirement on average), pairs at four finders
  0.866 at −35 % energy;
- **5 km:** 0.41 with four finders, 0.58 with eight.

CA-TBL beats Poucet fusion on five-node A(25 m) in every endgame variant at about 1/20 the energy.

The post-hoc readings below predate PRESET-2.

## Where the system stood (post hoc, before PRESET-2)

- **Endgame (B), tag 0 meets the requirement.** A(25 m) is 0.990–0.993 for every precise arm, CA-TBL included.
- **Endgame, five nodes: the subjects carry the gap.** Five-node A(25 m) is 0.84–0.88 under S and 0.91 under W for
  CA-TBL. Fewer than half the runs reach 0.95 across all five nodes.
- **5 km (A): far from the requirement.** CA-TBL tag 0 0.43 (S) and 0.54 (W); five-node 0.27 and 0.34. No run of
  any arm reaches 0.95. The cause is anchor supply (TECH_DEBT N.62): runs that never get usable geometry.
- **The window helps the requirement's metric at 5 km.** The registered test read CEP95 and called the 5 km effect
  unsupported. Paired by seed, W − S, CA-TBL, A(25 m):
  - 5 km, tag 0: **+0.115 [0.016, 0.234]**;
  - 5 km, five-node: **+0.070 [0.003, 0.134]**;
  - endgame, five-node: +0.070 [0.016, 0.130] on seeds 61–80, and +0.031 [−0.043, +0.093] on 81–100.
  `fixed_threshold` and Poucet gain similarly. The window is a property of the filter, not of the policy.
  Post hoc; PRESET-2 registers A(25 m) as its primary metric.

## Tables

## ekf5_window_2026-10-20

| group | arm | runs | A(25 m) tag 0 | A(25 m) five-node | runs meeting 0.95 | CEP95 tag 0 (m) | subject J |
|---|---|---|---|---|---|---|---|
| A_S | ca_tbl_full | 20 | 0.425 | 0.266 | 0.00 | 960.5 | 20.81 |
| A_S | fixed_threshold | 20 | 0.400 | 0.248 | 0.00 | 948.5 | 20.65 |
| A_S | static_868 | 20 | 0.010 | 0.005 | 0.00 | 785.3 | 8.03 |
| A_S | static_2p4 | 20 | 0.185 | 0.093 | 0.00 | 1444.9 | 14.04 |
| A_S | poucet_fusion | 20 | 0.336 | 0.256 | 0.00 | 952.1 | 465.29 |
| A_W | ca_tbl_full | 20 | 0.540 | 0.336 | 0.00 | 932.4 | 20.68 |
| A_W | fixed_threshold | 20 | 0.535 | 0.355 | 0.00 | 1001.4 | 21.28 |
| A_W | static_868 | 20 | 0.011 | 0.016 | 0.00 | 790.3 | 8.08 |
| A_W | static_2p4 | 20 | 0.170 | 0.094 | 0.00 | 1451.7 | 14.02 |
| A_W | poucet_fusion | 20 | 0.600 | 0.352 | 0.00 | 640.3 | 463.01 |
| B_S | ca_tbl_full | 20 | 0.990 | 0.844 | 0.25 | 4.6 | 19.86 |
| B_S | fixed_threshold | 20 | 0.990 | 0.856 | 0.30 | 4.5 | 19.59 |
| B_S | static_868 | 20 | 0.794 | 0.303 | 0.00 | 67.5 | 8.16 |
| B_S | static_2p4 | 20 | 0.990 | 0.679 | 0.00 | 6.4 | 5.23 |
| B_S | poucet_fusion | 20 | 0.992 | 0.812 | 0.15 | 3.6 | 372.81 |
| B_W | ca_tbl_full | 20 | 0.991 | 0.914 | 0.45 | 3.1 | 21.13 |
| B_W | fixed_threshold | 20 | 0.991 | 0.857 | 0.40 | 3.0 | 22.25 |
| B_W | static_868 | 20 | 0.881 | 0.301 | 0.00 | 42.9 | 8.21 |
| B_W | static_2p4 | 20 | 0.989 | 0.680 | 0.05 | 6.7 | 5.22 |
| B_W | poucet_fusion | 20 | 0.992 | 0.825 | 0.15 | 3.0 | 372.87 |

## ekf5_window_end_2026-10-20

| group | arm | runs | A(25 m) tag 0 | A(25 m) five-node | runs meeting 0.95 | CEP95 tag 0 (m) | subject J |
|---|---|---|---|---|---|---|---|
| B_S | ca_tbl_full | 20 | 0.993 | 0.875 | 0.35 | 3.9 | 20.32 |
| B_S | fixed_threshold | 20 | 0.993 | 0.841 | 0.25 | 3.7 | 20.62 |
| B_S | static_868 | 20 | 0.817 | 0.283 | 0.00 | 87.3 | 8.23 |
| B_S | static_2p4 | 20 | 0.990 | 0.707 | 0.10 | 6.4 | 5.49 |
| B_S | poucet_fusion | 20 | 0.993 | 0.753 | 0.05 | 3.3 | 389.75 |
| B_W | ca_tbl_full | 20 | 0.993 | 0.906 | 0.45 | 3.1 | 19.53 |
| B_W | fixed_threshold | 20 | 0.993 | 0.906 | 0.40 | 3.1 | 20.24 |
| B_W | static_868 | 20 | 0.867 | 0.278 | 0.00 | 58.0 | 8.31 |
| B_W | static_2p4 | 20 | 0.991 | 0.677 | 0.10 | 6.2 | 5.42 |
| B_W | poucet_fusion | 20 | 0.993 | 0.762 | 0.10 | 3.0 | 389.59 |


