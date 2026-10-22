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

---

# RESULT — after the registration above (`617d06bc`)

**Run.** Launched 2026-10-22T07:04:17Z from `617d06bc`, dirty 0. Done 08:31:40Z. 80 of 80 cells, every `rc` 0, one
source, no window solver failure; **problems none**. Scored once (`results/preset4_2026-10-22/PRESET4.md`).

| # | verdict | measured (CA-TBL, 20 seeds) |
|---|---|---|
| P1 | **FAILS** | NEW five-node at 5 km **0.947** (80 % of runs ≥ 0.95) |
| P2 | HOLDS | NEW − OLD five-node at 5 km **+0.127 [0.064, 0.201]** |
| P3 | HOLDS | NEW five-node in the endgame **0.993** (every run ≥ 0.95) |
| P4 | HOLDS | NEW − OLD five-node: 5 km +0.127, endgame +0.016 [0.011, 0.021] |
| P5 | HOLDS | NEW tag 0: 5 km 0.989, endgame 0.998 |

## What it says

- **DD 25's deployment is a large improvement and sits on the line at 5 km.** Two independent 20-seed sets give
  0.955 (DEPLOY-REQ) and 0.947 (here). At 8 gateways and 12 finders the five-node pool meets A ≥ 0.95 on one set and
  misses by 0.003 on the other. DD 25 recorded that margin as thin, and it is.
- **The endgame improves and meets it in every run** (0.977 → 0.993), at a cost: subject energy +6.9 J
  [5.7, 8.1] (+40 %), from the four extra finders' ranging.
- **5 km tag-0 CEP95** falls from 7.0 to 4.1 m.
- **Against the comparators under NEW:**
  - level with `fixed_threshold`;
  - above Poucet fusion on the five-node pool, +0.029 at 5 km and +0.017 in the endgame, at about 1/30 of its
    subject energy;
  - level with static 2.4 GHz at 5 km, and above it in the endgame.

**The deployment that meets the requirement with a margin** is DEPLOY-REQ's 12 gateways (0.970–0.975 at 12 or more
finders) or 8 gateways with 24 finders (0.982). This is reported to the owner, who chose DD 25.
