# Testing and gates

## Unit suites

Twelve programs, built and run explicitly ([installation](../getting-started/installation.md)); on the tree of
2026-10-25 they total **401 checks with zero failures**.

| suite | checks | covers |
|---|---|---|
| `ca_tbl_packet_validator` | 76 | MAC: message types, wire sizes, access categories, ACK policy |
| `ca_tbl_session_test` | 32 | the session layer |
| `ca_tbl_routing_test` | 27 | fix routing |
| `ca_tbl_lifecycle_test` | 16 | node lifecycle |
| `ca_tbl_netlayer_test` | 30 | the network layer |
| `ca_tbl_relay_admission_test` | 22 | relay admission |
| `ca_tbl_sparse_anchor_test` | 33 | sparse-anchor cases |
| `ca_tbl_loocv_test` | 9 | leave-one-out calibration |
| `ca_tbl_tdoa_test` | 33 | TDoA |
| `ca_tbl_nlos_test` | 41 | NLOS classifier |
| `ca_tbl_integrity_test` | 40 | integrity check |
| `ca_tbl_imu_test` | 42 | IMU filter |

A suite that gains a check changes its count; the requirement is zero failures, not the number.

## Regression gates

`bash scripts/catbl_gates.sh` runs gates G1–G166 and reports a summary. It **runs every gate and never exits on the
first failure**: while one gate was deliberately red, three others could not be observed to run at all.

Rules the gates follow, each earned by a real defect:

- **No gate is trusted without forced-failure evidence.** Each has a negative fixture built in a temporary directory
  and observed to fail. A new gate ships with its fixture in the same commit.
- **Verification must not mutate its subject.** A gate fails the suite if it exits with a modified tree; regeneration
  goes to a temporary directory.
- **Completeness gates count rows, not test for non-empty.** With `--seedCount=N` the summary is appended per seed, so
  it is non-empty from the first seed.
- **Design against the failure mode.** Before writing a check, write down what a firing looks like *on disk*; a check
  for a defect whose mode is absence cannot be a scan of present data.

**Expected reds.** G11 (the analytic link-budget model against the engine; not to be tuned, because it is the one
instrument that caught the six-decibel defect) and G93 (the code default's tier-2 sensitivity row; the preset carries the
data-sheet row). Any other red is a regression. [`RULES.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/RULES.md) holds the current list.

## Null control

Adding a mechanism requires a byte-identical null control: with the flag off, `arm_summary.csv` and every trace match
the pre-change binary. Arms consume the shared random stream at their own rate, so any added transmission would perturb
every arm and no before/after result would be comparable without it.

## This documentation

`.github/workflows/docs.yml` builds the site with `mkdocs build --strict`, so a warning or a broken internal link
fails the build.
