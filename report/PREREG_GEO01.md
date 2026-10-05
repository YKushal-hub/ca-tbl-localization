# PREREG GEO-01 stage 2 — the least-infrastructure gateway geometry, simulated

**Registered before any cell runs** (rule 13). Written 2026-10-03. Engine source at
registration: `7bb6aaa`, the commit that adds `--specProfile`. P4 is complete (`f062b5e`).
This commit adds this file, `scripts/geo_stage2.sh` (the runner) and
`scripts/geo_stage2_analyse.py` (the analysis), and nothing else. The outputs go to
`results/geo01_2026-10-03/`, which does not exist at registration.

Owner decision 5 (2026-09-30): the preset's gateway geometry is found by a registered
study. It is the least infrastructure that meets a SAR criterion, with gateways only where
a vehicle can reach (`docs/SPEC_CONFORMANCE_PLAN.md` §3.4). Stage 1 (`report/PREREG_GEO01_STAGE1.md`,
results `report/GEO01_STAGE1_RESULTS.md`) pruned the candidates to ten. Stage 2 simulates
them on the P4 build, with the preset minus its geometry. Stage 3 confirms the winner and
runner-up under the full preset inside CAMP-SPEC.

---

## 1. Configuration

- **The preset minus geometry:** `--specProfile=true` with the candidate's geometry given
  explicitly (an explicit flag wins over the preset). The preset is the 68 settings of
  `kSpecProfile` at `7bb6aaa`:
  - five measured subjects;
  - four finders (`--finderMode=2`, round-robin tasking);
  - `--boxHalf=5000`;
  - the evidence gate on (decision 7);
  - the mesh, relays, routing, sessions and the rest.
  Two things are not yet in it, and so not in this study: the LR1121 basis (SY-2) and the
  calibrated PCS (CAL-02).
- **Arms:** `static_868`, the tier-1 baseline, and `ca_tbl_full`, the system, in one
  process per cell (`--arms=static_868,ca_tbl_full`).
- **Seeds** 1-10; `--duration=1800`; `--trace=true`.
- **Candidates** (stage 1's pruned set, flags exactly as stage 1 built them):

| candidate | flags |
|---|---|
| N3_star | `--nGateways=3 --gwLayout=star` |
| N3_cluster | `--nGateways=3 --gwLayout=cluster` |
| N4_arc90 | `--nGateways=4 --gwArcDeg=90 --gwArcCentreDeg=180` |
| N4_star | `--nGateways=4 --gwLayout=star` |
| N5_arc90 | `--nGateways=5 --gwArcDeg=90 --gwArcCentreDeg=180` |
| N5_arc140 | `--nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180` |
| N6_arc180 | `--nGateways=6 --gwArcDeg=180 --gwArcCentreDeg=180` |
| N6_arc240 | `--nGateways=6 --gwArcDeg=240 --gwArcCentreDeg=180` |
| N8_ring | `--nGateways=8` |
| N8_arc240 | `--nGateways=8 --gwArcDeg=240 --gwArcCentreDeg=180` |

100 cells × 2 arms. A 1 800 s preset run is about a minute, so the grid is about 25
minutes on 11 cores.

## 2. Metrics (per cell and arm; `scripts/geo_stage2_analyse.py`)

- **Availability:** the mean over the five subjects of `fixCycles / steps`, any tier
  (`subject_summary.csv`; subject 1 is tag 0's row).
- **CEP50 / CEP95:** tag 0's (`arm_summary.csv`). Subjects 2-5 have no CEP column in their
  summary. That is a limit, stated.
- **tFirstFixS and tFirst100mS:** tag 0's. A run that never reaches the event counts as
  1 800 s (censored), and the censoring is counted.
- **First UWB contact:** the first cycle tag 0 has a UWB neighbour (`nbrUwb ≥ 1` in its
  trace), else 1 800 s. This stands in for the plan's "finders' time to first UWB
  contact": tag 0 is the subject whose trace carries `nbrUwb`.
- **R ≤ 25 m:** the fraction of tag 0's cycles with `errM ≤ 25` (the spec's R, reported,
  not required: tier-1 RSSI cannot meet it at this scale).
- **Energy:** the mean over the five subjects of `totalJ`.

## 3. The operational criterion and the selection rule (registered)

Evaluated on the `ca_tbl_full` arm, over the 10 seeds:
- **Criterion:**
  - mean availability ≥ **0.95** (the spec's A, Architecture §1.3); and
  - median `tFirst100mS` ≤ **600 s**. The spec gives no time bound. Ten minutes, a third
    of the run, is OURS, fixed here before any cell.
- **Selection:**
  1. the smallest N at which at least one candidate meets the criterion;
  2. among those, the lowest mean CEP95;
  3. ties within 1 % go to fewer access points (cluster or star, then arc, then ring).
  The runner-up is the next in that order.
- **If no candidate meets the criterion at any N**, that is the result, reported as
  such. The preset still needs a geometry, so the registered fallback is the candidate
  with the highest mean availability (ties by CEP95). It is called a fallback wherever it
  is used.

`static_868` is reported beside every candidate. It does not enter the selection.

## 4. Predictions, made before any cell

- **P1.** Within the ten, the best candidate's availability does not fall as N rises.
  Any exception is reported and explained.
- **P2.** No N = 3 candidate meets the criterion. Stage 1's mean P(≥3) at N = 3 was at
  most 0.106.
- **P3.** No candidate has R ≤ 25 m on more than half of tag 0's cycles (the preset
  delivers no UWB fix: report/MS3_UWB_CHECKPOINT.md).
- **P4.** `ca_tbl_full`'s availability is at least `static_868`'s on every candidate.
  The full arm keeps the tier-1 floor, and adds Case D, relays and the mesh.

Each is written up as held or failed in `report/GEO01_RESULTS.md`, whichever way it goes.

## 5. Abort conditions

- A cell exits non-zero, or puts a gateway outside the box: stop and fix the cause.
  Never drop the cell.
- `results/geo01_2026-10-03/` exists before the run: the runner refuses. Never write over
  a results directory.

## 6. Outputs

`results/geo01_2026-10-03/`:
- `raw/<candidate>/s<seed>/`, the runs (ignored by git, as in stage 1; they regenerate
  from the command);
- `geo_stage2.csv`, one row per candidate, arm and seed;
- `geo_stage2.md`, the per-candidate table and the selection;
- `RUN.md`, the command, the tree state and the SHA-256 of every output.

The write-up is `report/GEO01_RESULTS.md`. The preset's geometry is then set from the
selection, in its own commit, and stage 3 runs inside CAMP-SPEC.
