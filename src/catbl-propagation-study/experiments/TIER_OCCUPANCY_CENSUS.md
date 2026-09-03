# Which configurations put work on tier 3? — census

No prediction registered. This is a census, run because novelty part 1 self-calibrates tier-3
anchors and one configuration had just shown tier 3 supplying **1%** of fixes.

Population: **2963 seed directories** carrying `ca_tbl` traces, across **103 distinct
configurations**, every results directory on disk. `scripts/tier_occupancy_census.py`.

## Occupancy is not service, and the gap is large

Two different columns, reported separately because conflating them is what produced the vacuous
quiet-window campaign:

- **occupied** — `activeTier == T`: the policy selected T this cycle.
- **serving** — `fixTier == T` and `hadFix`: T actually produced the position.

The extreme case in the corpus is `results/p10r/box1800_seed2`: tier 3 **occupied on 81.6%** of
cycles and **serving 3.6%** of fixes. A tier can be selected almost always and still fix almost
never — the selection-not-fusion structure already on record, where the tier-1 floor supplies the
fix whenever the active tier fails to solve.

**Any statement of the form "tier 3 matters here" must say which of the two it means.**

## The census, by configuration

| configuration | uwb fix | uwb occ | key flags |
|---|---|---|---|
| `p10_grid/c_ef0_legacy_dw800_b1500` | **31.6%** | 32.8% | `nlosSource=legacy dwell=800 boxHalf=1500` |
| `p10_grid/c_ef0_legacy_dw800_b3000` | 29.5% | 30.6% | `legacy dwell=800` |
| `p10_grid/c_ef0_legacy_dw800_b2000` | 20.1% | 24.1% | `legacy dwell=800` |
| `_dwell_check` | 19.7% | 20.8% | `dwell=800` (source unrecorded) |
| `p10_grid/c_ef1_legacy_dw800_b1500` | 13.3% | 19.6% | `legacy dwell=800` |
| `p10` | 9.2% | 77.3% | `boxHalf=1000` |
| **`sweep20_v7_v2_b3000`** | **5.7%** | 40.8% | **`nlosSource=propagation`** |

424 of 2963 seed directories reach a tier-3 fix share ≥ 5%; the best single seed is **32.2%**.

## The finding: the high tier-3 configurations depend on a documented defect

Grouped by NLOS source:

| `nlosSource` | configs | max uwb fix | median |
|---|---|---|---|
| **legacy** | 34 | **31.6%** | 0.3% |
| unrecorded | 16 | 19.7% | 6.1% |
| **propagation** | 53 | **5.7%** | 0.3% |

`ca_tbl_causal_sim.cc:416-423`:

```
if (src == CA_TBL_NLOS_PROPAGATION) { return propNlos; }
return (nlosDb > 0.0) || (hiddenState > 0);
```

**`legacy` is the union of propagation-NLOS and hidden-channel state** — the conflation this
project already records as a defect, and the reason `nlosSource=propagation` exists. It
over-reports NLOS, which drives escalation, which is why tier 3 is *selected* on 33% of cycles
instead of 3%.

The comparison that isolates it, holding dwell and box size fixed:

| configuration | uwb occ | uwb fix |
|---|---|---|
| `c_ef0_legacy_dw800_b1500` | 32.8% | 31.6% |
| `c_ef3_propagation_dw800_b1500` | 3.1% | 3.3% |

**A tenfold difference in tier-3 service from the NLOS source alone.** In both cases occupancy
and service are nearly equal — when tier 3 *is* selected it does fix. The flag does not change
whether tier 3 works; it changes how often it is chosen.

Dwell is the second driver and it is real: `dwell=800` reaches 31.6% against `dwell=90`'s 8.1%.
That is physical — a node lingering at a facility site is inside the tier-3 anchor ring.

## Verdict: outcome 2, with the reason recorded

> Tier 3 is ≤ a few percent everywhere ⇒ novelty part 1 targets a tier that does not serve, in
> every configuration the project has run. B2 then targets **tier 2**.

**Under the corrected NLOS source, tier 3's best fix share across 53 configurations is 5.7%, and
its median is 0.3%.** Every configuration above ~6% depends on `nlosSource=legacy`.

Building B2 on tier 3 would mean selecting a scenario **because a known defect inflates the
stratum B2 needs**. That is choosing the configuration to fit the result, and the fact that the
choice would be made before any B2 number exists does not make it less so.

**So B2 targets tier 2**, where B1 measured **0.98** anchor-to-anchor delivery with an order of
magnitude of range headroom, and where the anchors serve. The cost is stated plainly: tier 2's
three-anchor sites have **zero redundancy** for self-calibration at every survey count
(`B2_GAUGE.md`), so B2 on tier 2 requires `--t2Anchors ≥ 4`, and the anchor count becomes the
swept deployment parameter rather than a fixed property.

## What is not concluded

That tier 3 is useless. It supplies 3% of fixes under the corrected source and it is the precision
tier — 3% of fixes at the best accuracy in the system is not nothing, and the escalation policy
choosing it rarely is a separate question from whether it works when chosen. **What is concluded
is narrower: there is no configuration, free of a known defect, in which tier-3 anchors serve
enough cycles to justify building self-calibration for them first.**
