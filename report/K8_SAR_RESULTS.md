# K8 — the base-camp scenario. The verdict flips, and the margin is 0.6%.

Registered `report/PREREG_K8_SAR.md` (`ba2c003`) before the cells ran. 20 seeds each,
120 rows, single commit, `dirty 0`. Both defect fixes on: `--mahalUseR=true` and the
rebuilt `--escalationRequiresEvidenceT2=true`.

## The result

### `k8_camp_box3000` — camp at (2700, 2700), casualty 1824–5813 m out

| arm | rmse | energy | fix cycles | escalations |
|---|---|---|---|---|
| **`ca_tbl_full`** | 437.1 m | **13.116 J** | 955 | **3** |
| `static_868` | 501.0 m | 13.359 J | 970 | 0 |
| `static_2p4` | 1174.5 m | 35.135 J | **8 / 1440** | 0 |
| `fixed_threshold` | 469.6 m | 13.240 J | 970 | 4 |
| `ca_tbl_no_integrity` | 469.4 m | 13.240 J | 966 | 2 |

**`ca_tbl_full` is NON-DOMINATED by every claim arm — the first time in this project.**
Formally it *dominates* all four.

### `k8_camp_box5000`

Every policy arm is **identical**: rmse 1648.834, 204 fix cycles, 0 escalations, energy
within 0.008 J. `ca_tbl_full` *is* `static_868` here, to the digit.

## What held, and what the numbers actually say

| # | prediction | outcome |
|---|---|---|
| C1 | `static_2p4` under 10% fix cycles | **HELD** — 0.6% at box3000, **0%** at box5000 |
| C2 | non-dominated in at least one cell | **HELD** — dominates all four at box3000 |
| C3 | beats `static_868` on accuracy at significance | **FAILED** — +2.34 m, 12+/7−, p 0.36 |
| C4 | without spending more energy | **HELD** — +0.082 J in its favour, 14+/4−, p 0.031 |
| C5 | the win is the integrity check, not the tiering | **NEITHER** — both gaps are inside noise |

**The domination is real and it is tiny.** It rests entirely on **0.082 J out of 13.1 J —
0.6%** — because accuracy against `static_868` is statistically indistinguishable
(p 0.36). The median column showing 437 m against 501 m is misleading: the arms are
bimodal across seeds, and the paired difference is the reliable statistic.

**Escalations: 3 of 1440 at box3000, 0 at box5000.** The tier switching is essentially
never invoked. What the system is doing is *correctly declining to escalate* into a tier
that cannot reach — which is the tier-2 predictor working exactly as designed, and which
is where the 0.6% comes from. **It is winning by not wasting energy, not by locating
better.**

So the honest headline is not "the architecture pays". It is: **in a search, the
architecture stops hurting.** That is a real change from every previous campaign, where it
cost 2× the energy for worse accuracy — but it is not the claim the project set out to
make.

## Why the tiering never fires, and it is probably the run length

`static_2p4` gets 8 fix cycles of 1440, so tier 2 is available on **0.6% of the run**. Four
finders walk from the camp at 1.2 m/s; the casualty's near corner is 1824 m away, so a
finder needs ~25 minutes just to reach the nearest point of a tour the casualty is moving
around. **A 3600 s run ends at roughly the moment the searchers arrive.**

The part of a search where tier 2 and tier 3 pay — the final approach, closing from
hundreds of metres to metres — is **after the simulation stops**. That is the most likely
reason tier 3 has obtained zero fixes in every cell of every campaign, and it is a scenario
parameter rather than a defect.

**The next run should be hours, not one hour.** That is the single change most likely to
show the mechanism doing what it was built for.

## What this cell still does not include

Stated so the result is not read as "everything on":

- **Foliage depth is still unclamped** — tier 3 carries ~12.8 dB it should not
  (`FOLIAGE_DEPTH_DEFECT.md`). Tier 3 is crippled in this cell.
- **Relayed ranging is off.** Finders act as direct one-hop anchors, which is on; a *second*
  hop through a peer is not.
- **Gateways are not edge servers**, tier 1 does not estimate time-of-arrival, the IMU is
  off, and hop provenance is inert.
- **Availability is the stand-in for the metric that matters.** Time-to-first-fix and
  terminal accuracy in the final approach are still unmeasured, and in a search they are
  the whole point.
