# FUSE-01 — results: fusing the active tier with the floor raises availability in both configurations and brings UWB-delivered fixes to every endgame seed; tag 0's endgame CEP95 is 8 % worse on average; the registered rule adopts it

**Registered in `report/PREREG_FUSE01.md` (`67da4ef`, engine `022212e`) before any cell
ran.** All 80 cells exit 0 and read `source 67da4ef dirty 0`. The outputs, commands and
digests are in results/fuse01_2026-10-04/ (`RUN.md`). The analysis is
`scripts/fuse01_analyse.py`, committed with the registration and applied unchanged; its
tables are `FUSE01.md` there.

**In one paragraph.** `--fuseFloorActive` (TECH_DEBT N.14) did what its mechanism
predicted:
- At 5 km the subjects keep two-range floor fixes off 868 MHz. Availability rose by 0.118,
  and `ca_tbl_full` is now above `static_868`, reversing GEO-01's P4.
- In the endgame the 1-2 UWB ranges a subject gets from a converging finder are fused with
  the floor's ranges. UWB-delivered fixes went from 1 (one seed) to 552 (all 20 seeds).

It costs:
- tag 0's endgame CEP95 is 8 % worse on average, with an interval reaching +18 %, so P4
  failed;
- the subjects spend more time, and more energy, on UWB.

By the registered rule (mean availability not lower by more than 0.01, mean CEP95 not worse
by more than 10 %, in both configurations) the preset adopts the flag.

---

## 1. The tables (`ca_tbl_full`, seeds 1-20, paired on − off)

| | 5 km (main) | endgame (end) |
|---|---|---|
| availability, off → on | 0.333 → **0.451** | 0.648 → **0.775** |
| paired difference | **+0.118** [+0.103, +0.132] | **+0.127** [+0.118, +0.136] |
| `static_868` (the same in both cells) | 0.385 | 0.586 |
| `ca_tbl_full` on − `static_868` | **+0.066** [+0.055, +0.079] | +0.189 [+0.178, +0.200] |
| tag 0 CEP50 / CEP95, off → on (m) | 21.9 / 47.3 → 20.1 / 47.6 | 5.0 / 9.0 → 4.7 / 9.3 |
| relative CEP95 change | +0.030 [−0.080, +0.152] | **+0.081** [−0.006, +0.184] |
| UWB-delivered fixes, off → on (seeds > 0) | 0 → 4 (3) | 1 (1) → **552 (20)** |
| of which from a fused set | 4 | 538 |
| sessions with a defined precision spread | 0 → 0 of 20 | 15 → 14 of 20 |
| subject energy, off → on (J) | 12.93 → 14.84 | 37.92 → 46.58 |

`static_868` has no floor, so the flag cannot act on it. It reproduced identically between
the cells of a configuration, as the registration said it would.

## 2. Predictions

- **P1 held.** At 5 km availability rose: +0.118, interval above 0.
- **P2 held.** At 5 km `ca_tbl_full` is no longer below `static_868`. It is above it by 0.066,
  where GEO-01 found it below by 0.03-0.06 in every cell.
- **P3 held.** UWB-delivered fixes on 20 of 20 endgame seeds (552 in all). But 538 of them
  come from a fused set: a union of 1-2 UWB ranges with the floor's 868 MHz ranges,
  labelled UWB by the flag's rule (OURS, N.14). Fourteen are UWB fixes with no floor range.
- **P4 failed.** Tag 0's endgame CEP95 is 8.1 % worse on average, and the interval's upper
  end, +18.4 %, exceeds the registered +10 %. At 5 km the change is +3.0 %, also with a
  wide interval.

## 3. Adoption (§7)

The rule asks for both, in both configurations:
- the mean paired availability difference at least −0.01: it is +0.118 and +0.127;
- the mean relative CEP95 change at most +0.10: it is +0.030 and +0.081.

**The preset adopts `--fuseFloorActive`,** in its own commit with G24 re-run. The
registration made the mean, not the interval, the adoption test. P4's failure is reported
here and is not a reason to withhold it.

## 4. What else the data say (descriptive, not registered)

- **Where the energy goes.** The subjects' UWB energy rose from 25.8 to 37.1 J in the
  endgame, and their UWB occupancy from 0.084 to 0.110. A fused UWB set solves, so a subject
  on UWB stays there longer instead of being demoted for want of three UWB anchors. At 5 km
  the rise is smaller (UWB 0.51 → 2.20 J; 2.4 GHz 6.79 → 7.58 J).
- **Tag 0 is barely moved.** Its tier occupancy is unchanged to 0.005 in both configurations
  and its escalations by about six. The availability gain is the subjects': it is their
  two-range floor fixes (5 km) and fused sets (endgame) that the old rule discarded.
- **The fused sets.** At 5 km, 40 345 node-cycles over 20 seeds took a union. 19 171 of them
  had no active range and a short floor, the GEO-01 case. 19 535 became fixes.
- **Why CEP95 can worsen while availability rises.** A union adds fixes on cycles that had
  none, at the geometry of 1-2 active ranges plus a short or distant floor. Those are worse
  than the median fix. The tail moves out even where every existing fix stays the same.
  This is a reading of the mechanism, not a measured decomposition.

## 5. What follows

- The preset takes `--fuseFloorActive` (its own commit; G24 re-run, where the UWB-delivered
  fix population should no longer be zero).
- The UWB-delivered fix population that G24 and CAMP-SPEC count is now mostly fused sets.
  Any report that quotes "UWB fixes" under the preset must say so: they are UWB ranges
  fused with 868 MHz ranges, not three-anchor UWB fixes.
- MS-3's finding stands for three-anchor UWB: a subject still has at most one finder in UWB
  reach.
