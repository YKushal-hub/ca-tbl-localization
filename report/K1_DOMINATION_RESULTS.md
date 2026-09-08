# K1 — does CA-TBL outperform a single band and a fixed threshold?

**Answer, on the registered criterion: no.** `ca_tbl_full` is dominated at every scale
measured and in the brief's own deployment. F1 of the registration names this outcome in
advance and names the response, which is to report it as false and state the envelope in
which it holds — or that there is none — rather than widen the sweep until a kinder cell
appears. **There is no such envelope inside the sweep.**

**Registered** `report/PREREG_K1_DOMINATION.md` + Amendments 1 and 2, all committed
before the cells they govern ran. **Campaign** 11 cells, 20 paired seeds (7–26), 3600 s,
every sidecar `dirty 0`, every cell 120 rows counted.

This is the first measurement of the system on a deployment the scenario still builds
**and** a link budget whose three tiers share an environment. Everything before it is
provisional for one reason or both.

---

## 0. Two link-budget defects had to be fixed before the question could be asked

### The environment split — `report/ENV_SPLIT_DEFECT.md`

Tiers 2 and 3 were simulated in `ENV_RURAL_OPEN` + `VEG_SPARSE_SHRUBS` while tier 1 sat
in `ENV_RURAL_FORESTED` + `VEG_DENSE_FOREST` — a relic of the pre-2026-09-07 model, in
which 2.4 GHz and UWB were separate anchors at facility sites. Under the tri-band
correction they are the same nodes, on the same masts, in the same canopy.

Measured by the probe, which needs no seeds and no policy:

| tier r80 | split (old default) | unified (corrected) |
|---|---|---|
| 868 MHz | 1515.3 m | 1515.3 m |
| 2.4 GHz | **1149.5 m** | **156.0 m** |
| UWB | 12.8 m | 0.0 m (r50 5.3 m) |

**2.4 GHz was getting 7.4× its true range and out-ranged 868 MHz** — against 17 dB more
receiver sensitivity, 8.8 dB less free-space loss and 1.5 dB more transmit power in tier
1's favour. A three-tier architecture whose second tier out-ranges its first has no
operating point at which the long-range tier is the only one that reaches, so it cannot
demonstrate anything about tiering. Every cell here passes `--envFacility=1
--vegFacility=3`.

### Foliage depth is not clamped to link length — `report/FOLIAGE_DEPTH_DEFECT.md`

Depth is drawn `LogNormal(ln 100, 0.6)` with no reference to the link, so a 10 m UWB hop
is charged 100 m of canopy: 13.77 dB where 0.98 dB is admissible. The error is **zero on
links longer than the draw and maximal on the shortest**, so it is invisible on tier 1
and worst on tier 3 — about 12.8 dB, a 3.6× range factor.

**Not fixed for this campaign, on purpose.** Two link-budget changes at once makes
neither attributable. **Every tier-3 number below is understated because of it.**

---

## 1. The registered result

Domination as defined in §1 of the registration: no worse on both `rmseM` and `totalJ`,
strictly better on one, by two-sided sign test on paired per-seed differences at α = 0.05.

| cell | vs `static_868` | vs `static_2p4` | vs `static_uwb` | vs `fixed_threshold` |
|---|---|---|---|---|
| `k1a_box750` | incomparable | **DOMINATED** | DOMINATES | tie |
| `k1a_box1000` | **DOMINATED** | **DOMINATED** | DOMINATES | tie |
| `k1a_box1500` | **DOMINATED** | **DOMINATED** | DOMINATES | **DOMINATED** |
| `k1a_box2000` | **DOMINATED** | **DOMINATED** | DOMINATES | **DOMINATED** |
| `k1a_box3000` | **DOMINATED** | **DOMINATED** | DOMINATES | **DOMINATED** |
| `k1c_brief` | **DOMINATED** | **DOMINATED** | DOMINATES | **DOMINATED** |

`static_2p4` dominates `ca_tbl_full` in **every cell of the campaign**, repaired and
unrepaired, at every scale. It beats the three-tier system on both registered axes
everywhere. That is the single most robust finding here and it is not the one the
project wanted.

`static_uwb` is dominated everywhere, obtains **zero** fix cycles in every cell, and is
carried only as a floor.

---

## 2. Where CA-TBL is not dominated, and how small that comfort is

At `boxHalf=750` — the smallest deployment measured — `ca_tbl_full` is **incomparable**
to `static_868`, which is the honest statement of a real win:

| arm | rmse | energy | fix cycles |
|---|---|---|---|
| `ca_tbl_full` | **25.47 m** | 16.07 J | 1395 / 1440 |
| `static_868` | 70.28 m | 15.04 J | 1440 / 1440 |

**45 m of accuracy for 1.04 J**, 18 of 20 seeds (p = 0.0004), CI [+39.5, +52.1] m. The
tiering does what it is for: when tier 2 is reachable, escalating to it is worth the
energy. That is the mechanism working.

It is small comfort because the same cell has `static_2p4` at **11.84 m for 10.17 J** —
better than CA-TBL on both axes, 19 of 20 seeds on accuracy and 20 of 20 on energy.

---

## 3. The coverage axis, which the registration required be reported

§6.5 required `fixCycles` alongside, "because a domination result driven by an arm
failing to obtain any fix is a coverage statement wearing an accuracy statement's
clothes." It is:

| cell | `ca_tbl_full` fixes | `static_2p4` fixes | `static_868` fixes |
|---|---|---|---|
| `k1a_box750` | 1395 (97%) | 1082 (75%) | 1440 (100%) |
| `k1a_box1000` | 1366 (95%) | 1010 (70%) | 1439 (100%) |
| `k1a_box1500` | 1358 (94%) | 924 (64%) | 1438 (100%) |
| `k1a_box2000` | 1294 (90%) | 785 (55%) | 1436 (100%) |
| `k1a_box3000` | 1242 (86%) | 570 (40%) | 1422 (99%) |
| `k1c_brief` | 1236 (86%) | 771 (54%) | 1400 (97%) |

`static_2p4` wins on the registered axes **while failing to produce a position on a
quarter to three-fifths of cycles**. Whether that is acceptable is an operational
question this campaign does not answer, and it is not a defence of the architecture —
it is a statement that the two registered axes do not capture everything a search system
is for.

**Extending domination to three axes** — accuracy, energy, coverage — is a *post-hoc*
analysis, not a registered test, and is labelled as such. Under it, `ca_tbl_full` is
non-dominated at `box750` and `box1000` and dominated from `box1500` upward. **The arm
that dominates it there is `fixed_threshold`, which is better on all three.** So the
extension rescues the claim against the single-band arms at small scale and does not
rescue it against the fixed threshold anywhere.

A caution on reading §1's median columns: at `box3000` `static_2p4`'s rmse distribution
is **bimodal** — it either holds a fix or goes blind and the filter diverges — so its
median jumps from 222.6 m to 999.3 m between two cells whose paired median difference is
35.7 m. The sign test is the reliable statistic there, and the verdict at that scale is
driven by energy (0+/20−), not accuracy (11+/9−, p = 0.82).

---

## 4. What the repaired configuration bought: about 95% of the energy

`k1a` and `k1b` differ by **five** flags, not the two P3 named — gated PCS, floor on
demand, the evidence gate, the corroboration witness, the bidirectional digest. This is
a bundle and is reported as one.

`ca_tbl_full`, paired by seed, positive = repaired better:

| `boxHalf` | energy repaired → unrepaired | median Δ | sign test | 95% CI |
|---|---|---|---|---|
| 750 | 16.07 → 345.69 J | **+329.8 J** | 20+/0− p<0.0001 | [+290.7, +352.9] |
| 1000 | 15.81 → 382.65 J | **+366.8 J** | 20+/0− p<0.0001 | [+348.9, +376.4] |
| 1500 | 15.94 → 320.99 J | **+305.5 J** | 20+/0− p<0.0001 | [+249.6, +386.8] |
| 2000 | 19.69 → 314.37 J | **+295.9 J** | 20+/0− p<0.0001 | [+263.7, +362.0] |
| 3000 | 20.69 → 159.23 J | **+132.3 J** | 20+/0− p<0.0001 | [+88.4, +176.4] |

**P3′ holds by a factor of three over its own threshold** — it predicted ≥30% and the
measurement is 83–96%. The static arms' energy moves by 0.06–0.23 J across the same two
cells, so this is roughly **1500× the RNG-displacement floor**.

**The mechanism is almost certainly the evidence gate.** In `k1b` the policy escalates
196–204 times and spends 300+ J; `static_uwb` costs ~890 J for full-time use, so the
excess is UWB. In `k1a` the evidence gate suppresses escalation to a tier that has no
coverage, and escalations fall to 99–172. `--escalationRequiresEvidence` is **off by
default**, and on this evidence it should not be.

**P4 is not answerable at this resolution and is recorded as such.** It predicted the
energy cut would cost < 1.0 m of rmse. No accuracy sign test across the five scales is
significant (11+/9−, 10+/10−, 11+/9−, 6+/14−, 12+/8−), but the bootstrap CIs run to
±170 m, and the noise floor below shows why: **a policy-arm rmse difference smaller than
about 30 m carries no information here.** A 1 m prediction cannot be tested against a
30 m floor.

### The noise floor, which bounds every accuracy claim in §4

The static arms carry no policy, so their `k1a`-vs-`k1b` difference is pure shared-RNG
displacement (rule 8):

| cell | arm | median \|drift\| | sign test |
|---|---|---|---|
| `box750` | `static_868` | 1.25 m | 9+/11− p 0.82 |
| `box1000` | `static_868` | 3.16 m | 13+/7− p 0.26 |
| `box1500` | `static_868` | **24.93 m** | 14+/6− p 0.12 |
| `box2000` | `static_868` | **29.62 m** | 12+/8− p 0.50 |
| `box2000` | `static_2p4` | **29.39 m** | 7+/13− p 0.26 |
| `box3000` | `static_2p4` | **35.74 m** | 11+/9− p 0.82 |

None is significant, and all are large. On energy the same floor is 0.06–0.23 J, which
is why §4's energy result stands and its accuracy column does not.

---

## 5. The integrity check is costing accuracy and energy

In every cell, `ca_tbl_no_integrity` beats `ca_tbl_full` on **both** axes:

| cell | rmse full → no_integrity | energy full → no_integrity | sign test (rmse / energy) |
|---|---|---|---|
| `k1a_box1000` | 75.6 → 45.1 m | 15.81 → 15.45 J | 6+/14− p 0.115 / 7+/13− p 0.263 |
| `k1a_box1500` | 99.8 → 69.2 m | 15.94 → 14.94 J | 4+/16− p 0.0118 / 6+/14− p 0.115 |
| `k1a_box2000` | 206.7 → 90.8 m | 19.69 → 16.15 J | 2+/18− p 0.0004 / 3+/17− p 0.0026 |
| `k1c_brief` | 344.4 → 135.5 m | 21.60 → 15.88 J | **2+/18− p 0.0004** / **1+/19− p 0.0000** |

**Up to 209 m and 5.7 J, on 18 and 19 seeds of 20.** The integrity check is not failing
to help; it is costing.

**This inverts `report/INTEGRITY_CHECK_RESULTS.md`**, which measured energy improved 8/8
and accuracy 7/8 and is described in the debt register as the strongest finding in the
repository. The two are not in contradiction. That campaign ran at `--duration=600
--boxHalf=1000` **on the split environment**, where 2.4 GHz had r80 1149.5 m instead of
156.0 m. Same mechanism, two propagation regimes, and only one of them is the regime the
scenario now builds.

**Stated mechanism, to be tested rather than assumed.** χ²₀.₉₉ = 9.21 presumes a
consistent filter; A4 measured tier-1 sigma over-stated ~1.7×, and the corrected
propagation makes coasting longer and innovations larger. A mis-scaled innovation
statistic marks good fixes suspect, and `suspectForcesEscalation` (default **true**)
converts each into an escalation to a tier that cannot solve. The predicted signature is
more escalations and fewer fixes; `k1c_brief` shows 170 escalations against 101 and 1236
fix cycles against 1320.

`report/PREREG_K2_INTEGRITY_COST.md` registers the three-cell ablation that separates
corroboration, forced escalation and the Mahalanobis test — with the threshold explicitly
barred from being tuned.

---

## 6. Predictions, scored

| # | prediction | outcome |
|---|---|---|
| P1 | At the smallest scale `static_2p4` dominates `ca_tbl_full` | **HELD.** It dominates at every scale. |
| P2 | A crossover exists in the sweep above which `static_2p4` stops dominating | **FAILED.** No such scale. |
| P3′ | The repaired bundle cuts `totalJ` by ≥ 30% at every scale | **HELD**, by 83–96%. |
| P4 | That cut costs < 1.0 m of rmse | **NOT ANSWERABLE** — the noise floor is ~30 m. |
| P5 | `ca_tbl_full` dominates `fixed_threshold` at and above the crossover | **FAILED.** Ties at 750/1000, dominated from 1500 up. |
| P6 | `static_868` never dominates `ca_tbl_full` | **FAILED.** It dominates from `box1000` up. |

Registering P1 — a prediction that the system loses — was worth it: it held, and it is
the only prediction about the comparison that did.

---

## 7. What this does not settle

- **The tier-2-abundant regime was not measured.** `boxHalf` below ~400 m costs 900–2000 s
  per seed and did not fit (Amendment 2 §A2.4). The trend across `box750`→`box3000` is
  that CA-TBL's position improves as the box shrinks — at 750 it is non-dominated by
  `static_868` — and **nothing here extrapolates into the untested regime.** It is the
  first thing to run next.
- **Tier 3 never fires.** `static_uwb` obtains zero fixes in all 11 cells, and part of
  that is the foliage defect rather than the architecture. A three-tier claim resting on
  a tier this scarce is not yet a three-tier claim.
- **The repaired bundle is five flags.** Only the energy result survives the noise floor,
  and it is not attributed to a single flag.
- **One duration, one mobility model, one environment.** 3600 s of the same waypoint walk
  in `RURAL_FORESTED`. The correction makes the tiers consistent with each other; it does
  not make this forest the right forest.
- **None of this vindicates the confidence score.** `PCS_FORMS_RESULTS` §2 answered that
  separately and negatively. Domination is a systems claim, not a claim for PCS.

---

# K4 — the small-box regime, measured. The gap does not close.

**Registered** `report/PREREG_K4_SMALLBOX.md` (`cc8aebb`), committed before the cell ran.
`boxHalf=400`, gateway spacing 360 m against the measured 156 m tier-2 range — the regime
§7 above recorded as a **gap** and said nothing extrapolated into.

**Abort conditions cleared before analysis:** 120 rows exactly, all ten shards at the same
commit `cc8aebb`, all `dirty 0`.

## The result

| arm | rmse | energy | fix cycles |
|---|---|---|---|
| `ca_tbl_full` | 6.800 m | 17.832 J | 1428 |
| `static_868` | 44.911 m | 15.401 J | 1439 |
| `static_2p4` | **3.256 m** | **14.063 J** | 1414 |
| `static_uwb` | 203.411 m | 871.729 J | 0 |
| `fixed_threshold` | 9.247 m | 17.423 J | 1426 |
| `ca_tbl_no_integrity` | 8.555 m | 17.367 J | 1428 |

| vs | rmse | energy | verdict |
|---|---|---|---|
| `static_868` | **+37.133 m, 20+/0− p<0.0001** | −2.429 J, 0+/20− | **incomparable** |
| `static_2p4` | −3.699 m, 0+/20− | −3.698 J, 0+/20− | **DOMINATED** |
| `static_uwb` | +197.210 m | +853.416 J | DOMINATES |
| `fixed_threshold` | **+2.607 m, 19+/1− p<0.0001** | −0.353 J, 2+/18− | **incomparable** |
| `ca_tbl_no_integrity` | +1.735 m, 17+/3− p 0.0026 | −0.398 J, 0+/20− | incomparable |

**All three predictions held, including T2, which predicted the system still loses.**

- **T1 held.** `ca_tbl_full` is non-dominated by `static_868`: **37.1 m of accuracy for
  2.43 J**, 20 of 20 seeds. This is the strongest showing of the tiering mechanism in the
  project.
- **T3 held.** It also beats `fixed_threshold` on accuracy for the first time at
  significance — 2.61 m on 19 of 20 seeds — for 0.35 J. That is an improvement on the tie
  at 750 and 1000 and the domination from 1500 up.
- **T2 held.** `static_2p4` still dominates, on **both** axes, at **20 of 20 seeds each**.

## U1 does not fire, and the trend it was about is real but does not save the claim

The favourable trend §7 reported is genuine — `ca_tbl_full` improves from 276.9 m at
`box3000` to **6.80 m** at `box400`, and its standing against `static_868` goes from
dominated to non-dominated, against `fixed_threshold` from dominated to better-on-accuracy.

**But `static_2p4` improves faster.** The ratio of `ca_tbl_full`'s rmse to `static_2p4`'s
barely moves as the box shrinks — 3.87× at `box1000`, 2.15× at 750, **2.09× at 400** — and
the coverage argument that qualified the earlier cells disappears here: `static_2p4` holds
**1414 of 1440** fix cycles at `box400`, against 1082 at 750 and 570 at 3000. At this
scale it is not winning by declining to answer.

**So the answer stands, and is now measured rather than extrapolated: across
`boxHalf` 400–3000, there is no scale at which CA-TBL beats a single 2.4 GHz radio.**
As tier 2's coverage improves, the single-band arm improves at least as fast.

## What K4 changes

- §7's gap is closed for `boxHalf=400`. The regime below that is still unmeasured.
- The architecture demonstrably works *as a mechanism*: it converts tier-2 availability
  into accuracy over tier 1, at 20 of 20 seeds, and it beats the fixed threshold on
  accuracy where tier 2 is reachable.
- **What it does not do is beat carrying one 2.4 GHz radio**, in any cell of this project,
  and that is now sixteen cells across seven deployment scales.

## The technique, which is reusable

The cell ran as **ten processes of two seeds** and was concatenated. The split is
byte-identical — the registration carries the proof, seed 8 alone against the same seed as
the second of twenty inside `k1a_box1500`, `cmp` clean on all six arms. That turns a
5-hour single-threaded cell into a 30-minute one, and it is why this regime was measurable
at all. Every future campaign whose makespan is set by one slow cell should use it.
