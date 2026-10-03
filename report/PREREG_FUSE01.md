# PREREG FUSE-01 — the active tier fused with the floor (TECH_DEBT N.14), on the preset

**Registered before any cell runs** (rule 13). Written 2026-10-04. Engine source at
registration: `022212e` (`--fuseFloorActive`, gate G63), with the preset as it stands
there (83 settings). This commit adds this file, `scripts/fuse01_run.sh` (the runner) and
`scripts/fuse01_analyse.py` (the analysis), and nothing else. The outputs go to
results/fuse01_2026-10-04/, which does not exist at registration.

**Owner decision of 2026-10-03.** N.14's remedy is built behind a default-off flag, and this
registered campaign decides whether the preset adopts it.

**The question.** The MS-3 checkpoint, GEO-01 and CAMP-SPEC traced the preset's weakest
results to one rule. If the active tier returns fewer than three ranges, the cycle uses the
868 MHz floor's set when the floor has three. Otherwise it uses the active tier's 1-2
ranges for the sparse cases. So:
- a UWB range is never fused when the floor solves;
- a subject off 868 MHz loses the two-range floor fixes `static_868` keeps.

`--fuseFloorActive` fuses the active tier's ranges with the floor's whenever the active tier
is short, and runs the sparse cases on that union. Does that recover the availability and
the UWB-delivered fixes, without costing accuracy?

---

## 1. Cells (`fuse01_run.sh`)

- **The preset** (`--specProfile=true`), arms `ca_tbl_full` and `static_868`, 1 800 s, traces
  on, seeds 1-20.
- Four cells:
  - `main_off`, `main_on`: the preset at 5 km, without and with `--fuseFloorActive`;
  - `end_off`, `end_on`: its endgame (`--boxHalf=750 --finderMode=1`), likewise.
- `static_868` has no floor, so the flag cannot act on it. Each arm runs on its own reset
  stream, so `static_868` is the same reference in both cells of a configuration.
- 80 runs in all.

## 2. Metrics (`ca_tbl_full`; `static_868` as the reference)

- **Availability:** the mean over the five subjects of `fixCycles / steps`
  (`subject_summary.csv`), as GEO-01 and CAMP-SPEC defined it.
- **Tag 0's CEP50 / CEP95** (`arm_summary.csv`).
- **UWB-delivered fixes:** cycles with `hadFix` 1 and `fixTier` UWB, over tag 0's trace and
  the subjects' traces.
  - Under the flag, a union that holds UWB ranges is labelled UWB (OURS; the help says
    so).
  - So the UWB fixes that came from a fused set are counted separately, from
    `fusefloor_*.csv`.
- **Sessions:** seeds on which the base station's batch precision spread is defined.
- **Subject energy:** the mean `totalJ` of the five subjects.
- **The fused sets:** how many, how many had no active range and a short floor, and how many
  became fixes (`fusefloor_*.csv`).

## 3. Comparison

Paired by seed, on minus off, `ca_tbl_full`. Each mean gets a 95 % seed bootstrap (10 000
resamples, generator seed 20261005):
- the availability difference;
- the relative CEP95 change, on / off − 1, tag 0;
- the availability of `ca_tbl_full` on, minus `static_868`.

The flag changes which ranges are fused, so the filter, the escalations and every frame
after the first fused cycle move. The comparison is a paired difference over 20 seeds,
never a single seed's (rule 8).

## 4. What the flag is expected to do, by mechanism

- **At 5 km.** Subjects off 868 MHz with 0-1 active ranges keep their two-range floor
  fixes, now Case A on the floor's ranges.
- **In the endgame.** A UWB cycle with one or two UWB ranges fuses them with the floor's
  set, instead of discarding them.

## 5. Predictions, made before any cell

- **P1.** At 5 km the availability of `ca_tbl_full` rises: the paired difference's interval
  lies above 0.
- **P2.** At 5 km `ca_tbl_full` with the flag is no longer below `static_868` on
  availability: the upper end of the interval of (on − `static_868`) is at least 0. GEO-01
  P4 found it below by 0.03-0.06 in every cell.
- **P3.** In the endgame, UWB-delivered fixes occur on at least 15 of the 20 seeds with the
  flag. CAMP-SPEC found one in 20 seeds without it.
- **P4.** In the endgame, tag 0's CEP95 is not worse by more than 10 %: the upper end of the
  interval of the relative change is at most +0.10.

Each is written up as held or failed in report/FUSE01_RESULTS.md.

## 6. Abort conditions

- A run exits non-zero: stop, fix the cause, never drop the cell.
- An `on` cell writes no `fusefloor_*.csv`: the flag did not act. Stop.
- results/fuse01_2026-10-04/ exists before the runner starts: it refuses.

If the grid is interrupted, the cells without an `rc` file are re-run with the same command
line (`fuse01_run.sh --resume`). The simulation is deterministic, so a re-run cell
reproduces every file; that is recorded in the results' `RUN.md`.

## 7. Adoption

The preset adopts `--fuseFloorActive`, in its own commit (G24 re-run), if both hold in
**both** configurations:
- the mean paired availability difference is at least −0.01;
- the mean relative CEP95 change of tag 0 is at most +0.10.

Otherwise the preset keeps the floor-first rule, and the report says why. The predictions do
not decide adoption; this rule does.
