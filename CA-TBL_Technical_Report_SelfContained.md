# CA-TBL — Confidence-Aware Tier-Based Localization

**A simulation study of when a tiered radio architecture pays, and when it does not.**

> Here is precisely where tiering pays, where it does not, and why — established under
> pre-registration, on reproducible provenance, with the design defects the method
> caught along the way.

Every figure below cites a results directory whose `run_id.txt` records
`dirty 0` — a clean tree at a named commit — with digests verified against the files
on disk. **One exception exists and is named in §9.**

---

## 1. Executive summary

**What was built.** A three-tier GPS-denied localization stack for search and rescue,
simulated in ns-3.47: 868 MHz LoRa (RSSI-inversion ranging), 2.4 GHz (two-way
time-of-flight), and UWB (two-way ranging), with an EKF, a confidence score (PCS)
driving tier escalation, and a Mahalanobis integrity gate rejecting suspect fixes.
Six arms, twenty seeds, a pre-registered requirement of **availability ≥ 0.95 at
accuracy ≤ 25 m**, and a decision rule fixed before any run.

**Results that stand.**

| result | figure | qualification | source |
|---|---|---|---|
| Tiering buys reliability in a degraded environment | `ca_tbl_full` **19/20** vs `static_2p4` **0/20** deployments, at **1.56×** the energy | **No verdict attached** — no reliability criterion was registered, and inventing one after seeing the result is what pre-registration prevents. §4.1 | `results/p10_grid` |
| ~~Tiering wins on reliability~~ | ~~19/20 vs 0/20~~ | **DID NOT REPRODUCE.** On 52 fresh prospectively-registered seeds both arms meet the requirement **52/52**; the advantage was a one-seed margin in a 20-seed set. Paired availability delta falls to **+0.001629 [+0.000251, +0.003008]** | `experiments/JOINT_CRITERION_RESULT.md` |
| ~~One configuration in 320 is cheaper *and* equally reliable~~ | ~~M1 **0.9531** [0.9296, 0.9775]~~ | **WITHDRAWN (W11).** Measured under `nlosSource=legacy`; the same cell re-measures to **M1 = 1.1873 [1.1782, 1.1965]** — a FAIL — under the corrected source, and **no cell in the re-scored grid returns a PASS**. Claim 3 stands withdrawn with no replacement | `experiments/CLAIM3_NLOS_SOURCE_RESULT.md`, `experiments/P10_GRID_PROPAGATION_RESULT.md` |
| The dimensionless axis transfers | window held at `spacing/r80` 1.02–1.23 across a **26%** change in `r80` | A result about method; independent of any claim passing. §4.3 | `results/p10_env_v7`, `_v8` |

**Results falsified.**

- **The integrity gate does not pay** (Claim 9) — all four response policies lose
  availability against no gate at all, in eight of eight cells. Narrower than it
  sounds; §5.1.
- **The confidence model does not beat a flat threshold** (Claim 8) — 19/20 vs 18/20
  at 24.26 J vs 22.77 J.
- **The original operating window is withdrawn** (Claim 3) — it was inferred from a
  region where every pre-registered verdict was FAIL or INCONCLUSIVE.
- **Packet-level physics realism is re-scoped, not proven** (Claim 7) — the cited
  checks verify ranging precision and energy accounting, and a six-decibel sensitivity
  defect passed all of them. §5.4b.

**Defects the method found.** A six-decibel sensitivity-table indexing error on the
tier carrying 95% of active cycles; PCS confidence saturating so it cannot express a
diverged filter; a gate suite that overwrote the evidence it verified; a degeneracy
screen that conflated four mechanisms into one verdict. Two were found by checks
failing, two by results being arithmetically impossible. §6.

**The structural finding.** The project registered a **cost** criterion for an
architecture whose value proposition is **reliability**, and the two select for
opposite conditions — a cost win needs a qualifying competitor, a reliability win
needs the competitor to fail. Demonstrated, not argued: the screening rule and the
verdict rule carried disjoint cell sets, and all ten cells where the rule could
adjudicate sat in the *easiest* environment — the one where tiering is least needed.
§7.

---

## 2. The architecture

Design intent, in the past tense. Nothing in this section is a claim.

**Three tiers.** Tier 1, 868 MHz LoRa (SX1262-class), SF7, 125 kHz, 14 dBm — wide-area
coverage, ranging by inverting RSSI through a path-loss model. Tier 2, 2.4 GHz
(SX1280-class), SF7, 800 kHz, 12.5 dBm — mid-range refinement by symmetric two-way
time-of-flight; measured `r80` **1149.5 m**. Tier 3, UWB (DW3000-class), 499.2 MHz at
7.99 GHz, −16.47 dBm — precision two-way ranging at facilities, measured `r80` **12.8 m**
(corrected 2026-10-14 from a stale "≈ 13.5 m": recomputed from `results/probe_v7` as the distance
at which PDR falls through 0.80, source `fbf2153 dirty 0`, which also reproduces the 1515.3 m and
1149.5 m above).

**The estimator.** A four-state EKF (position, velocity) with sequential scalar
updates, process noise 1.0 m/cycle in position, fed by multilateration over the
active tier's anchors.

**The confidence score (PCS).** An unweighted sum of three terms, each bounded in
`[0,1]`: filter uncertainty `traceRef/(traceRef + traceSigma)`, geometry
`1/max(gdop,1)`, and line-of-sight fraction `nLos/nTotal`. Escalation was intended to
fire when PCS fell below a per-tier threshold — 1.55 on tier 1.

**The integrity gate.** Each candidate fix was tested against the EKF prior by
Mahalanobis distance, `D² = dᵀ(P_prior + R_fix)⁻¹d`, against χ²₀.₉₉ = 9.21 at 2 DOF. A
fix exceeding it on an all-line-of-sight cycle was marked *suspect* and rejected. Four
response policies were built: `none` (reject only), `inflate`, `bounded`, `reinit`.

---

## 3. Method

**This section leads because it is the strongest part of the work.**

**Pre-registration.** Every campaign's predictions, falsifiers and abort conditions
were written to a `PREDICTION.md` and committed **before the first cell ran**;
directory timestamps are checkable against the commit. The decision rule — three
outcomes, with "no competitor meets the requirement" deliberately INCONCLUSIVE rather
than PASS — was fixed in advance and is applied verbatim, including where it returns
no verdict at all.

**The gate suite, G1–G10.** Build, determinism, null ablation, verifier, verifier
self-test, tracked-evidence-versus-fresh-run, tree-clean-at-exit,
must-stay-untracked, deliverables-tracked-and-committed, and cited-paths-resolve.

**Every gate has a negative fixture, built in `/tmp`, and every one has been observed
to fail.** A suite that has only ever passed proves nothing about what it would catch.
G6 in particular was re-verified *after* being repointed at new evidence, because a
gate that turns green by being repointed is indistinguishable from one defanged by it:
it fails on mutated evidence, and still distinguishes "the tracked evidence disagrees"
from "the fresh run errored".

The suite also runs **every** gate and reports a summary rather than exiting on the
first failure. It used to exit early, and while G6 was deliberately red, G7–G9 could
not be observed to run at all.

**Provenance.** Every run writes `run_id.txt` recording the commit, the count of
modified tracked files under the source tree, the build profile, and a SHA-256 digest
of each output file — written after the last stream closes. `dirty 0` means the
binary is rebuildable from a named commit.

**Clone reproduction.** A fresh clone builds `optimized` and reproduces
`results/baseline_v7/arm_summary.csv` **byte-for-byte** using the flags recorded in
that baseline's own sidecar. Default flags do **not** reproduce it — a property worth
stating, because it reads as a reproduction failure when it is a flag-set difference.

**Mechanism-aware degeneracy screening.** One seed is run and screened before twenty
are spent. "Degenerate" is meaningful only relative to a named mechanism — `tiering`,
`integrity`, `trigger`, `energy` — and the tool refuses a single verdict without one.
Over 3205 directories: `tiering` 1.1%, `integrity` 27.3%, `trigger` 40.0%,
`energy` 0.1%.

---

## 4. Results that stand

### 4.1 Tiering buys reliability in a degraded environment

Source: `results/p10_grid`, `source 0a489df dirty 0`, 20 seeds, digests verified.

At `envFacility = RURAL_FOREST`, `nlosSource = legacy`, `dwell = 800`,
`boxHalf = 2000`:

| arm | deployments meeting (A ≥ 0.95, R ≤ 25 m) | energy |
|---|---|---|
| `ca_tbl_full` | **19 / 20** | 1.56× |
| `static_2p4` | **0 / 20** | 1.00× |

Tier 2 fails completely in forest at that spacing; the tiered arm survives. The paired
per-seed difference peaks at **+0.95** — near-perfect separation.

**No verdict is attached to this.** The project registered a decision rule for *cost*,
not for reliability, and inventing a reliability criterion after seeing which cells
would pass it is precisely what pre-registration exists to prevent. Whether +19
deployments for 1.56× the energy is worth buying is a deployment decision, not a
simulation result.

### 4.2 One configuration in 320 is cheaper and equally reliable

Source: `results/p10_grid/c_ef0_legacy_dw800_b1500`, `source 0a489df dirty 0`,
20 seeds.

At `RURAL_OPEN`, `legacy`, `dwell = 800`, `boxHalf = 1500`, both `ca_tbl_full` and
`static_2p4` meet the requirement on 20/20 seeds, and:

> **WITHDRAWN (W11), 2026-09-04.** The passage below stands as written; a withdrawal that edits its
> own subject leaves nothing to check. The cell was measured under `nlosSource=legacy`, and the same
> cell with only that flag changed gives **M1 = 1.1873 [1.1782, 1.1965]** — a FAIL. `static_2p4` is
> unchanged to three decimals in every energy component, because it has no tier 3; `ca_tbl_full`
> moves 14.523 → 18.039 J. The mechanism: under `legacy` the node parks on UWB for 553 of 1440
> cycles and stops paying the tier-1 floor. **The saving was the floor being switched off, not a
> cheap precision tier.**

> **M1 = 0.9531, 95% CI [0.9296, 0.9775]** — geometric mean of per-seed ratios with a
> seed-level bootstrap over 10 000 resamples. **Not** a ratio of means.
> `ca_tbl_full` 14.523 J against `static_2p4` 15.201 J: **4.5% cheaper, equally
> reliable.**

This is the **only PASS in 320 cells** under the registered rule, and the first in the
project's history. **Five caveats travel with it and none is optional:**

1. **A single isolated cell of 320.** A point, not a region.
2. **A 4.5% margin.**
3. **At the `dwell` level added to the grid after the preceding stage reported** —
   disclosed in the pre-registration, and load-bearing: the same cell at `dwell = 90`
   is 1.1571.
4. **Its neighbours are on the wrong side** — 1.0878 at `boxHalf` 2000, 1.1980 at
   2500. And **all ten adjudicable cells were `RURAL_OPEN`**: only there does a
   single-band arm also qualify, giving the rule something to compare against. The
   cost claim is decidable **only where tiering is least needed**.
5. **The filter's calibration in this cell is imperfect** — NEES 2.95 at 20 seeds
   against a 2-DOF expectation of 2.0, median 0.58, 3.3% of cycles above the 95%
   bound, concentrated in UWB. The PASS is adjudicated on ground-truth error and
   energy, neither of which reads the covariance, so its arithmetic is unaffected —
   but the tier decisions leading to it were taken on a confidence score derived from
   that covariance, and should not be described as confidence-driven in the sense the
   architecture claims.

### 4.3 The dimensionless axis transfers

Sources: `results/p10_env_v7`, `results/p10_env_v8`.

Correcting the tier-2 sensitivity defect moved `r80` from 909.5 m to **1149.5 m**, a
26% change. The region where `static_2p4` loses availability while the tiered arms
retain it moved **~29% in physical units** — `boxHalf` ~1935–2515 to 2500–3000 — and
stayed at **`spacing/r80` 1.02–1.23** in dimensionless units.

The axis was chosen to be scale-free and it survived a 26% change in its own
denominator. This is a result about **method**, and it does not depend on any claim
passing.

---

## 5. Results that do not stand

### 5.1 Claim 9 — the integrity gate does not pay, and the statement is narrower than it sounds

Sources: `results/c9_clean` (`source cd594c9 dirty 0`, 40 cells, 120 digest entries
verified) and `results/abl_clean` (`source ccaedb0 dirty 0`, 120 cells, 360 entries
verified).

Availability, `ca_tbl_full − ca_tbl_no_integrity`. Negative means the gate **loses**.
Estimator: mean of per-seed differences over 20 seeds, sign test beside it.

| policy | F1 | sign | F3 | sign |
|---|---|---|---|---|
| `none` | −0.0305 | 1/18/1 | −0.0540 | 1/19/0 |
| `inflate` | −0.0053 | 7/13/0 | −0.0085 | 3/17/0 |
| `bounded` | −0.0152 | 5/15/0 | −0.0136 | 2/18/0 |
| `reinit` | −0.0164 | 5/15/0 | −0.0172 | 2/18/0 |

**Negative in the mean in all eight cells**, with the sign test running against the
policy in all eight. Detection itself is sound — the gate fires 120–206 times per run
and the arms differ in every cell — but rejecting a fix subtracts availability without
supplying a better one.

The ordering `inflate < bounded ≈ reinit < none` in loss, roughly a 6× spread, is
reported as a **measured value with no mechanism claimed**. It is consistent with a
bounded response subtracting less than an unbounded rejection; no measurement
establishes that, and it is not asserted.

**What was actually falsified, stated carefully.** Both campaigns ran
`suspectForcesEscalation = false`. That flag removes a documented side effect under
which a suspect fix *also* forces an escalation. So these campaigns isolate
**rejection alone**: detect a bad fix, discard it, and do nothing else.

**The combined detect-and-escalate mechanism was not falsified.** Under
propagation-derived NLOS it moves `cep50M` from **92.15 m to 3.10 m**. The correct
statement is *rejection without escalation does not pay*, not *the integrity check
does not work*. The difference between those two sentences is the difference between
an accurate result and an overclaimed negative.

### 5.2 Claim 8 — the confidence model does not beat a flat threshold

`ca_tbl_full` 19/20 deployments at 24.26 J against `fixed_threshold` 18/20 at 22.77 J.
The flat variance threshold escalates on nearly the same occasions. Falsified thinly,
and reported as thin.

### 5.3 Claim 3's original window — withdrawn

The ledger read *PROVEN, SCOPED — window 1.0–1.3*. Every pre-registered verdict in the
campaign that window was drawn from was **FAIL or INCONCLUSIVE**, and the INCONCLUSIVE
entries read *"`ca_tbl_full` does not meet (A,R)"*. A window was inferred from a region
in which the proposed system did not meet its own requirement. Withdrawn with its
reason; replaced by §4.2.

### 5.4 Two withdrawn tables, both pre-D1

**The 19/20-vs-14/20 deployment-reliability table** was measured with tier 2 six
decibels deaf. Regenerated (`results/p10_plane_v7`, `source 63bba4f dirty 0`):
`ca_tbl_full` 19/20 → **20/20**, `static_2p4` 14/20 → **19/20**. A five-seed
reliability gap became one seed, at 2.1× the energy, and `static_2p4` also wins the
energy metric — 12.679 J against 27.094 J. **The argument does not survive.**

**The integrity ablation** (positive 4, negative 1, tied 15) is withdrawn as **pre-D1,
not as wrong**. It reproduces exactly from its cited directory — reproducibility
established provenance, never validity. The mechanism: the two arms differ only at the
gate, whose trigger reads `nNlos`, and **tier-2 occupancy in that campaign is 99.6%**
(27 322 cycles against 108 on tier 1), so `nNlos` and the residuals come almost
entirely from the links the six-decibel error changes. Gate eligibility measured at
**151/28 718 = 0.53%** all-LOS fix cycles, yielding **9 suspects over 20 seeds**. A
sensitivity error that moves which anchors are heard moves `nNlos`, and it does not
cancel in the difference between the arms.

### 5.4b Claim 7 — packet-level physics realism, re-scoped from PROVEN

The ledger asserted *Physics realism on packets — PROVEN*, citing a CRLB ratio of
1.105, a UWB ranging floor of 0.154 m, and energy reconciled to ten significant
figures.

**Those three checks pass and are unchanged.** What they verify is **ranging
precision and energy accounting**. None of them reads the receiver sensitivity
tables — they measure ranging error and energy, both downstream of whether a packet
was received at all.

The six-decibel indexing error of §6.1 sat in those tables, on the tier carrying ~95%
of active cycles, and **passed every gate, verifier check and validation in the
project**. An unqualified fidelity claim cannot stand after a physics defect that the
fidelity checks did not catch.

Asked directly: **no check in the project would catch that defect today.** What found
it was a closed-form propagation model built to reproduce the engine and refusing to
be tuned when it could not — not a gate. The honest status is that packet-level
physics is **verified for ranging precision and energy accounting, and unverified for
receiver sensitivity**. §8.4 specifies the check that would close it.

### 5.5 PCS escalation under propagation NLOS

With PCS as the sole escalation trigger and the uncertainty term saturated,
**escalation cannot fire however large the error becomes.** Measured: `term1` 0.0006,
`term2 + term3` = 1.9173 against a threshold of 1.55, **0 escalations**, 1440/1440
cycles on tier 1, `cep50` 89.91 m — with **NEES 1.95**, a correctly calibrated filter
whose confidence metric cannot express its own uncertainty. Control: the same
configuration under `legacy` NLOS has `term3` 0.1932, sum 1.1932, and escalation
fires.

**Three qualifications narrow this, and the registered prediction was a miss:**

- The both-hold fraction spans **0.054 to 0.998** across ten populations rather than
  clustering as predicted.
- **Registered prediction 1 is a MISS.** The discriminating variable is the position
  error, not the LOS term: saturation is common where error is already 68–90 m and
  rare where it is 2.9–4.3 m. Saturation is **coupled to divergence rather than
  independently causing it**.
- In **nine of ten populations the suspect path supplies escalation**, so PCS's
  contribution is unobservable there. Only one population isolates PCS as the sole
  trigger, and it is the only one with zero escalations.

---

## 6. Defects the method found

### 6.1 A six-decibel sensitivity-table indexing error

`GetSensitivityThreshold` computed `sf - 7` against tables whose own headers declare
`index 0 = SF5`. At the configured SF7 the 2.4 GHz engine returned **−100.0 dBm**, the
SF5 cell, instead of −106.0 — on the tier carrying ~95% of active cycles. Indices 6
and 7 were unreachable.

**Found because a result was impossible.** A closed-form propagation model built to
reproduce the engine implied `P(deliver | NLOS)` **below zero** at 1500 m. Four
mechanisms were proposed and refuted before the model itself was found to be feeding
aggregate columns into a per-link formula; correcting that left a residual that
resolved to the table index. **No gate would have caught it** — the fidelity checks
verify ranging precision and energy accounting, both downstream of whether a packet
was received at all.

Effect: tier-2 `r80` **909.5 m → 1149.5 m**. Every result predating it was regenerated.

### 6.2 PCS unweighted-sum term saturation

Three terms each bounded by `[0,1]`, summed without weights against a threshold of
1.55. The uncertainty term tends to 0 as the covariance grows, so once it saturates,
geometry and LOS fraction alone hold PCS above threshold regardless of how uncertain
the filter is. It can fail to contribute; it can never veto.

**Found because a result was impossible**: 361 suspects and 0 escalations in the same
run, with a filter calibrated at NEES 1.95. Recorded, **not fixed** — a fix changes the
engine and invalidates the results tree.

### 6.3 The gate suite overwrote the evidence it verified

The suite regenerated three tracked result directories **in place**, silently, and
re-dated their sidecars to the last gate run rather than the run that produced the
cited numbers. A verification step that rewrites its own subject cannot establish
whether the subject was correct before it ran.

**Found because a check failed** — a diagnostic batch compared the tree at start and
end and caught six modified files. Fixed: regeneration goes to a temporary directory,
tracked copies are read-only, and a new gate fails the suite if it exits with a
modified tree.

### 6.4 The degeneracy screen conflated four mechanisms

The screen returned one verdict per run, derived mostly from tier occupancy. It failed
a valid pre-flight in which the integrity gate fired 86 times and the arms differed in
37 fields, and it flipped verdict on **1.4 percentage points** of that incidental
quantity — 97.8% passed, 99.2% failed, same configuration and flags.

**Found because a check failed on a campaign that was demonstrably live.** Fixed to
report per mechanism.

**And one of the corrected rules was wrong.** The energy rule flagged *any* non-zero
draw on a zero-cycle tier and reported **66.5%** of the corpus degenerate. An
unoccupied tier still draws CAD/idle listening energy — `static_2p4` draws 0.014 J on
868 MHz and 0.008 J on UWB while occupying neither — which is correct behaviour.
Requiring a material share (>1% of total, a threshold of ours and not physical) gives
**0.1%**. Caught before publication; recorded because a methodology section that
reports its own screen's false-positive rate is more credible than one that does not.

---

## 7. The structural finding

The project registered a **cost** criterion — PASS requires the tiered arm's energy to
be lower than a qualifying competitor's, with a bootstrap CI entirely below 1.0 — for
an architecture whose value proposition is **reliability**.

The two select for opposite conditions:

```
screening rule   selects cells where the single-band arm FAILS
verdict rule     requires the single-band arm to QUALIFY
                 (a cost win needs a competitor in the race; "no competitor meets
                  the requirement" is INCONCLUSIVE by the rule's own text)
```

**Demonstrated, not argued.** A pre-registered 320-cell grid was run; its screening
rule and its verdict rule carried **disjoint cell sets**, and the single cell that
passed sat in a combination the screen had rejected. All ten cells where the rule could
adjudicate at all were `RURAL_OPEN` — the easiest environment, the one where tiering is
least needed — because only there does a single-band arm also qualify.

The registered metric cannot express the claim the architecture makes. At
`spacing/r80` = 1.02 the tiered arm meets the requirement in **82% more deployments**
for **27% more energy**; M1 asks only which qualifying arm is cheapest, and reliability
enters solely through a binary qualifier. That limitation was named in the
pre-registration *before* the campaign ran, which is what makes it an observation about
the metric rather than an excuse constructed after an unwelcome result.

---

## 8. What a corrected study would register

Specified, untested, with the measured motivation attached to each.

1. **A joint criterion over availability and energy**, with the trade-off stated in
   advance — for example a maximum acceptable energy multiplier per additional
   deployment met — rather than one axis binarised into a qualifier. Motivation: §7,
   and the 82%-more-deployments-for-27%-more-energy cell the current metric scores as
   FAIL.
2. **A weighted or gated PCS.** A weighted sum, a `term1` gate that vetoes when the
   covariance term saturates, or `min` instead of sum. Motivation: §6.2 — all three are
   plausible and **none has been run**.
3. **Mechanism-aware degeneracy screening as a mandatory one-seed pre-flight.**
   Motivation: §6.4, and the 27.3% / 40.0% of arm-cells that cannot speak to integrity
   or trigger claims respectively.
4. **A tier-2 fidelity check that reads the sensitivity tables.** Motivation: §6.1 — no
   check in the project would catch that defect today.

---

## 9. Limitations

**The one qualified citation.** `[M-24]` — integrity detection separation ratios, the
F0 suspect rate, and detection latency — rests on `results/campaign_20260810/stage1`,
which records `dirty 4`. It could not be repointed: at the time no injection campaign
anywhere recorded `dirty 0`, and the two campaigns that now do
(`c9_clean`, `abl_clean`) do not re-measure those particular statistics. Closing it
would require a fifth campaign of roughly 3.8 hours. **Every other number published as
a result traces to a `dirty 0` source.**

**Single-scenario.** One mobility model, one deployment geometry family, one seed
family. The environment sweep covers four `envFacility` levels and two NLOS sources;
it does not cover terrain, weather, or node mobility patterns beyond a facility tour.

**Simulation, not hardware.** Radio parameters come from datasheets with cited
`file:line` provenance, but no measurement campaign validates the engines against real
SX1262/SX1280/DW3000 hardware.

**PCS is recorded as defective and left unfixed.** §6.2's saturation is a live defect
in the shipped configuration. Fixing it changes the engine and invalidates every
result in this report, so it is recorded rather than corrected.

**The reliability result in §4.1 has no verdict** because no reliability criterion was
registered. It is reported as a measurement, and a reader who wants a verdict must
supply the criterion — which is exactly the gap §8.1 exists to close.

**Screened-out evidence.** 27.3% of arm-cells cannot speak to integrity claims and
40.0% have no trigger population. Those runs are not defective; they were not designed
to exercise those mechanisms. Reading a number off them would be reading a number with
no population.
