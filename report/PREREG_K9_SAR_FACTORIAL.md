# PREREG K9 — the SAR factorial

**Registered before the cells run.** Rule 13.

---

## 1. The design

Two factors, six cells, **20 seeds each** (7–26) as four processes of five. 4 simulated
hours, because the 1-hour runs ended at roughly the moment the searchers arrived — the
final approach, where tiers 2 and 3 pay, happened after the simulation stopped.

| | WAN only | MESH only | WAN + MESH |
|---|---|---|---|
| **lost node MOVING** | `k9_wanonly_move` | `k9_meshonly_move` | `k9_wanmesh_move` |
| **lost node STILL** | `k9_wanonly_still` | `k9_meshonly_still` | `k9_wanmesh_still` |

- **WAN only** — `--nFinders=0`. Fixed gateways at the camp and nothing else.
- **MESH only** — `--gwGrid=1` plus four finders. **One gateway cannot trilaterate**, so
  any fix in this cell is necessarily mesh-derived. That is the cleanest definition
  available without a "no gateways" flag, and it is stated rather than implied.
- **WAN + MESH** — both.

**868-only, 2.4-only and UWB-only fall out as arms inside every cell**, along with
`fixed_threshold` and `ca_tbl_full`/`no_integrity`, so the radio comparison and the
connectivity comparison are both covered without extra runs.

**The scenario:** camp at 0.9·6000 m, tour sites 3.6–11.6 km from it against a measured
868 r80 of **1515 m**, `--subjectKeepOutM=3000`. The lost node **starts at the camp** —
last known position — and walks out of coverage and stays out. Every previous campaign put
it inside the gateway footprint; this is the first that does not.

## 2. The metrics, and the one that is missing

Reported per arm per cell, all from `arm_summary.csv`:

| SAR question | column |
|---|---|
| how wrong is the position | `rmseM`, `cep50M`, **`cep95M`**, `maxErrM` |
| can it produce a position at all | `fixCycles`, `noFixCycles` / 5760 |
| what did it cost | `totalJ`, and per tier `j_868` / `j_2p4` / `j_uwb` |
| accuracy per unit of battery — the SWAP number | `accuracyPerJoule` |
| how hard the policy worked | `escalations`, `cycles_868/2p4/uwb` |

**`cep95M` is the operationally honest accuracy figure** — the radius you must sweep to be
95% sure of containing the subject — and it is reported ahead of `rmseM` for that reason.

**TIME TO CONVERGE IS NOT MEASURED AND IS THE ONE THE OWNER ASKED FOR.**
`arm_summary.csv` carries no per-cycle timing, so first-fix latency and time-to-convergence
need either traces or a new scalar column. A new column changes the CSV schema and would
require regenerating the v10 gate references, which does not fit before the deadline.
**It is owed, it is the next thing built, and nothing below should be read as answering
it.** `fixCycles` is availability, not latency, and the two are not the same.

## 3. Predictions

| # | prediction | how it fails |
|---|---|---|
| E1 | In **WAN only**, every arm is effectively blind — under 20% fix cycles — because the subject is 3.6–11.6 km from the only infrastructure | any arm exceeds 20% |
| E2 | **MESH only beats WAN only** on `cep95M` for `ca_tbl_full` — four mobile anchors that come to you beat fixed ones that cannot | it does not |
| E3 | **WAN + MESH ≥ MESH only**, and the margin is small, because the gateways are mostly out of range and contribute little | WAN+MESH is materially worse |
| E4 | `static_2p4` is the **worst** arm in every cell with finders — its 156 m reach cannot be met by a searcher who does not already know where to walk | it is not the worst |
| E5 | **STILL beats MOVING** on `cep95M` in every connectivity condition — a stationary subject is an easier target and the gap quantifies what mobility costs the searchers | still is not better |

## 4. Falsifier

**F1.** If `ca_tbl_full` fails to beat `static_868` on `cep95M` in the **MESH only** and
**WAN + MESH** cells, then the tiering does not pay even in the scenario built specifically
to let it — and the conclusion is that the value is in the mesh (mobile anchors) rather
than in the tier switching. That distinction is the point of the factorial and it will be
reported either way.

## 5. Abort conditions

120 rows per cell after concatenation, counted (rule 12); all four shards of a cell at one
commit; all `dirty 0`; new paths only. Every cell on the same binary, so cross-cell
comparisons inside K9 carry no cross-binary caveat.

**Twenty seeds per cell with a paired sign test, because the owner asked for confidence
that the result is not noise, and a single seed has never been a result in this project
(rule 8).**
