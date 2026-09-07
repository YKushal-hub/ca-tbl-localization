# PREREG K1 — does CA-TBL dominate the single-band arms and the fixed threshold?

**Registered before the campaign runs.** Rule 13. The commit that adds this file must
pre-date every directory this campaign writes, and the directory timestamps must be
checkable against it.

---

## 0. Why this campaign exists, and what is already known against it

The owner's requirement is stated plainly: the system must outperform every single-band
configuration and the fixed-threshold decision rule. **At the one operating point that
has been measured on the corrected topology, it does not.** `results/baseline_v8`,
seed 7, `boxHalf` at its 1000 m default:

| arm | rmse (m) | total (J) |
|---|---|---|
| `static_868` | 118.79 | 13.608 |
| `static_2p4` | **2.008** | **13.845** |
| `static_uwb` | 503.78 | 872.35 |
| `fixed_threshold` | 3.293 | 21.806 |
| `ca_tbl_no_integrity` | 101.12 | 14.375 |
| `ca_tbl_full` | 2.821 | 26.661 |

`static_2p4` is better on **both** axes than `ca_tbl_full` — 0.81 m more accurate for
12.8 J less. That is domination in the strict sense, and it is the thing this campaign
has to either overturn or report as standing.

Three facts already on the record say the measured point is not a fair test of the
architecture, and each is a reason to re-measure rather than to accept the number:

1. **The decision layer has never been run in a working state.**
   `report/ROOTCAUSE_ENVELOPE.md` Q2 is a DEFECT verdict: `--pcsForm` defaults to
   `legacy`, which saturates, and `report/PCS_FORMS_RESULTS.md` measured `gated` as
   the repair that buys the accuracy for a fifth of the energy of the alternatives.
   `baseline_v8` was run at `--pcsForm=legacy`. Every comparison above is therefore a
   test of a known-degenerate score.
2. **The tier-1 floor is 54.5% of `ca_tbl_full`'s energy** (`ROOTCAUSE_ENVELOPE.md`
   Q1, ARCHITECTURE verdict), and `--floorOnDemand` — the mechanism that makes the
   floor exchange conditional — is **off by default**. `ca_tbl_full` is paying a
   near-complete tier-1 bill on cycles where tier 1 supplies nothing.
3. **`boxHalf = 1000` is inside the regime where tiering is unnecessary by
   construction.** The casualty is confined to ±0.2·`boxHalf` of the centre and a
   3×3 gateway grid puts one gateway at the origin, so tier 2 reaches on essentially
   every cycle (`static_2p4`: 1440/1440 fix cycles). A tiering policy cannot beat a
   single band in a deployment that single band already covers. The brief specifies
   3 gateways over 5 km, which is a different regime entirely.

**None of this is a licence to tune until the answer is the wanted one.** The
predictions below are committed in advance, including the prediction that CA-TBL loses
at small scale, and the falsifier that fails the whole claim is stated as plainly as
the rest.

---

## 1. The question, stated so it can come out either way

For a deployment scale *s*, over a fixed seed set:

> Is `ca_tbl_full` **non-dominated** by every arm in
> {`static_868`, `static_2p4`, `static_uwb`, `fixed_threshold`}?

**Domination, defined before the data.** Arm *A* dominates arm *B* at scale *s* iff,
over the paired seed set, *A* is no worse on both axes and strictly better on at least
one, where:

- **accuracy axis** = `rmseM`; **energy axis** = `totalJ`;
- "no worse" = the paired per-seed difference does not favour *B* at a two-sided sign
  test, p < 0.05;
- "strictly better" = the paired difference favours *A* at that same test.

Both axes are compared **paired by seed**, never as pooled means. Rule 8 — a
single-seed cross-arm difference is never a result — applies to every number this
campaign produces, including the six in §0, which are seed 7 alone and are quoted here
as the motivation, not as a finding.

An arm that never obtains a fix is not thereby "energy-efficient". `static_uwb` reaches
0 fix cycles at 872 J, so it is dominated on both axes by construction and is carried
through the analysis only as a floor.

---

## 2. Cells

Common to all: `--duration=3600`, `--seedCount=20` (seeds 1–20), `--trace=false`,
`--nlosSource=propagation`.

`--trace=false` because the domination test reads `arm_summary.csv` only, and 100
run-directories of traces is ~13 GB that `EXCLUDED.md` forbids tracking anyway. Any
cell that later needs per-cycle attribution is re-run with tracing to a new path.

### K1a — the envelope, repaired decision layer

`--gwGrid=3 --pcsForm=gated --floorOnDemand=true`, `--boxHalf` ∈ {1000, 1500, 2000,
2500, 3000}.

Gateway spacing is 0.9·`boxHalf`, so this sweeps spacing/r80 from ≈0.6 to ≈1.8 against
the measured 868 r80 of 1515 m. **Gateway density falls as 1/`boxHalf`² and that is a
deliberate confound**: a search-and-rescue team does not acquire more gateways when the
search area grows, so holding density constant would answer a question nobody has. The
confound is named here so no reading of the result can mistake it for a pure coverage
axis.

### K1b — the same envelope, unrepaired

Identical to K1a but `--pcsForm=legacy --floorOnDemand=false` — i.e. `baseline_v8`
settings. This is the control that says what the two repairs bought, separately from
what the deployment scale bought. Without it, any improvement in K1a is unattributable.

### K1c — the brief's deployment

`--nGateways=3 --gwRingFrac=0.6 --boxHalf=2500 --pcsForm=gated --floorOnDemand=true`.

Three tri-band gateways in an equilateral ring over a 5 km box: the configuration the
context document actually specifies. K1a and K1b sweep a grid because the grid is what
the envelope work used and comparability matters; K1c is the one that answers "does it
work in the deployment we said we were building".

---

## 3. Predictions, committed in advance

| # | prediction | how it fails |
|---|---|---|
| P1 | At `boxHalf` = 1000, `static_2p4` dominates `ca_tbl_full` in K1a — the repairs do **not** rescue the small-scale case | `ca_tbl_full` non-dominated at 1000 |
| P2 | There is a crossover scale *s\** in [1500, 3000] above which `static_2p4` no longer dominates, because tier-2 coverage falls below three anchors | no such scale in range |
| P3 | `--floorOnDemand=true` cuts `ca_tbl_full` `totalJ` by ≥ 30% at every scale (K1a vs K1b) | < 30% at any scale |
| P4 | That energy cut costs < 1.0 m of `rmseM` at every scale | ≥ 1.0 m at any scale |
| P5 | `ca_tbl_full` dominates `fixed_threshold` at *s\** and above | dominated or incomparable there |
| P6 | `static_868` never dominates `ca_tbl_full` at any scale | it does, at any scale |

P1 is a prediction that the system **loses**. It is registered because the temptation
this campaign has to guard against is reporting only the scales where the answer is
pleasant.

---

## 4. Falsifiers — what would make the headline claim fail

- **F1.** If `ca_tbl_full` is dominated at **every** scale in K1a and in K1c, then the
  claim "CA-TBL outperforms every single-band arm and the fixed threshold" is **false
  as tested**, and must be reported as false. The permitted follow-up is to state the
  envelope in which it holds — or that there is none — not to widen the sweep until a
  winning cell appears.
- **F2.** If `--floorOnDemand=true` degrades `rmseM` by ≥ 1.0 m anywhere (P4 fails),
  the energy saving is a **trade, not a free win**, and every energy figure that uses
  it carries that trade in the same sentence.
- **F3.** If `static_2p4` is non-dominated at every scale tested, the three-tier
  premise does not pay in this deployment class, and that is the finding.

A partial result is a result: "non-dominated above spacing/r80 = X, dominated below"
is the expected shape of the answer and is reported as such, with X measured rather
than chosen.

---

## 5. Abort conditions — checked before any cell is analysed

- **A1.** The tree must reproduce `results/baseline_v8` byte-for-byte at the campaign
  commit, at that baseline's own recorded flags. If it does not, the campaign is void:
  no comparison to prior work survives a tree that has drifted.
- **A2.** Every cell must have **exactly 120 rows** of arm data — 20 seeds × 6 arms —
  counted, not tested for non-empty (rule 12; a `--seedCount` summary is appended per
  seed and is non-empty from seed 1). A cell short of 120 rows is **excluded by name**
  and reported as excluded, never silently pooled.
- **A3.** Every sidecar must record `dirty 0`. A cell produced from a modified tree is
  not citable and is excluded by name.
- **A4.** No tracked results directory is regenerated in place (rule 7). Every cell
  writes to a new path under `results/k1_*`.

---

## 6. Analysis, fixed before the data exists

1. Read `arm_summary.csv` per cell; assert the row count (A2) and the sidecar's
   `dirty 0` (A3) **before** any statistic is computed.
2. Per scale, per comparator arm, per axis: paired per-seed differences against
   `ca_tbl_full`, two-sided sign test.
3. Build the domination matrix from §1's definition. Report it in full — every arm at
   every scale, including the cells where CA-TBL loses.
4. Seed-level bootstrap, 10 000 resamples, for a 95% CI on the median paired
   difference on each axis at *s\**.
5. Report `cyclesInUwbCoverage`, `fixCycles` and `blindCycles` alongside, because a
   domination result driven by an arm failing to obtain any fix is a coverage
   statement wearing an accuracy statement's clothes.

**The verdict string from `pareto_sweep.py --analyse` is not used** and must not be
quoted in support of anything here: it compares the two decision rules to each other
and does not test domination (`RULES.md` §2).

---

## 7. What this campaign does not test

- Whether the confidence score carries information the EKF covariance does not. That
  question was answered — negatively — by `PCS_FORMS_RESULTS.md` §2 and is untouched
  here. Domination is a systems claim, not a claim for PCS.
- Anything about tier 1 as a time-of-arrival estimator (`TECH_DEBT.md` J.2).
- G11's model-versus-engine disagreement (`TECH_DEBT.md` J.1), which stays red.

---

# Amendment 1 — 2026-09-08, before any cell was analysed

Two launches were started and discarded before a single number was looked at. Both are
recorded here rather than quietly replaced, because a registration that only shows the
launch that worked is not a registration.

## A1.1 The first launch took the CLI defaults, and one of them is disqualifying

§2 named `--pcsForm`, `--floorOnDemand`, `--gwGrid` and `--boxHalf` and left everything
else at its default. Those defaults are a historical baseline, not the designed system.
Most of the gaps were merely unflattering; one was fatal:

**`nFinders` defaults to 0.** Its own help text calls finders *"the mechanism the
architecture turns on"* — UWB reaches tens of metres, so a fixed gateway can offer tier 3
only to something already beside it. With no finders, tier 3 has **no source**, every
escalation to it is pure loss, and the campaign would have measured a two-tier system
while reporting on a three-tier one.

Also defaulted off and part of the design: `--escalationRequiresEvidence`,
`--corroborationCheck` (the mechanism behind the strongest result in the repository),
`--bidirectionalDigest`, `--sendHandshakeFrames` (four other flags are silently
conditional on it — `TECH_DEBT` I.4), `--terrainPerTierFreq`, and
`--uwbCoverageIncludesFinders`.

**Change:** every cell now runs the designed deployment, identical across arms, so the
comparison is one physical world with different radio policies.

`--imuCrossCheck`, `--relayedRanging` and `--hopPenalty` stay **off**, each for a
measured reason and not to save time: the SHS driver does not track (330 m,
`DESIGN_DECISIONS` §8), and A3 admitted a relayed measurement on 2 cycles of 240, which
makes the provenance term inert rather than helpful.

## A1.2 The second launch was on the wrong propagation

A one-seed smoke test at `boxHalf=2000` showed `static_2p4` holding **10.46 mean
neighbours against tier 1's 9.71** — 2.4 GHz out-ranging 868 MHz on the same deployment,
against ~27 dB of link budget in tier 1's favour. The cause is
`report/ENV_SPLIT_DEFECT.md`: tiers 2 and 3 are simulated in `ENV_RURAL_OPEN` with
`VEG_SPARSE_SHRUBS` while tier 1 is in `ENV_RURAL_FORESTED` with `VEG_DENSE_FOREST`,
a leftover from the pre-2026-09-07 topology in which tiers 2/3 were separate anchors
"at a facility site". Under the tri-band correction they are the same nodes.

**A three-tier architecture whose second tier out-ranges its first cannot demonstrate
anything about tiering.** The domination question was unanswerable at that default, and
the answer it would have given — *use one radio* — is an artefact of a vegetation
constant.

**Change:** every cell now passes `--envFacility=1 --vegFacility=3`, putting all three
radios in the environment their shared node actually stands in.

## A1.3 The envelope moved, in both directions and for stated reasons

The original sweep was `boxHalf` ∈ {1000 … 3000}. It moved twice:

- **up**, when the smoke test at 2000 showed tier 2 still solving 1437 of 1440 cycles,
  putting the crossover outside the range;
- **back down**, when the environment correction collapsed tier 2's reach — 863 of 1440
  cycles at the same scale — putting the crossover *below* the original range instead.

Final sweep: **{500, 750, 1000, 1500, 2000, 3000}**, which spans tier 2 comfortable to
tier 2 hopeless at the corrected propagation. The range was set by where the tiers
change behaviour, and both moves are recorded above so that is checkable.

## A1.4 P3 is a joint effect, and is restated

K1a moves `--pcsForm` **and** `--floorOnDemand` together, so K1a − K1b is their joint
effect and P3, which names the floor alone, was not answerable as written. P3 now reads:

> **P3′.** The repaired configuration (gated PCS + floor on demand, together) cuts
> `ca_tbl_full` `totalJ` by ≥ 30% at every scale.

The attribution cell that would separate them is dropped for compute, and this is
recorded as a limitation rather than left for a reader to discover.

## A1.5 A new cell, to put 20 seeds under §A1.2

`k1e_split1000` — the K1a policy at `boxHalf=1000` with the environment split **left
at its defective default**. Against `k1a_box500…3000` it measures the defect over 20
paired seeds instead of the one seed that motivated the change. Registered now, before
either cell has run.

## A1.6 Two corrections to §2 as written

- Seeds are **7–26**, not 1–20: `seedStart` defaults to 7 and the campaign passes
  `--seedCount=20`. Every cell shares the same set, so the pairing is unaffected.
- §6.5 asks for `cyclesInUwbCoverage`. It is readable only because A1.1 turned
  `--uwbCoverageIncludesFinders` on; at the default the column is structurally zero and
  would have been quoted as evidence of something it cannot see.

**Nothing in this amendment was written after looking at a cell.** The abort conditions,
the domination definition, the predictions and the falsifiers of the original
registration are unchanged, P3 excepted, and F1 in particular still stands as written.
