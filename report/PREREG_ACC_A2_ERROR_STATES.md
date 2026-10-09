# PREREG ACC-A2 — the error-state IMU filter (Plan A, A2)

Exploratory grade. Committed before the first evaluation cell (seeds 1–20).

**Tuning, already done on seeds 21–30.** Arms `ca_tbl_full` and `static_2p4`, A and B, against the reference's
`ca_tbl_full` (`CR_*`), q_b ∈ {0, 3e-7, 1e-6}. The plan's rule picks the smallest endgame tag-0 CEP95 that does
not worsen 5 km:

| | endgame CEP95 (m) | 5 km |
| --- | --- | --- |
| reference | 18.9 | |
| q_b = 0 | 4.34 | |
| q_b = 3e-7 | 4.41 | |
| q_b = 1e-6 | 3.86 | mean per-seed CEP95 64 m against 37 m at q_b = 0: excluded |

**q_b = 0.** At q_b = 0 on the tuning seeds:
- 5 km: CEP50 19.4 → 10.5 m and CEP95 65.8 → 49.0 m;
- endgame: CEP50 4.8 → 1.4 m;
- energy: unchanged.

## Binary and cells

- **Binary:** the build of `88b0a101`. With the flag off it is byte-identical to the pre-change binary
  (334/334, seed 1, A and B). The reference cells are therefore the citable re-run's `REF_*` and `REFC_*`, and
  its `CAND_*` and `CANDC_*` for the B3 flags; those were built from `e87378cb` with identical off-path outputs.
- **Cells:** `--specProfile=true --duration=1800 --trace=true`, seeds 1–20. A = 5 km; B = endgame
  (`--boxHalf=750 --finderMode=1`).
- **Settings:**
  - **IE:** `--imuErrorStates=true`.
  - **IEB3:** `--imuErrorStates=true --dualBaseRelayTier=true --caseDInfoRatio=3`.
- **Arms:** `ca_tbl_full,fixed_threshold,static_2p4` in one run, and `poucet_fusion,lora_fusion` in another.

Metrics, the bootstrap and the verdict rule are those of `PREREG_ACC_B3_HYBRID.md`.

## Predictions

Unless stated, each is `ca_tbl_full` under IE minus `ca_tbl_full` in REF, paired over seeds.

- **E1.** Endgame tag 0 CEP95: WIN.
- **E2.** Endgame tag 0 CEP50: WIN.
- **E3.** 5 km tag 0 CEP50: WIN.
- **E4.** Availability: no LOSS in either configuration.
- **E5.** Energy: no LOSS above +1.5 J per node.
- **E6.** The change is filter-wide: `static_2p4`'s endgame CEP95 under IE minus REF is a WIN.
- **E7.** IEB3 minus IE, `ca_tbl_full`: no accuracy LOSS (within 25 m, CEP50, CEP95) in either configuration.
- **E8.** Under IEB3, `ca_tbl_full` minus each of `fixed_threshold`, `static_2p4`, `poucet_fusion` and
  `lora_fusion`, all in the same binary and setting: no accuracy LOSS in either configuration.

## Falsifiers

- E1 failing falsifies the filter's case.
- E8 failing means the combined configuration is not the most accurate arm. It is reported per comparator.
- Nothing is tuned on these seeds.
