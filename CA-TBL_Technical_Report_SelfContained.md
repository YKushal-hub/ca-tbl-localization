# CA-TBL — Technical Report

**Confidence-Aware Tier-Based Localization for GPS-Denied Search and Rescue**

Status **2026-08-10**. Results from `results/p10_plane_fix/`, 20 seeds, source `a059203`,
build profile `optimized`, digest-verified provenance on every result directory.
Pre-registration `b49df46` (2026-08-09 22:35) and amendment `a059203` (2026-08-09 23:58),
both committed before any run they govern.

---

## 1. Summary

A search-and-rescue tag must be locatable to within an actionable distance, and it must be
locatable *reliably* — a system that finds the casualty most of the time is not a system
that finds the casualty. No single radio band satisfies both halves of that requirement:
long-range bands lack the precision, precise bands lack the range.

CA-TBL is a three-tier localization architecture that runs a long-range tier continuously
and escalates to a precision tier when position confidence indicates escalation is
warranted **and** evidence indicates the precision tier can actually deliver. This report
presents the measured performance of that architecture against three single-band controls
and one prior-art baseline.

### 1.1 Headline results

Requirement: a position fix accurate to **R = 25 m**, available **A = 95%** of the time.
Derivation in §4.2. Each of the 20 seeds is an independent deployment realization.

| arm | deployments meeting spec | availability | CEP50 | energy |
|---|---|---|---|---|
| **`ca_tbl_full`** (proposed) | **19 / 20** | **0.9917** | **2.69 m** | 24.26 J |
| `fixed_threshold` (prior art) | 18 / 20 | 0.9828 | 3.09 m | 22.77 J |
| `static_2p4` (2.4 GHz only) | 14 / 20 | 0.9426 | 2.73 m | 9.68 J |
| `static_868` (868 MHz only) | 0 / 20 | 0.0691 | 100.60 m | 14.81 J |
| `static_uwb` (UWB only) | 0 / 20 | 0.0551 | 0.16 m | 769.45 J |

Three findings, each established below:

1. **Trigger correctness (§5.1).** Evidence-gated escalation raises trigger precision from
   **0.078 to 0.934** and F1 from **0.144 to 0.803**. Escalations into geometrically
   unsolvable conditions fall from 21,116 to 90 across 20 runs.

2. **Energy (§5.2).** The same mechanism reduces energy from 622.92 J to **24.26 J**, a
   **25.7×** reduction, while *improving* both accuracy (CEP50 4.79 m → 2.69 m) and
   availability (0.9742 → 0.9917).

3. **Deployment reliability (§5.3).** The proposed architecture meets the joint
   requirement in **19 of 20 deployments**; the best single band meets it in **14 of 20**.
   In its worst deployment the single band leaves the tag unlocatable for **25.5%** of the
   mission; the proposed architecture, **9.0%**.

---

## 2. System architecture

### 2.1 Three tiers

| tier | radio | SF | bandwidth | TX power | measured r80 | ranging method |
|---|---|---|---|---|---|---|
| 1 | 868 MHz LoRa (SX1262) | 7 | 125 kHz | 14.0 dBm | **1515.3 m** | RSSI inversion |
| 2 | 2.4 GHz LoRa (SX1280) | 7 | 800 kHz | 12.5 dBm | **909.5 m** | two-way time-of-flight |
| 3 | UWB (DW3000) | 7 | 499.2 MHz | −16.47 dBm | **12.8 m** | two-way ranging |

`r80` is the distance at which packet delivery ratio falls to 0.80, measured from a swept
probe (`results/causal/probe_range_sweep.csv`) rather than assumed. The three tiers span
two orders of magnitude in range and three in precision — the spread that makes tiering
worth doing.

### 2.2 Energy model

Charged per radio action from datasheet currents at 3.3 V:

| tier | TX | RX | idle | sleep |
|---|---|---|---|---|
| 868 MHz | 45.0 mA | 5.3 mA | 1.5 mA | 0.0012 mA |
| 2.4 GHz | 24.0 mA | 7.0 mA | 1.4 mA | 0.0012 mA |
| UWB | 82.0 mA | **94.0 mA** | 12.0 mA | 0.0007 mA |

UWB receive current exceeds its transmit current — a real property of the DW3000 and the
reason a UWB-only system costs 769 J where 2.4 GHz costs 9.68 J. Receive-window time is
charged explicitly via `AccountListen`, not assumed free.

Tier 1 operates under a 2.5% duty-cycle limit (WPC G.S.R. 853(E) Table-II). A frame of
airtime *t* arms a lockout of *t*/0.025, so a 56.58 ms frame blocks the tier for 2.263 s.
Tiers 2 and 3 are unrestricted in this configuration.

### 2.3 State estimation

An extended Kalman filter with sequential scalar updates and per-measurement variance.
Position confidence is scored each cycle:

```
PCS = traceRef/(traceRef + traceSigma)  +  1/max(GDOP, 1)  +  nLOS/nTotal
```

with `traceRef = 10.0 m²`. The three terms capture posterior uncertainty, geometric
dilution, and line-of-sight quality respectively. Escalation thresholds are
1.55 / 1.70 / 2.30 per tier. Innovations are gated by Mahalanobis distance against
χ²₀.₉₉ = 9.21 at 2 degrees of freedom.

### 2.4 Evidence-gated escalation

The mechanism that produces the results in §5. Escalation from tier 2 to tier 3 requires
**both**:

1. **Confidence** — `PCS < pcsThresholdUp[tier]`, i.e. the current fix is not good enough.
2. **Evidence** — the node's own EKF estimate predicts at least 3 UWB anchors within
   `uwbPredictRadiusM = 20 m`, behind a quality gate that suppresses the prediction when
   `sqrt(traceSigma) > 0.25 × radius`.

The predicate is **suppress-only**: it is applied after the confidence test and can never
create an escalation that confidence did not request. The quality gate matters — a
position estimate too uncertain to be trusted must not be used to predict coverage, and
without the gate the predicate would propagate its own error.

The predicate's operating curve was measured against geometric ground truth at two
deployment scales:

| radius | boxHalf=1000 recall / precision | boxHalf=2000 recall / precision |
|---|---|---|
| 12.8 m | 0.283 / 1.000 | 0.219 / 1.000 |
| **20.0 m** | **0.843 / 1.000** | **0.854 / 0.988** |
| 30.0 m | 0.981 / 0.981 | 1.000 / 0.970 |

R = 20 m was selected because it holds across a doubling of deployment scale. The
reconstruction used to compute this curve was validated against each trace's own
visibility column — **0 mismatches in 1440 cycles at both geometries** — before any figure
was derived from it.

---

## 3. Seed generation and experimental control

Every comparison in this report is made across 20 independent seeds. This section states
exactly what a seed controls, because the strength of every claim depends on it.

### 3.1 Seeding mechanism

```
RngSeedManager::SetSeed(seed)              // the --seed argument, 1..20 contiguous
RngSeedManager::SetRun(1)                  // fixed
RngSeedManager::ResetNextStreamIndex()     // called at the head of every arm
```

`SetRun` is held constant; the seed is the only varied input.

### 3.2 Why `ResetNextStreamIndex()` is required

The physics engines construct random variable streams without explicit `SetStream()`
assignment, so each takes the next automatic stream index. `SetSeed()` does not reset that
counter — it is a separate call. Without an explicit reset, arm 6's engines begin drawing
where arm 5's stopped, and no two arms ever share a channel realization.

This was measured, not assumed. Two arms with identical EKF state and identical poll counts
reached **334 versus 336** engine evaluations by t = 2.5 s — the first cycle, before either
arm's policy had made a single decision. At t = 7.5 s the same six anchors returned 6
decodes on one arm and 2 on the other.

The reset is present in both `RunArm` and `RunProbe`. Its absence invalidated an earlier
generation of cross-arm results, which were discarded rather than corrected.

### 3.3 What a seed controls

| component | stochastic? | stream |
|---|---|---|
| gateway, anchor, facility placement | **no** — closed-form from geometry parameters | — |
| tag trajectory | yes | explicitly assigned stream |
| channel realization (shadowing, fading, Markov regime) | yes | engine-private, auto-indexed |
| NLOS state | yes | 4-state Markov chain within the engine |
| packet delivery | yes | engine draws |
| EKF process and measurement noise | **no** — `Q` and `R` are scaled analytically | — |

Deployment geometry is deterministic: at a given `boxHalf` and `gwGrid`, every seed places
every anchor identically. The seed varies the *channel*, the *trajectory*, and the
*propagation environment* — which is what makes each seed a distinct deployment
realization rather than a distinct network.

### 3.4 What is common across arms, and what is not

**Common at a given seed:** the deployment geometry, and the tag trajectory — every arm
walks an identical path.

**Not common:** the alignment of channel draws to individual links in time. All arms read
the same stream, but they consume it at arm-dependent rates, because which tier is polled
and how many anchors are addressed depend on the arm's own behaviour. Arm A's tenth draw
and arm B's tenth draw are the same number, but they are not applied to the same link at
the same instant.

The consequence is stated plainly because it governs how these results may be read: a
cross-arm difference at a single seed carries a channel-draw component. **The 20-seed sweep
is therefore the minimum evidence for any claim comparing two arms, not a refinement of a
single-seed result.** All comparisons in §5 are made across the full 20 seeds, and per-seed
distributions are reported rather than means alone.

### 3.5 Determinism

Two runs of the same binary at the same seed produce byte-identical output (gate G2). More
strongly, two **independently rebuilt** binaries produced byte-identical traces across all
six arms — establishing that the results depend on the source, not on the build.

---

## 4. Method

### 4.1 Arms

| arm | description |
|---|---|
| `static_868` | tier 1 only — maximum range control |
| `static_2p4` | tier 2 only — the strongest single-band control |
| `static_uwb` | tier 3 only — maximum precision control |
| `fixed_threshold` | prior art: escalate when `traceSigma > 100 m²` |
| `ca_tbl_no_integrity` | proposed architecture, integrity check disabled |
| `ca_tbl_full` | proposed architecture, complete |

### 4.2 The requirement, derived before measurement

**Accuracy R = 25 m.** A ground searcher dispatched to a reported position sweeps an
effective width *W* either side of their track; a fix is actionable if the true position
falls inside that swept area, giving `R = W/2`. With `W = 50 m`, `R = 25 m`.

**Availability A = 0.95.** A search coordinator tolerates the tag being unlocatable for at
most three minutes in any hour of operation: `A = 1 − 3/60 = 0.95`.

Both values were fixed in pre-registration `b49df46` before any arm was scored against
them.

### 4.3 Metric and its population

```
availability = (cycles with a fix AND error <= 25 m) / ALL cycles in the run
```

The denominator is **every cycle**, not every cycle that produced a fix. A cycle with no
fix counts against availability. This is the definition that makes the metric meaningful
for a rescue system: a fix that was never produced cannot guide a searcher, and a scoring
rule that ignores it would reward a system for declining to answer.

`CEP50` and `RMSE` are reported over fix-cycles only and are stated alongside availability,
never instead of it.

Run configuration: 3600 s at `dt = 2.5 s` = **1440 cycles per run**, `boxHalf = 2000 m`,
`gwGrid = 5`. Facility spacing is 940 m against a measured tier-2 r80 of 909.5 m — a
`spacing/r80` ratio of **1.03**, the regime in which tier-2 coverage is marginal and the
escalation decision therefore carries information.

---

## 5. Results

### 5.1 The escalation trigger fires correctly

Scored against geometric ground truth, using the simulator's own definition: the positive
class is `visUwb >= 3` (tier 3 is geometrically solvable), the prediction is
`activeTier == UWB`, and the population is all cycles. Pooled over 20 seeds, 28,800 cycles:

| | TP | FP | FN | precision | recall | **F1** |
|---|---|---|---|---|---|---|
| confidence only | 1,778 | 21,116 | 40 | 0.0777 | 0.9780 | **0.1439** |
| **confidence + evidence** | 1,280 | **90** | 538 | **0.9343** | 0.7041 | **0.8030** |

**Precision rises from 0.078 to 0.934 — a 12-fold improvement — and F1 by 5.6×.**
Escalations into conditions where the precision tier is geometrically incapable of
producing a fix fall from 21,116 to 90.

Recall falls from 0.978 to 0.704: the predicate declines 538 cycles in which tier 3 would
in fact have been solvable. This is the deliberate cost of the quality gate, which
suppresses escalation when the position estimate driving the prediction is itself too
uncertain to trust. The trade is strongly favourable — F1 is the correct summary because
both error classes carry cost, and it improves by a factor of 5.6.

The effect is consistent across seeds: F1 improves on 20 of 20.

### 5.2 Escalation efficiency and energy

| quantity | confidence only | confidence + evidence |
|---|---|---|
| time on tier 3 | 79.5% of cycles | **4.8%** |
| fixes sourced from tier 3 | 5.6% | 3.9% |
| **escalation efficiency** | **0.071** | **0.827** |
| escalations per run | 216.2 | 6.8 |
| CEP50 | 4.79 m | **2.69 m** |
| availability | 0.9742 | **0.9917** |
| **energy** | 622.92 J | **24.26 J** |

Escalation efficiency — the ratio of fixes obtained from a tier to time spent on it — rises
from 0.071 to 0.827. Under confidence-only escalation the system spent roughly 14 cycles on
the precision tier per fix it produced; with evidence gating, 1.2.

**Energy falls 25.7×, and accuracy and availability improve simultaneously.** These are not
in tension because they share a cause: time spent on a tier that cannot deliver is time not
spent on one that can. The system was paying UWB receive current — 94 mA, the highest in
the platform — while taking its fixes from the fallback tier.

Escalation is suppressed, not eliminated: 6.8 escalations per run survive, and 4.8% tier-3
occupancy against roughly 6.6% geometric availability confirms the architecture still
reaches the precision tier when the precision tier is present.

### 5.3 Deployment reliability

Each seed is an independent deployment realization: a distinct channel, trajectory and
propagation environment over identical infrastructure. The operational question is not what
a system achieves on average but **how often a deployment meets specification**, because a
rescue team cannot know in advance which deployment it has drawn.

| arm | deployments meeting spec | mean availability | worst deployment |
|---|---|---|---|
| **`ca_tbl_full`** | **19 / 20** | 0.9917 | **0.9097** |
| `ca_tbl_no_integrity` | 18 / 20 | 0.9903 | 0.9097 |
| `fixed_threshold` | 18 / 20 | 0.9828 | 0.8889 |
| `static_2p4` | 14 / 20 | 0.9426 | **0.7451** |
| `static_868` | 0 / 20 | 0.0691 | 0.0437 |
| `static_uwb` | 0 / 20 | 0.0551 | 0.0361 |

**The proposed architecture meets the requirement in 19 of 20 deployments; the best single
band in 14 of 20.** Six deployments in twenty — 30% — in which a 2.4 GHz-only system fails
to locate the tag as often as the mission requires.

The worst case is the sharper number. In its poorest deployment `static_2p4` leaves the tag
unlocatable for **25.5% of the mission**. The proposed architecture's poorest deployment
leaves it unlocatable for **9.0%**. Mean availability of 0.9426 obscures this entirely — it
reads as "close to 95%" while concealing that one deployment in five loses the tag for a
quarter of the operation.

**Both single-band extremes fail categorically, and for opposite reasons.** `static_868`
achieves 0.0% outage — it always produces a fix — but at 100.6 m CEP50, far outside
actionable range: **0/20**. `static_uwb` achieves 0.16 m CEP50, the best in the study, but
is geometrically unavailable 94.4% of the time: **0/20**. Range without precision and
precision without range each fail the same joint requirement. This is the argument for
tiering, measured rather than asserted.

### 5.4 Confidence scoring versus a flat variance threshold

The prior-art baseline escalates on a single quantity, `traceSigma > 100 m²`. The proposed
policy fuses posterior uncertainty, geometric dilution and line-of-sight quality.

| arm | deployments | availability | CEP50 | energy |
|---|---|---|---|---|
| `ca_tbl_full` | **19/20** | **0.9917** | **2.69 m** | 24.26 J |
| `fixed_threshold` | 18/20 | 0.9828 | 3.09 m | 22.77 J |

Paired per seed, `ca_tbl_full` achieves higher availability on **20 of 20 seeds — no ties,
no reversals** (mean margin +0.0089). A sign test on 20/20 gives p ≈ 1×10⁻⁶: the effect is
small in magnitude but perfectly consistent, which is the signature of a real mechanism
rather than noise.

The cost is 6.5% additional energy (24.26 J against 22.77 J) and the benefit is one
additional deployment meeting specification plus 0.4 m of median accuracy. The confidence
model earns its complexity, and the honest scale of that benefit is small.

---

## 6. Why this architecture performs better

Four mechanisms, each traceable to a measurement above.

**It is the only configuration that satisfies both halves of the requirement.** Tier 1
reaches 1515 m but resolves to 100 m; tier 3 resolves to 0.16 m but reaches 12.8 m. Neither
clears a joint availability-accuracy bar at any seed. Composing them under a policy does —
19/20 — because the tiers cover each other's failure mode rather than competing on a single
axis (§5.3).

**Escalation is conditioned on evidence, not on need alone.** A system that escalates
whenever its fix is poor will escalate hardest exactly where the precision tier is absent,
because poor fixes and sparse coverage share a cause. Requiring predicted coverage before
spending precision-tier energy raises trigger precision from 0.078 to 0.934 and cuts energy
25.7× while improving accuracy (§5.1, §5.2).

**It degrades rather than fails.** Every cycle retains a long-range fallback, so the
architecture's worst deployment holds 0.9097 availability where the single band drops to
0.7451. The failure mode is graceful loss of precision, not loss of the tag.

**Confidence fusion outperforms a single-variable trigger consistently.** 20 of 20 seeds,
no reversals, for 6.5% additional energy (§5.4).

---

## 7. Verification and reproducibility

Every number above is regenerable from the published repository.

- **Gate suite** `scripts/catbl_gates.sh`: G1–G5 all pass, suite exit 0. Determinism,
  ablation causality, and a verifier self-test of 11 cases.
- **Verifier** `verify_causal_sim.py`: 141 checks passed, 161 gates evaluated, all six arms
  audited.
- **Provenance.** Each result directory carries a `run_id.txt` recording the run identifier,
  the full invocation, the source commit, the working-tree dirty count, the build profile,
  and a SHA-256 digest of every file the run produced. The digest is written after the last
  output stream closes, so its presence is also a completeness signal — a run that died
  part-way leaves none.
- **Clone test.** A fresh clone of the published commit, built under
  `--build-profile=optimized`, reproduces `results/baseline_v6/arm_summary.csv`
  byte-for-byte at seed 7 with the flags recorded in that baseline's own sidecar.
- **Pre-registration.** Metric definitions, the requirement `(A, R)`, the testability
  predicate and the decision rule were committed at `b49df46` and `a059203`, both
  timestamped before every run they govern.

---

## 8. Limitations and future work

Each item below is a property of the evaluation scenario, not a result about the
architecture. They are stated with what would be required to resolve them.

### 8.1 Single tag

Every run carries one tag. Multi-target arbitration, channel contention under load, and the
duty-cycle regime's behaviour at scale are therefore untested. The tier-1 duty cycle arms a
2.263 s lockout per 56.58 ms frame, so contention is expected to become the binding
constraint well before the radio does — but with one tag there is no contention to measure.

*Required:* sweep 5, 10 and 20 tags, measuring offered load, collisions, capture events and
per-node lockout time.

### 8.2 Tiers are co-located by design

Tier-2 and tier-3 anchors ring the same four facility sites, at 150 m and 10 m
respectively. This was a deliberate experimental control — it ensures an escalation failure
is unambiguously a decision error rather than a siting artefact, which is what makes the
trigger-correctness result in §5.1 clean.

But it means the tiers fail together in the gaps between facilities, where only tier 1
reaches. The architecture's spatial argument — that tiers should be sited to cover each
other's gaps — is therefore untested, and any upper bound on the architecture's operating
range in this study is a property of co-located siting rather than of tiering.

*Required:* a complementary-siting variant run alongside co-located as a second treatment
arm, so the comparison isolates siting rather than confounding it.

### 8.3 One deployment density evaluated

Results are at `boxHalf = 2000 m`, `spacing/r80 = 1.03` — the regime where tier-2 coverage
is marginal. The architecture's behaviour across the density spectrum is not mapped.

*Required:* sweep `boxHalf ∈ {1500…6000}`, spanning `spacing/r80` from 0.78 to 3.10, and
report each arm's availability against that dimensionless axis. This yields a deployment
rule — the infrastructure density above which tiering becomes necessary — which transfers
to any radio pair rather than being specific to this study's hardware.

### 8.4 Cross-arm channel alignment

As stated in §3.4, arms consume the shared random stream at arm-dependent rates, so
per-seed cross-arm differences carry a channel-draw component. This raises the variance of
paired comparisons; it does not bias them, and all claims here rest on 20-seed aggregates
rather than single-seed differences.

*Required:* true common random numbers — binding streams to links rather than to draw
order — which would tighten every paired comparison in §5.

### 8.5 The integrity check

Disabling the integrity check costs one deployment (19/20 → 18/20) and 0.0013 mean
availability. Paired per seed the difference is positive on 4, negative on 1 and exactly
tied on 15 — too weak to attribute to the mechanism at this scale. The check's value, if
any, lies in fault conditions this scenario does not create.

*Required:* fault injection — anchor position error, spoofed ranges, systematic bias — under
which an integrity check has something to detect.

### 8.6 Tier-2 escalation is ungated

The evidence predicate governs the tier-2 → tier-3 step. The tier-1 → tier-2 step remains
ungated, because the tier-2 radio sleeps while another tier is active and its neighbour
table is consequently empty, making observed evidence unavailable from tier 1.

*Required:* a predicted-coverage gate for tier 2, symmetric with the tier-3 predictor, using
the same anchor map at a radius derived from the measured 909.5 m r80. The ratio is
favourable — 909.5 m against a tier-1 position error of 63–83 m is roughly 11:1, against
20 m versus the same error for the tier-3 case — so the quality gate that constrains the
tier-3 predictor would be satisfied almost everywhere.

---

## 9. Reproducing these results

```bash
./ns3 configure --enable-examples --build-profile=optimized && ./ns3 build

mkdir -p results/verify
./ns3 run "ca_tbl_causal_sim --seed=1 --duration=3600 --boxHalf=2000 --gwGrid=5 \
  --escalationRequiresEvidence=true --trace=true --outdir=results/verify"
```

`--build-profile=optimized` is required, not preferred: every figure in this report was
produced under it, and the flags above are recorded in each result directory's
`run_id.txt`. Repeat for seeds 1–20 to reproduce the tables in §5.
