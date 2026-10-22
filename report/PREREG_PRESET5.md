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

---

# RESULT — after the registration above (`8e92fe3a`)

**Run.** Launched 2026-10-22T15:37:24Z from `8e92fe3a`, dirty 0. Done 17:07:32Z. 80 of 80 cells, every `rc` 0, one
source, no window solver failure; **problems none**. Scored once (`results/preset5_2026-10-22/PRESET5.md`).

| # | verdict | measured (CA-TBL, 20 seeds) |
|---|---|---|
| P1 | HOLDS | NEW five-node at 5 km **0.952** (90 % of runs ≥ 0.95) |
| P2 | HOLDS | NEW − OLD five-node at 5 km **+0.077 [0.029, 0.123]** |
| P3 | HOLDS | NEW five-node in the endgame **0.993** (every run ≥ 0.95) |
| P4 | HOLDS | NEW − OLD five-node: 5 km +0.077, endgame +0.011 [0.007, 0.015] |
| P5 | HOLDS | NEW tag 0: 5 km 0.986, endgame 0.999 |

## The requirement under the adopted preset

| configuration | tag-0 A(25 m) | five-node A(25 m) | runs ≥ 0.95 (five-node) | CEP95 tag 0 | subject J |
|---|---|---|---|---|---|
| A, 5 km | **0.986** | **0.952** | 0.90 | 4.7 m | 13.3 |
| B, endgame | **0.999** | **0.993** | 1.00 | 1.4 m | 25.7 |

- **A ≥ 0.95 at R ≤ 25 m is met for tag 0 and for all five nodes, at 5 km and in the endgame,** on seeds no campaign
  had run.
- **The 5 km margin is modest.** The same deployment gave 0.975 on DEPLOY-REQ's seeds and 0.952 here. The requirement
  is met on both 20-seed sets; it is not met with a wide margin.
- **Energy.** Endgame subject J rises 7.8 J [6.6, 8.9] against the brief's deployment, from the extra finders. At
  5 km it is unchanged.
- **Against the comparators under the preset:**
  - level with `fixed_threshold`;
  - above Poucet fusion in the endgame (+0.011 [0.003, 0.025]) and level at 5 km, at about 1/30 of its subject energy;
  - level with static 2.4 GHz on availability.
