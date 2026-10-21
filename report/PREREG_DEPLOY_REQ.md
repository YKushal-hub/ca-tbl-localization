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
