# FUSE-01b — results: with the subjects' finder oracle closed, the fuse still raises availability by about 0.12 and keeps ca_tbl_full above static_868, but costs tag 0 13.7 % of its endgame CEP95; the registered rule does not adopt it, so the preset drops it

**Registered in `report/PREREG_FUSE01B.md` (`528752f`, engine `9aebbba`) before any cell
ran.** All 80 cells exit 0 and read `source 528752f dirty 0`. The outputs, commands and
digests are in results/fuse01b_2026-10-04/ (`RUN.md`). The analysis is
`scripts/fuse01b_analyse.py`, FUSE-01's computation unchanged, committed with the
registration and applied unchanged. Its tables are `FUSE01B.md` there.

**In one paragraph.** FUSE-01's comparison, repeated on a preset in which no measured
subject ranges a finder at its true position (TECH_DEBT N.21):
- **Availability:** the fuse raises it by +0.116 at 5 km and +0.120 in the endgame.
  `ca_tbl_full` with the fuse is above `static_868` at 5 km by 0.036, and P2 held again.
- **UWB-delivered fixes:** on all 20 endgame seeds, 525 of them.
- **The cost:** tag 0's endgame CEP95 is 13.7 % worse on average (interval [+1.6 %,
  +27.9 %]). FUSE-01 had +8.1 %.

The registered rule allows at most +10 % on the mean in both configurations. So **the
preset drops `--fuseFloorActive`**, and FUSE-01's adoption no longer stands.

---

## 1. The tables (`ca_tbl_full`, seeds 1-20, paired on − off), FUSE-01 beside

| | 5 km | endgame |
|---|---|---|
| availability, off → on | 0.317 → **0.433** | 0.652 → **0.773** |
| paired difference | **+0.116** [+0.100, +0.133] | **+0.120** [+0.109, +0.131] |
| FUSE-01's paired difference | +0.118 | +0.127 |
| `static_868` | 0.397 | 0.589 |
| `ca_tbl_full` on − `static_868` | **+0.036** [+0.018, +0.056] | +0.184 [+0.169, +0.199] |
| tag 0 CEP50 / CEP95, off → on (m) | 22.5 / 48.0 → 21.8 / 48.9 | 5.2 / 9.5 → 5.4 / 10.3 |
| relative CEP95 change | +0.026 [−0.070, +0.124] | **+0.137** [+0.016, +0.279] |
| FUSE-01's relative CEP95 change | +0.030 | +0.081 |
| UWB-delivered fixes, off → on (seeds > 0) | 0 → 0 | 0 → **525 (20)**, 518 from fused sets |
| sessions with a defined spread | 0 → 0 of 20 | 14 → 15 of 20 |
| subject energy, off → on (J) | 13.68 → 15.61 | 39.70 → 48.58 |

## 2. Predictions

- **P1 held.** At 5 km the fuse raises availability: +0.116, interval above 0.
- **P2 held.** At 5 km `ca_tbl_full` with the fuse is above `static_868`, by 0.036
  [+0.018, +0.056]. With the oracle closed in both arms, the fuse still reverses GEO-01's
  P4. The margin is smaller than FUSE-01's 0.066.
- **P3 held.** UWB-delivered fixes on 20 of 20 endgame seeds (525). 518 are fused sets: UWB
  ranges with the floor's 868 MHz ranges, labelled UWB by the flag's rule (OURS, N.14).
- **P4 failed.** Tag 0's endgame CEP95 is 13.7 % worse on average; the interval lies
  entirely above 0 and reaches +27.9 %.

## 3. Adoption (§6)

The rule needs both, in both configurations:
- the mean availability difference at least −0.01: it is +0.116 and +0.120, so it passes;
- the mean relative CEP95 change at most +0.10: it is +0.026 at 5 km but **+0.137 in the
  endgame**, so it fails.

**The preset drops `--fuseFloorActive`,** in its own commit with G24 re-run, as registered.
FUSE-01's adoption is recorded as no longer standing. Its paired availability differences
reproduce closely (+0.118/+0.127 against +0.116/+0.120), but its endgame accuracy cost does
not (+8.1 % against +13.7 %).

## 4. What else the data say (descriptive)

- **N.21's oracle moved availability little,** once subjects ask for finder entries and hold
  them:
  - 5 km: 0.333 → 0.317 (off) and 0.451 → 0.433 (on);
  - endgame: 0.648 → 0.652 and 0.775 → 0.773.
  - The 5-seed probe in the registration showed a larger drop (0.450 → 0.385 at 5 km). It
    ran the first build of the flag, whose unstaggered digest questions drowned the
    finders' answers (fac5da6), so it measured the collision as much as the oracle.
- **Where the accuracy goes.** A fused set adds fixes at the geometry of 1-2 active ranges
  plus the floor's, as FUSE-01 found.
  - In the endgame, subjects spend more time on UWB with the fuse: occupancy 0.085 → 0.115,
    UWB energy 28.2 → 38.7 J.
  - Tag 0's tail moves out more here than in FUSE-01. Rule 8 forbids reading the
    difference between the two campaigns as a mechanism: their binaries differ in what
    the subjects transmit.
- **The fuse remains a working mechanism** (G63), and its availability gain is robust
  across both campaigns. What fails is the registered accuracy bound in the endgame.

## 5. What follows

- The preset drops `--fuseFloorActive` (G24 re-run). The UWB-delivered fix population,
  which the fuse alone supplied, returns to zero under the preset.
- N.14 is reopened. The remedy exists and works on availability, but it fails the
  registered accuracy bound in the endgame. Whether to adopt it anyway is the owner's
  call, as is a variant that fuses only when the floor is short, which helped 5 km without
  the endgame cost.
