# GEO-01 stage 2 — results: no geometry meets the criterion; the registered fallback is N8_ring

**Registered in `report/PREREG_GEO01.md` (`d05992b`) before any cell ran.** The outputs,
the commands and the SHA-256 of every summary are in `results/geo01_2026-10-03/`; all 100
cells' `run_id.txt` read `source d05992b dirty 0`. The first pass was interrupted with 52
cells finished. The rest were re-run with the registered command line after a determinism
check: a finished cell reproduced all 71 of its files byte for byte (`RUN.md`,
`resumed_cells.txt`). The analysis is `scripts/geo_stage2_analyse.py`, committed with the
registration and applied unchanged. Its table is `geo_stage2.md`.

**Verdict.** No candidate meets the operational criterion at any N. Availability ≥ 0.95
was missed by a wide margin: at most 0.329 on `ca_tbl_full`. The registered fallback, the
highest mean availability, picks **N8_ring** (8 gateways on a ring at 0.6 of the
half-width, 3 km). The runner-up is **N8_arc240**. N8_ring becomes the preset's geometry,
**as a fallback, not as a geometry that meets the SAR criterion**.

---

## 1. The table (`ca_tbl_full`, 10 seeds each; `static_868` beside it)

| candidate | availability full / 868 | tFirst100m median (s) | CEP50 / CEP95 full (m) | CEP50 / CEP95 868 (m) | err ≤ 25 m (full) |
|---|---|---|---|---|---|
| N3_star | 0.245 / 0.280 | 8 | 17.7 / 43.0 | 151.7 / 256.7 | 0.685 |
| N3_cluster | 0.224 / 0.263 | 8 | 18.3 / 44.4 | 157.3 / 306.8 | 0.675 |
| N4_arc90 | 0.273 / 0.312 | 8 | 16.7 / 41.0 | 27.6 / 48.2 | 0.713 |
| N4_star | 0.250 / 0.283 | 8 | 57.4 / 93.4 | 35.7 / 60.7 | 0.618 |
| N5_arc90 | 0.291 / 0.342 | 6 | 16.0 / 37.0 | 27.5 / 52.4 | 0.766 |
| N5_arc140 | 0.292 / 0.343 | 6 | 19.0 / 49.7 | 22.3 / 42.2 | 0.708 |
| N6_arc180 | 0.306 / 0.351 | 5 | 16.6 / 39.7 | 16.3 / 33.0 | 0.722 |
| N6_arc240 | 0.306 / 0.356 | 8 | 14.8 / 36.0 | 15.2 / 29.9 | 0.764 |
| N8_arc240 | 0.322 / 0.377 | 8 | 13.6 / 31.3 | 14.4 / 27.6 | 0.798 |
| **N8_ring** | **0.329** / 0.384 | 8 | 14.5 / 29.8 | 12.9 / 22.1 | 0.808 |

Availability is the mean over the five subjects of `fixCycles / steps`. N8_ring's per-seed
range is 0.29-0.37. The time bound (median `tFirst100mS` ≤ 600 s) is met by every
candidate, with no run censored. **Availability is the whole of the failure.** First UWB
contact never occurs (1 800 s in every cell), as MS-3 found.

## 2. Predictions

- **P1 held.** The best candidate's availability rises with N: 0.245, 0.273, 0.292, 0.306,
  0.329 at N = 3, 4, 5, 6, 8.
- **P2 held.** No N = 3 candidate meets the criterion.
- **P3 failed.** Tag 0's error is within 25 m on 62-81 % of its cycles on `ca_tbl_full`,
  not under half. The prediction leaned on the absence of UWB. But the metric is tag 0's
  error over all its cycles, and tag 0 fixes on nearly every cycle (709 of 720 in N8_ring
  seed 1), at 868 MHz and 2.4 GHz ranges good to tens of metres. R ≤ 25 m is still not a
  claim about the subjects: availability says they mostly have no fix at all.
- **P4 failed.** `static_868` has higher availability than `ca_tbl_full` on every
  candidate, by 0.03-0.06. Tag 0 is not the cause (709 against 693 fixes in N8_ring
  seed 1, the full arm ahead); subjects 2-4 are. In that cell subject 3 escalates to 2.4
  GHz 29 times and then spends most cycles there with 0-1 anchors in reach. The 868 MHz
  floor needs three ranges, and the sparse cases (Case A, two ranges) run on the active
  tier only. On `static_868` the same subject stays on 868 MHz and takes 75 two-range
  fixes. At this geometry, escalating costs a subject the two-range fixes it would have
  had at 868 MHz: a relative of TECH_DEBT N.14, recorded there.

## 3. What else the table says (descriptive, not a result)

- At N = 3, `ca_tbl_full`'s CEP is an order of magnitude better than `static_868`'s
  (CEP50 18 against 152-157 m). From N = 6 the two converge. The full arm's advantage is
  at sparse geometry, where its 2.4 GHz and sparse fixes matter.
- Subject energy is about 14 J on the full arm against 8 J on `static_868` over 1 800 s.
  That is the price of the escalations and the preset's traffic.

## 4. What follows

- The preset's geometry becomes `--nGateways=8` (ring, default `--gwRingFrac`), in its own
  commit, labelled as the fallback.
- Stage 3: N8_ring and N8_arc240 are re-run under the full preset inside CAMP-SPEC (cells
  A and C). A flip in their order is reported, not re-tuned.
- SAR availability at 5 km is far from the spec's A ≥ 0.95 with 8 gateways or fewer. No
  shape in this family reaches it. Meeting it needs either more infrastructure or the
  subjects' own mesh carrying fixes (the routing and relaying whose populations G24 shows
  thin at this scale). That is a finding for the owner, not something tuned here.
