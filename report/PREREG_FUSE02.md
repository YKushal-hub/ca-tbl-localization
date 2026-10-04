# PREREG FUSE-02 — the fuse narrowed to a short floor (TECH_DEBT N.14), on the finished preset

**Registered before any cell runs** (rule 13). Written 2026-10-04. Engine source at
registration: `c10b85d` (`--fuseFloorShort`, gate G85), with the preset as it stands there
(97 settings: the spec's three gateways, N3_star; half-duplex per chip; both colliding
frames judged; requests over the mesh). The preset sets no fuse. This commit adds this file,
`scripts/fuse02_run.sh` (the runner) and `scripts/fuse02_analyse.py` (the analysis:
`scripts/fuse01_analyse.py`'s computation unchanged, its labels pointing here, and P3's
threshold this registration's own), and nothing else. The outputs go to
results/fuse02_2026-10-04/, which does not exist at registration.

**Why.** FUSE-01b (report/FUSE01B_RESULTS.md) found that fusing a short active set with any
floor set raises availability by about 0.12 in both configurations but costs tag 0's
endgame CEP95 13.7 %, over FUSE-01's registered +10 % bound. The owner's call on N.14,
delegated on 2026-10-04 (docs/DESIGN_DECISIONS.md §9), is the narrower variant: fuse only
when the floor is short too, the case FUSE-01b's 5 km gain came from. This measures it on
the preset as it now stands, which differs from FUSE-01b's in geometry and channel.

**Disclosed before registration.** G85's runs (seed 7, 900 s, `ca_tbl_full` only, the
narrowed fuse on; not data for this registration): 414 unions in the endgame (38 on UWB, 1
fix using UWB ranges) and 506 at 5 km. The full fuse in the endgame formed 729 (63 on UWB,
2 fixes using UWB ranges). No run of the preset without a fuse was compared with them.

---

## 1. Cells (`fuse02_run.sh`)

- The preset (`--specProfile=true`), arms `ca_tbl_full` and `static_868`, 1 800 s, traces on,
  seeds 1-20. Four cells, 80 runs:
  - `main_off`, `main_on`: 5 km, the preset as it stands, and with
    `--fuseFloorActive=true --fuseFloorShort=true`;
  - `end_off`, `end_on`: the endgame (`--boxHalf=750 --finderMode=1`), likewise.

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

- **P1.** At 5 km the narrowed fuse raises availability: the paired difference's interval
  lies above 0. Its mechanism, the two-range floor fixes kept off 868 MHz, is the one the
  narrowing keeps.
- **P2.** At 5 km `ca_tbl_full` with the narrowed fuse is not below `static_868`: the upper
  end of the interval of (on − `static_868`) is at least 0.
- **P3.** In the endgame UWB-delivered fixes occur on at least 5 of 20 seeds with it. Lower
  than FUSE-01's 15: the narrowing fuses a UWB set only when the floor is short, and G85's
  single run had one fix using UWB ranges.
- **P4.** In the endgame tag 0's CEP95 is not worse by more than 10 %: the interval's upper
  end is at most +0.10. A floor of three or more keeps the floor-first path, which is where
  FUSE-01b's cost was measured.

## 5. Abort conditions

FUSE-01's §6, unchanged:
- a non-zero exit stops the grid;
- an `on` cell with no `fusefloor_*.csv` stops it;
- an existing outdir is refused;
- `--resume` re-runs only the cells without an `rc`.

## 6. Adoption

FUSE-01's rule. The preset adopts `--fuseFloorActive=true` with `--fuseFloorShort=true`, in
its own commit (G24 re-run), if, in both configurations:
- the mean paired availability difference is at least −0.01; and
- the mean relative CEP95 change of tag 0 is at most +0.10.

Otherwise the preset keeps the floor-first rule and N.14 is recorded as closed without a
fuse. Each prediction is written up as held or failed in report/FUSE02_RESULTS.md, beside
FUSE-01b's numbers, which were measured on a different geometry and channel and so are not
compared as magnitudes (rule 8).
