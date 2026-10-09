# PREREG ACC-B3-US — the unknown start (N.39)

Exploratory grade. Committed before the first cell. One smoke run (seed 1, 5 km, 1800 s,
`ca_tbl_full` and `static_2p4` at s = 100) was made while building the gate. In it, five-node within
25 m was 0.566 for `ca_tbl_full` and 0.624 for `static_2p4`. It is disclosed here and is not part of
the evaluation.

## Question

Every filter is seeded at or next to the truth (ACC-A static 2.4 GHz diagnosis, `4ece0f0f`), so the
5 km within-25 m share partly measures dead reckoning from a known start. With the start unknown at
the level the prior already claims (s = 100 m), how do the arms compare?

## Binary and cells

- **Binary:** the build of `53766acb` (scenario code identical to it). With s = 0 it is byte-identical
  to the pre-change binary (334/334, seed 1, A and B). The s = 0 reference therefore comes from the
  citable re-run's `REF_*` and `CAND_*` cells, built from `e87378cb`, whose outputs at s = 0 are the
  same.
- **Cells:** `--specProfile=true --duration=1800 --trace=true`, seeds 1–20.
  - A = 5 km.
  - B = endgame (`--boxHalf=750 --finderMode=1`).
- **Settings:**
  - **US:** `--unknownStartSigmaM=100`.
  - **USB3:** `--unknownStartSigmaM=100 --dualBaseRelayTier=true --caseDInfoRatio=3`.
- **Arms:** `ca_tbl_full,fixed_threshold,static_2p4` in one run, and `poucet_fusion,lora_fusion` in
  another.

Metrics, the bootstrap and the verdict rule are those of `PREREG_ACC_B3_HYBRID.md`.

## Predictions

- **U1.** At 5 km, `static_2p4`'s five-node within-25 m falls by at least 0.10 from s = 0 to s = 100
  (paired over seeds).
- **U2.** At 5 km and s = 100 (US), `ca_tbl_full` minus `static_2p4` in five-node within 25 m is a WIN.
- **U3.** At s = 100 (US), `ca_tbl_full` minus `static_2p4` in availability is a WIN in A and B.
- **U4.** In the endgame at s = 100 (US), `ca_tbl_full` minus `static_2p4` in five-node within 25 m is
  not a LOSS.
- **U5.** At 5 km, `ca_tbl_full`'s drop in five-node within 25 m from s = 0 to s = 100 is smaller than
  `static_2p4`'s. Test: the difference of the paired differences has a bootstrap interval above 0.
- **U6.** At s = 100, USB3's `ca_tbl_full` minus US's `ca_tbl_full` has no accuracy LOSS (within 25 m,
  tag 0 CEP50, CEP95) in either configuration.

## Falsifiers

- U2 or U4 failing means the hybrid's accuracy case does not hold even once the start is honest.
  It is reported as such.
- Nothing is tuned on these seeds.
