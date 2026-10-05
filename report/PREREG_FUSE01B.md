# PREREG FUSE-01b — FUSE-01 re-measured with the subjects' finder oracle closed (TECH_DEBT N.21)

**Registered before any cell runs** (rule 13). Written 2026-10-04. Engine source at
registration: `9aebbba`. The preset there (87 settings) sets both
`--fuseFloorActive` (FUSE-01's adoption) and `--subjectFinderPseudoAnchor` (N.21's fix). This
commit adds this file, `scripts/fuse01b_run.sh` (the runner) and `scripts/fuse01b_analyse.py`
(the analysis: `scripts/fuse01_analyse.py`'s computation unchanged, its labels pointing
here), and nothing else. The outputs go to results/fuse01b_2026-10-04/, which does not exist
at registration.

**Why.** FUSE-01 (report/FUSE01_RESULTS.md) measured `--fuseFloorActive` on a preset in which
measured subjects ranged finders against the finders' TRUE positions (N.21). That held in
both cells and both arms, so FUSE-01's paired differences compare two leaky
configurations. Its adoption may still stand, but its absolute availabilities, and P2's
"above static_868", rest on the leak. This repeats FUSE-01's design and adoption rule
exactly, on the preset with the leak closed.

**Disclosed before registration.** An unregistered probe of the preset with the fuse on
(seeds 1-5, 1 800 s, `ca_tbl_full`), N.21's fix off against on:
- 5 km: subject availability 0.450 -> 0.385, UWB-delivered fixes 2 -> 0;
- endgame: availability 0.756 -> 0.688, UWB-delivered fixes 164 -> 147.

It did not run the fuse off, and did not run `static_868`.

The analysis was then exercised, after the predictions above were drafted and without
changing them, on a throwaway grid outside results/: seeds 1-2, 120 s, all four cells.
In it, availability rose with the fuse in both configurations (+0.06 and +0.18), and one
endgame seed had a UWB-delivered fix. That grid is not data for this registration.

---

## 1. Cells (`fuse01b_run.sh`)

- The preset (`--specProfile=true`), arms `ca_tbl_full` and `static_868`, 1 800 s, traces on,
  seeds 1-20. Four cells, 80 runs:
  - `main_off`, `main_on`: 5 km, with `--fuseFloorActive=false` given explicitly, and as the
    preset sets it;
  - `end_off`, `end_on`: the endgame (`--boxHalf=750 --finderMode=1`), likewise.
- N.21's fix applies in every cell and both arms.

## 2-3. Metrics and comparison

Exactly FUSE-01's (`report/PREREG_FUSE01.md` §2-3), computed by the same code:
- availability (the five subjects);
- tag 0's CEP50/95;
- UWB-delivered fixes, and those from a fused set;
- sessions with a defined spread;
- subject energy;
- paired on − off differences with a 10 000-resample seed bootstrap (generator seed
  20261005).

## 4. Predictions, made before any cell

- **P1.** At 5 km the fuse still raises availability: the paired difference's interval lies
  above 0. Its mechanism, two-range floor fixes kept off 868 MHz, does not involve finders.
- **P2.** At 5 km `ca_tbl_full` with the fuse is not below `static_868`: the upper end of the
  interval of (on − `static_868`) is at least 0.
- **P3.** In the endgame UWB-delivered fixes occur on at least 15 of 20 seeds with the fuse.
- **P4.** In the endgame tag 0's CEP95 is not worse by more than 10 %: the interval's upper
  end is at most +0.10.

## 5. Abort conditions

FUSE-01's §6, unchanged:
- a non-zero exit stops the grid;
- an `on` cell with no `fusefloor_*.csv` stops it;
- an existing outdir is refused;
- `--resume` re-runs only the cells without an `rc`.

## 6. Adoption

FUSE-01's rule, applied to this re-measurement. The preset keeps `--fuseFloorActive` if, in
both configurations:
- the mean paired availability difference is at least −0.01; and
- the mean relative CEP95 change of tag 0 is at most +0.10.

Otherwise the preset drops it, in its own commit (G24 re-run), and FUSE-01's adoption is
recorded as resting on N.21's leak. Each prediction is written up as held or failed in
report/FUSE01B_RESULTS.md, with FUSE-01's numbers beside these.
