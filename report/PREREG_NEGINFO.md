# PREREG NEGINFO — can negative information find the subjects that lock onto a wrong position? (N.57 / N.51)

**Registered before any cell runs.** Written 2026-10-21. This commit adds this file and `scripts/neginfo_run.sh`. The
instrument (`--traceNegInfo`, G162) and the scorer (`scripts/neginfo_analyse.py`) were committed before it. Outputs
go to `~/ns/catbl-work/neginfo_2026-10-21/`, which does not exist at registration.

## Why

N.51's per-range consistency read, against the effective sigma, under today's preset (exploratory,
`~/ns/catbl-work/n51_m1`):
- every measurement class is near consistent, E[z²] 1.0–2.4, **except relayed 2.4 GHz**;
- the relayed tail is one failure: in the endgame, 6–12 % of subject-runs lock 130–300 m off while claiming about 4 m
  (PRESET-2, ANCHOR-SUPPLY, FIDELITY-1b, read post hoc);
- their roundrobin finder steers on that wrong entry;
- with eight finders there were none.

N.57's remedy is negative information: a finder that should be heard but is not. An exploratory read of seeds
201–212 (`~/ns/catbl-work/neg_m1`, from an uncommitted tree, not citable) found it weak on both sides. This campaign
measures it from a clean tree, on fresh seeds, so that the decision it informs is citable.

## Cells

The endgame preset as it stands, with REAL-BEST's stack, `--ekf5Window=60` and `--traceNegInfo=true`; `ca_tbl_full`;
`--boxHalf=750 --finderMode=1`. Seeds 221–240, 1800 s: 20 cells.

## Predictions (from the disclosed exploratory read)

| # | prediction |
|---|---|
| N1 | subject side, the best-precision setting (D = 30 m, T = 10 s): recall below 5 % of wrong subject-cycles |
| N2 | subject side, every setting with recall above 2 %: precision below 0.75 |
| N3 | finder side: precision below 0.15 |

## Decision, stated now

- **If N1–N3 hold:** negative information, as the information this system's nodes hold, cannot drive a
  re-acquisition mechanism worth building. Its false fires on correct subjects match or exceed its true fires, and
  it misses at least 95 % of the failure.
- **Then N.51's wrong-root subjects are answered by deployment:** eight finders gave zero locks in ANCHOR-SUPPLY
  (O11). N.57 closes as a measured limit.
- **If any fails:** the mechanism is designed and registered on the setting that held.

## Abort conditions

- not 20 cells, or a cell's `rc` not 0;
- a sidecar not `dirty 0`, or more than one source;
- a manifest mismatch.

---

# RESULT — after the registration above (`3c4027ef`)

20 of 20 cells, every `rc` 0, every sidecar `source 3c4027ef dirty 0 profile optimized`. Scored once with
`scripts/neginfo_analyse.py`; the output is `results/neginfo_2026-10-21/NEGINFO.txt`.

**Population:** 57 600 subject-cycles; 3 457 wrong (> 50 m), 52 682 right (≤ 25 m).

| # | verdict | measured |
|---|---|---|
| N1 | HOLDS | D = 30 m, T = 10 s: fires on 37 / 3 457 wrong (1.1 %), precision 0.21 |
| N2 | HOLDS | recall > 2 % only at D = 60 m and D = 100 m with T = 10 s; precision 0.16 and 0.11 |
| N3 | HOLDS | finder side: 254 / 1 942 wrong finder-cycles, 3 236 right; precision 0.07 |

**Decision, as registered:**
- **Negative information cannot drive a re-acquisition mechanism worth building with the information these nodes
  hold.** Every setting fires on correct subjects more often than on wrong ones, and misses at least 96 % of the
  failure.
- **Why, from the trace:**
  - wrong subjects spend about half their cycles on the 868 MHz floor, with the precise radio off;
  - when on 2.4 GHz, the nearest finder their table places nearby is a median 77 m away;
  - finder entries are stale enough that a finder at a correct subject's entry often finds nobody within 30 m.
- **N.57 closes as a measured limit.**
- **N.51's remaining overconfidence**, the relayed-2.4 GHz tail, is the wrong-root subjects. Their answer is
  deployment: eight finders gave zero locks in ANCHOR-SUPPLY (owner decision O11).
