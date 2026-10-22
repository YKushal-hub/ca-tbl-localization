# PREREG PRESET-5 — the preset after DESIGN_DECISIONS 26 against the brief's deployment, on fresh seeds

**Registered before any cell runs** (rule 13). Written 2026-10-22. This commit adds this file,
`scripts/preset5_run.sh` and `scripts/preset5_analyse.py`, and nothing else. Outputs go to
`~/ns/catbl-work/preset5_2026-10-22/`, which does not exist at registration.

## Why

DESIGN_DECISIONS 26 grew the preset's ring to 12 gateways (12 finders, pairs). PRESET-4 put DD 25's 8 gateways on
the line at 5 km (0.947 on fresh seeds). DEPLOY-REQ measured 12 gateways at 0.970–0.975 for 12 or more finders.
This is the confirmation on seeds no campaign has run: does the adopted preset meet A ≥ 0.95 at R ≤ 25 m for tag 0
and the five-node pool, at 5 km and in the endgame? No adoption rule: the decision is taken, and a failure is
reported to the owner.

## Cells

- **NEW:** the preset as it stands (`d4580fbb` + DD 26).
- **OLD:** the brief's deployment, `--nGateways=3 --gwLayout=star --nFinders=8`.

**Configurations:**
- A: 5 km.
- B: endgame, `--boxHalf=750 --finderMode=1`.

All five arms. Seeds 341–360: **80 cells**, one frozen binary.

## Predictions

| # | prediction | basis |
|---|---|---|
| P1 | NEW's CA-TBL five-node mean A(25 m) in A ≥ 0.95 | DEPLOY-REQ 12 gateways + 12 finders 0.975 |
| P2 | NEW raises CA-TBL five-node A(25 m) in A against OLD: interval above 0 | DEPLOY-REQ, PRESET-4 |
| P3 | NEW's CA-TBL five-node mean A(25 m) in B ≥ 0.95 | PRESET-4 endgame 0.993 at 8 gateways |
| P4 | NEW lowers CA-TBL five-node A(25 m) nowhere: lower bounds above −0.02 | |
| P5 | NEW's CA-TBL tag-0 mean A(25 m) ≥ 0.95 in A and in B | DEPLOY-REQ 0.988; PRESET-4 0.998 |

## Abort conditions

- not 80 cells, or a cell's `rc` not 0;
- a sidecar not `dirty 0`, or more than one source;
- a manifest mismatch;
- a seed outside 341–360;
- a finder count or preset lines not matching the variant;
- a window solver failure;
- an arm with fewer than 20 seeds.
