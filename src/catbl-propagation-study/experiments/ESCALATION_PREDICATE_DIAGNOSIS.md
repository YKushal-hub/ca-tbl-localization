# Why tier 3 is never selected under the corrected NLOS source

Four results arrive at one observation — Claim 1's withdrawal (`tp = 0`, `fp = 0`), the
tier-occupancy census (≤ 5.7% fix share), W11 (0 UWB cycles), and Claim 9's falsification. This
decomposes the predicate that produces it. Measured on `results/claim2_rebase`, 20 seeds per cell,
`dirty 0`, existing traces — no new campaign.

## 1. The predicate as implemented

Read from source, not from documentation. Node on tier 2, `MODE_FULL`:

| # | gate | `file:line` | condition |
|---|---|---|---|
| 1 | **PCS** | `ca-tbl-policy.cc:1127` | `wantEscalate = (pcs < pcsThresholdUp[TIER_2P4])`, threshold **1.70** |
| — | PCS form | `:679-685` | `pcs = term1 + term2 + term3`, unweighted |
| — | `term1` | `:679` | `traceRef / (traceRef + traceSigma)`, `traceRef = 10.0` |
| — | `term2` | `:681` | `1.0 / max(gdop, 1.0)` |
| — | `term3` | `:683` | `nLos / nTotal` |
| 2 | suspect override | `:1128-1140` | `suspectForcesEscalation` — **off by default, inert** |
| 3 | **evidence: quality** | `:1190-1194` | blocked if `sqrt(traceSigma) > 0.25 × 20.0 = 5.0 m` |
| 4 | **evidence: coverage** | `:1197-1215` | blocked if fewer than **3** UWB anchors within **20 m** of the *EKF estimate* |
| 5 | floor bypass | `:1220-1232` | if the floor supplied the fix, the PCS decision is bypassed entirely |

Gates 3 and 4 apply only under `escalationRequiresEvidence=true`, and can only *suppress*.

## 2. Each gate measured, on cycles where the node is on tier 2

| | **propagation** | **legacy** |
|---|---|---|
| tier-2 cycles | 19 188 | 27 305 |
| `term3 < 0.567` (LOS term small) | **0.01%** | 93.90% |
| **`term2 + term3 ≥ 1.70`** (PCS cannot fall below threshold) | **85.18%** | 0.81% |
| **PCS permits** (`pcs < 1.70`) | **11.09%** | 76.23% |
| …and passes the quality gate | **0.61%** | 23.85% |
| …and passes the coverage gate — **escalation fires** | **0.00%** | 0.17% |
| floor supplied the fix (PCS bypassed) | 0.72% | 0.77% |

**Three gates in series, each removing an order of magnitude, and the third is absolute.**

**The floor bypass does not bind** (0.72%) — it was a candidate and it is measured out.

## 3. Which candidate: **B**, and both of its sub-mechanisms bind

**Not candidate A.** The policy is not correctly declining to escalate because tier 2 suffices. It
is structurally unable to request escalation, and then structurally unable to act on the request.

**B-via-D5 binds.** Under `propagation` the NLOS fraction is honest and near zero, so
`term3 = nLos/nTotal ≈ 1.0`; with `gdop ≤ 1` on most cycles, `term2 ≈ 1.0`. **`term2 + term3 ≥ 1.70`
on 85.18% of cycles — PCS exceeds its threshold before `term1` contributes anything.** This is D5,
PCS term saturation, already recorded, and the corrected NLOS source is what activates it: under
`legacy` the same sum exceeds 1.70 on only **0.81%** of cycles, because the defect inflated `nNlos`
and pushed `term3` down.

> **The confidence model only ever requested escalation because a defect was inflating its NLOS
> term.** With honest NLOS reporting it is arithmetically incapable of falling below threshold on
> 85% of cycles.

**B-via-evidence-gate also binds**, and independently. Of the 11.09% where PCS does permit, the
quality gate removes 94.5%, and the coverage gate removes **all** of the remainder.

## 4. The deeper cause: the predicate is self-defeating on a co-located deployment

The coverage gate reaching exactly zero is not bad luck. Splitting tier-2 cycles by whether UWB
coverage is available at all:

| | n | mean PCS | PCS permits | mean σ |
|---|---|---|---|---|
| **UWB coverage available** (≥3 anchors within 20 m) | 1 014 | **2.517** | **0.10%** | 2.84 m |
| no UWB coverage | 18 174 | 2.344 | **11.70%** | 3.66 m |

**Where the node can escalate, it does not want to. Where it wants to, it cannot.**

The cause is in the deployment, and it is deliberate. `ca_tbl_causal_sim.cc:277-281`:

> *"Co-location is deliberate. If tier-2 and tier-3 coverage were in different places, an escalation
> failure could be a siting artefact rather than a decision error…"*

Tier-2 and tier-3 anchors sit at the same four facilities. So:

- PCS falls when the fix is poor;
- the fix is poor when the node is far from tier-2 anchors;
- tier-3 anchors are **at the same places** as tier-2 anchors;
- therefore *far from tier-2* implies *far from tier-3*, i.e. no UWB coverage.

**The condition that triggers escalation and the condition that permits it are anti-correlated by
construction.** The precision tier is reachable exactly where it is least needed.

This is stronger than either sub-mechanism: fixing D5's saturation alone would raise the PCS-permit
rate, and the coverage gate would still reject those cycles, because they are the far-from-facility
cycles by definition.

## 5. Consequence

**For the ledger.** Every claim about escalation, confidence, integrity or precision-tier energy
measured under `propagation` was measured on a policy that cannot act. This is the shared cause of
Claim 1's withdrawal (W12) and it is consistent with Claim 9's falsification — the switching seam
the integrity check protects barely exists because the switch cannot throw. **It does not reopen
Claim 9**, which was falsified on its own evidence; it explains it.

**For B2.** The target question is settled mechanically rather than empirically. Calibrating tier-3
anchors serves cycles the policy cannot reach — and the reason is not that tier 3 is unnecessary
(candidate A) but that it is unreachable by a predicate anti-correlated with its own coverage. **B2
targets tier 2.** Should the predicate be repaired, tier-3 occupancy changes and the question
reopens; that dependency is now explicit rather than assumed.

## 6. What is not concluded

That the architecture is wrong. A three-tier design with co-located anchors and a
confidence-triggered escalation is coherent; what is measured here is that *this* predicate on
*this* deployment cannot express it. Whether a repaired predicate — a weighted PCS, a lower
threshold, or non-co-located siting — would let tier 3 serve is **untested**, and it is the obvious
next experiment.
