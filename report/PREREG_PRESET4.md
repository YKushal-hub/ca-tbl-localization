# PREREG PRESET-4 — the preset after DESIGN_DECISIONS 25, measured on fresh seeds

**Registered before any cell runs** (rule 13). Written 2026-10-22. This commit adds this file,
`scripts/preset4_run.sh` and `scripts/preset4_analyse.py`, and nothing else. Outputs go to
`~/ns/catbl-work/preset4_2026-10-22/`, which does not exist at registration.

## Why

DESIGN_DECISIONS 25 (owner decision, from DEPLOY-REQ) moved the preset from the brief's 3 gateways and 8 finders to
8 gateways (ring) and 12 finders, the smallest deployment that met A ≥ 0.95 at R ≤ 25 m at 5 km. Two things are not
yet measured:
- **The margin.** DEPLOY-REQ's 8-gateway result met the requirement on the mean (0.955), with a non-monotone
  neighbour (8 + 16 finders: 0.941).
- **The endgame.** DEPLOY-REQ ran 5 km only, and the preset change also moves the endgame.

This campaign measures both on fresh seeds. No adoption rule follows: the decision is taken. If a prediction fails,
it is reported as a property of the adopted preset, and the owner sees it.

## Cells

- **NEW:** the preset as it stands (`d4580fbb`).
- **OLD:** NEW with DD 25 pinned back: `--nGateways=3 --gwLayout=star --nFinders=8`.

**Configurations:**
- A: 5 km.
- B: endgame, `--boxHalf=750 --finderMode=1`.

All five arms; `--duration=1800 --trace=true --traceCovariance=true --reportAirtime=true`. Seeds 321–340, fresh:
2 × 2 × 20 = **80 cells**, one frozen binary checked before every cell.

## Predictions

| # | prediction | basis |
|---|---|---|
| P1 | NEW's CA-TBL five-node mean A(25 m) in A ≥ 0.95 | DEPLOY-REQ 0.955; a thin margin, so this may fail |
| P2 | NEW raises CA-TBL five-node A(25 m) in A against OLD: interval above 0 | DEPLOY-REQ 0.955 against 0.794 |
| P3 | NEW's CA-TBL five-node mean A(25 m) in B ≥ 0.95 | OLD met it (0.96–0.98); more infrastructure |
| P4 | NEW lowers CA-TBL five-node A(25 m) nowhere: lower bounds above −0.02 | |
| P5 | NEW's CA-TBL tag-0 mean A(25 m) ≥ 0.95 in A and in B | DEPLOY-REQ 0.987; the endgame 0.997 |

## Abort conditions

- not 80 cells, or a cell's `rc` not 0;
- a sidecar not `dirty 0`, or more than one source;
- a manifest mismatch;
- a seed outside 321–340;
- a cell whose finder count or preset lines do not match its variant;
- any window solver failure;
- an arm with fewer than 20 seeds.
