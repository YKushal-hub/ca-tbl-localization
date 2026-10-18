# PREREG S2-C-NLOS — does NLOS-keyed R inflation add anything, and can a fielded detector replace the oracle? (exploratory grade)

**Registered before any cell runs** (rule 13). Written 2026-10-09. Engine: `4e087627`
(`--nlosRSource`, G131) on the S2-C branch. This commit adds this file,
`scripts/s2c_nlos_run.sh` and `scripts/s2c_nlos_analyse.py`. Outputs go outside the repository.

**Provenance.** `run_id.txt`'s `source` line is this commit's worktree, not the build commit. The
engine is pinned by the `build_s2c_nlos.sha256` manifest, checked before launch, and by the compile
stamp, identical in every run.

## 1. What is already known (disclosed; `report/S2C_NLOS_AUDIT.md`, from ACC-A's logs)

- **The preset keys R on the propagation truth** (`--nlosSource=propagation`) and triples an NLOS
  range's sigma (`nlosSigmaInflation` 3, `ca-tbl-policy.cc:133`).
- **Simulated NLOS biases range little.**
  - 868 MHz: none; the range is truth plus zero-mean noise.
  - 2.4 GHz: N(1.5, 0.8) m truncated to [0, 6], with a 3.6 m floor against 3.0.
  - UWB: lognormal, median 0.3 m.
  - Measured 2.4 GHz NLOS residual mean: +0.42 to +0.64 m.
- **Most NLOS links are lost before they are scored.** At 2.4 GHz, 1 027 lost against 24
  scored (5 km).
- **The detectors are weak:** precision 0.02-0.08, AUC 0.62-0.78.
- G131's seed-7 run is disclosed. No ablation result has been read.

## 2. Cells (`scripts/s2c_nlos_run.sh`)

Common flags: `--arms=ca_tbl_full --specProfile=true --duration=1800 --trace=true
--traceCovariance=true --traceSubjectConsistency=true --imuQuScale=10`. Known start.

| cell | `--nlosRSource` | configuration | seeds |
|---|---|---|---|
| N_{A,B}_oper | operative (the control: the oracle, as the preset runs) | A 5 km; B `--boxHalf=750 --finderMode=1` | 1-10 |
| N_{A,B}_none | none | A; B | 1-10 |
| N_{A,B}_det | detector | A; B | 1-10 |

That is 60 runs. Every difference is paired within this binary and campaign.

## 3. Metrics (`scripts/s2c_nlos_analyse.py`)

Per seed, as `PREREG_S2C` §3 defines them (`s2c_analyse.seed_metrics`):
- availability, w25, median error, tag 0 CEP95, J per node, first fix;
- subjects' NIS > 9.21 share and ANEES;
- tag 0's NIS > 9.21 share (trace `mahalanobis`, fix rows, t ≥ 120 s).

Paired as `none − oper` and `det − oper`, mean [95 % bootstrap].

## 4. Predictions

| # | prediction | confidence |
|---|---|---|
| **Q1** | none − oper: tag 0 CEP95 and w25 intervals each contain 0, A and B (the oracle inflation adds nothing measurable) | medium |
| **Q2** | det − oper: tag 0 CEP95 interval not entirely below 0, A and B (the detector does not beat the oracle) | high |
| **Q3** | det − none: tag 0 CEP95 and w25 intervals each contain 0, A and B (the detector is no better than no NLOS handling) | medium |
| **Q4** | none − oper: tag 0 NIS > 9.21 share, mean ≥ 0, A and B (less inflation, larger innovations) | low |

## 5. Abort conditions

The cause is fixed and no cell is dropped:
- a run without an `rc`, or exiting non-zero;
- a `run_id.txt` without `dirty 0`, or whose compile stamp differs from the other runs';
- fewer than 10 seeds in a cell;
- a prediction that cannot be evaluated.
