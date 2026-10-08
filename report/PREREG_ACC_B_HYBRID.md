# PREREG — S2 Part B: hybrid range supply (N.35 + N.38)

Exploratory grade. Committed before the first evaluation cell (seeds 1–20) of the candidate runs.
**Not blind:** the flags were chosen on calibration seeds 21–30 after diagnosis on seeds 1–20 of the
*reference* (unmodified preset), and an earlier version of N.35 alone was already run on seeds 1–20
(availability endgame +0.021 WIN, accuracy ties, 5 km energy +1.25 J LOSS). What is new here is the
combination with N.38 at k = 3, fixed before this evaluation.

## Candidate and reference

| | binary | flags |
| --- | --- | --- |
| reference | the merged tree before Part B (`100cc650`) | `--specProfile=true` |
| candidate | `1c1886c7` (G91 commit) | `--specProfile=true --dualBaseRelayTier=true --caseDInfoRatio=3` |

Null control before reading any number: the candidate binary with the flags off reproduces the reference's
outputs byte for byte (seed 1, A and B, arms `ca_tbl_full,fixed_threshold,static_2p4`).

## Cells

`--duration=1800 --trace=true`, seeds 1–20. A = 5 km (preset default); B = endgame
(`--boxHalf=750 --finderMode=1`). Arms `ca_tbl_full,fixed_threshold,static_2p4`, and separately
`poucet_fusion,lora_fusion`, each under both binaries. `--caseDInfoRatio` acts on every arm that runs
Case D, so every comparator is re-run under the candidate flags as well.

## Metrics (scoreboard of `plan_part_b_hybrid.md` §7)

Five-node fix availability; five-node within-25 m share; tag 0 within 25 m; tag 0 CEP50 and CEP95 (per seed);
subject J per node. Paired seed bootstrap (4 000 resamples, 95 %); WIN/LOSS only when the interval excludes 0.

## Predictions (candidate `ca_tbl_full` minus reference `ca_tbl_full`)

- **P1, availability:** WIN in both configurations.
- **P2, endgame accuracy:** CEP50 and CEP95 WIN.
- **P3, 5 km accuracy:** five-node within 25 m ≥ 0 and tag 0 CEP50 ≤ 0 (a tie is consistent).
- **P4, energy:** no LOSS greater than +1.5 J per node in either configuration.

## Falsifiers and abort

- Any accuracy LOSS (within 25 m, CEP50 or CEP95) in either configuration falsifies "improvement"; it is reported
  as such, not tuned away on these seeds.
- The null control failing aborts the evaluation.
- Comparator verdicts are read only within one binary (rule 8): candidate `ca_tbl_full` against the candidate
  comparators, reference against reference.
