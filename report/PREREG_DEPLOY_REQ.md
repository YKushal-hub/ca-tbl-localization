# PREREG DEPLOY-REQ — what the brief's 5 km geometry needs to meet the requirement (plan item 26)

**Registered before any cell runs** (rule 13). Written 2026-10-21. This commit adds this file,
`scripts/deploy_req_run.sh` and `scripts/deploy_req_analyse.py`, and nothing else. Outputs go to
`~/ns/catbl-work/deploy_req_2026-10-21/`, which does not exist at registration.

## Why

Under the preset, the 5 km five-node pool reaches 0.83–0.84 against the requirement of A ≥ 0.95 at R ≤ 25 m.
DISSEM's addendum located the cause:
- **No node is within 1.5 km of a gateway.** The brief's three gateways sit on the west edge.
- **The finders cannot cover the rest.** 11 of 100 node-runs, a median 4.25 km from any gateway, carry two thirds
  of the shortfall.
- **The software levers are spent.** Every finder-side lever measured tonight moves the pool by a few hundredths.

Plan item 26 asks for this answer as a **deployment requirement** (MASTER_PLAN.md's framing): what gateway and finder
count meets the requirement. ANCHOR-SUPPLY ran 4 and 8 finders only. This campaign measures the grid.

## Cells

**The preset as it stands (`63cb93eb`), unchanged, at 5 km.** Only the deployment varies:
- **Gateways:** g3 is the brief's three; g8 and g12 are rings of 8 and 12 (`--nGateways=N --gwLayout=legacy`).
- **Finders:** 8 (the preset's), 12, 16 and 24 (`--nFinders`), tasked in pairs as the preset does.

All five arms; `--duration=1800 --trace=true --traceCovariance=true --reportAirtime=true`. Seeds 301–320, fresh:
3 × 4 × 20 = **240 cells**, one frozen binary checked before every cell.

## Predictions

| # | prediction | basis |
|---|---|---|
| E1 | g3 + 24 finders: CA-TBL five-node mean < 0.95 | finders cannot hold two precise anchors on every subject 1.5–5 km from any gateway; ANCHOR-SUPPLY 4 → 8 finders gave +0.17 at 5 km |
| E2 | g8 + 8 finders: CA-TBL five-node mean < 0.95 | the ring measured 0.909 (UWB-MODE) and 0.914 (DISSEM) |
| E3 | g12 + 16 finders: CA-TBL five-node and tag-0 means ≥ 0.95 | more of the box within a gateway's reach, and more finders for what remains |
| E4 | CA-TBL five-node A(25 m) rises from 8 to 24 finders at every gateway count: paired interval above 0 | anchor supply (N.62, ANCHOR-SUPPLY) |

## The answer (registered)

The deployment requirement is **every cell whose CA-TBL means reach 0.95 for tag 0 and for the five-node pool**. The
smallest has the fewest gateways, then the fewest finders. If no cell meets it, that is the answer. The preset is
not changed by this campaign: the brief fixes the deployment, and this measures what that deployment would have to
become.

## Abort conditions

- not 240 cells, or a cell's `rc` not 0;
- a sidecar not `dirty 0`, or more than one source;
- a manifest mismatch;
- a seed outside 301–320;
- a cell whose finder count or preset lines do not match;
- any window solver failure;
- an arm with fewer than 20 seeds.

---

# RESULT — after the registration above (`7ff477db`)

**Run.** Launched 2026-10-21T16:53:47Z from `7ff477db`, dirty 0. Done 2026-10-22T01:01:09Z. 240 of 240 cells, every
`rc` 0, one source, no window solver failure; **problems none**. Scored once
(`results/deploy_req_2026-10-21/DEPLOY_REQ.md`).

| # | verdict | measured (CA-TBL means, 20 seeds) |
|---|---|---|
| E1 | HOLDS | 3 gateways + 24 finders: five-node 0.936 |
| E2 | HOLDS | 8 gateways + 8 finders: five-node 0.916 |
| E3 | HOLDS | 12 gateways + 16 finders: five-node 0.970, tag 0 0.990 |
| E4 | HOLDS | five-node A(25 m), 24 − 8 finders: +0.143 (3 gateways), +0.066 (8), +0.061 (12); every interval above 0 |

## The deployment requirement

| gateways \ finders | 8 | 12 | 16 | 24 |
|---|---|---|---|---|
| **3** (the brief) | 0.956 / 0.794 | 0.985 / 0.827 | 0.988 / 0.900 | 0.992 / 0.936 |
| **8** (ring) | 0.986 / 0.916 | **0.987 / 0.955** | 0.988 / 0.941 | **0.990 / 0.982** |
| **12** (ring) | 0.985 / 0.912 | **0.988 / 0.975** | **0.990 / 0.970** | **0.992 / 0.972** |

Each cell is CA-TBL A(25 m), tag 0 / five-node; bold cells meet A ≥ 0.95 for both.

- **By the registered rule, the smallest deployment that meets the requirement is 8 gateways and 12 finders**
  (five-node 0.955; 95 % of runs ≥ 0.95).
- **The 8-gateway margin is thin.** 8 gateways with 16 finders gave 0.941, below 8 with 12. One 20-seed mean is not
  monotone in finders. With 8 gateways the requirement is met on the mean, not with room.
- **12 gateways meet it in every finder count tested from 12 up** (0.970–0.975; 95–100 % of runs). That is the
  deployment with a margin.
- **The brief's 3 gateways do not meet it at any finder count up to 24** (best 0.936). Finders alone cannot replace
  infrastructure within reach of the subjects.
- **Tag 0 meets it in every cell but one** (3 gateways, 8 finders: 0.956, which still meets it on the mean).

**Against the comparators**, CA-TBL − comparator, five-node, in the cells that meet the requirement:
- **`fixed_threshold`:** level everywhere (within ±0.02).
- **`poucet_fusion`:** CA-TBL above it at 8 + 12 (+0.033 [0.004, 0.068]), 8 + 24 (+0.021) and 12 + 12 (+0.050
  [0.014, 0.100]).
- **static 2.4 GHz:** CA-TBL above it at 8 + 24 (+0.022) and 12 + 12 (+0.050).

**What this changes.** The preset keeps the brief's deployment (3 gateways, 8 finders); the brief fixes it. The
requirement-level statement is now measured: *at 5 km, A ≥ 0.95 for all five nodes needs about 8 gateways and 12
finders, and 12 gateways for a margin*. The system meets the requirement in every geometry once that infrastructure
is present: the endgame and the ring already did, and with this grid the 5 km box does too.
