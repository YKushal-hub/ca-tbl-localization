# The p10 grid under the corrected NLOS source — result

Registered in `P10_GRID_PROPAGATION_REGISTRATION.md`, committed **before scoring** — and labelled
there as weaker than a pre-registration, because the cells already existed on disk. Scorer
`scripts/claim3_m1.py`, validated against the published legacy cell.

## The prediction: HELD

> No cell returns a PASS under `propagation`.

**No PASS.** Three cells are adjudicable at 20 seeds; all three are FAIL, all three CIs entirely
above 1.0.

## The paired comparison — every adjudicable cell, both sources

| cell (20 seeds) | legacy | propagation | ΔM1 |
|---|---|---|---|
| `ef0_dw800_b1500` | **PASS** 0.9531 [0.9296, 0.9772] | **FAIL** 1.1873 [1.1782, 1.1965] | **+0.2342** |
| `ef0_dw800_b2000` | FAIL 1.0878 [1.0550, 1.1187] | FAIL 1.2787 [1.2615, 1.2961] | **+0.1909** |
| `ef0_dw90_b1500` | FAIL 1.1571 [1.1507, 1.1639] | FAIL 1.1658 [1.1564, 1.1757] | **+0.0087** |

**M1 rises in every adjudicable cell, and nothing crosses into PASS.** The falsifier did not fire.

**Adjudicability is unchanged.** The same three cells are adjudicable under both sources, and the
same twelve are INCONCLUSIVE for the same reason — no single-band arm meets (A = 0.95, R = 25 m),
so there is nothing to compare against. The flag moves M1; it does not move spec-meeting. Of the
two routes by which a verdict could have changed, only one was live.

## The rise tracks dwell, which is the mechanism confirming itself

The three ΔM1 values are not alike, and the pattern is the W11 mechanism restated:

| cell | dwell | ΔM1 |
|---|---|---|
| `ef0_dw800_b1500` | 800 | +0.2342 |
| `ef0_dw800_b2000` | 800 | +0.1909 |
| `ef0_dw90_b1500` | **90** | **+0.0087** |

W11's account is that legacy lets the node **park** on UWB and stop paying the tier-1 floor. Parking
requires the node to stay somewhere — which is what `dwell = 800` provides and `dwell = 90` does
not. At `dwell = 90` the correction is worth **0.0087**, twenty-seven times smaller than at
`dwell = 800`.

**This was not predicted and is not claimed as one.** It is a consistency check the data offered
for free: an account that attributed the artefact to parking predicts the effect should nearly
vanish where parking cannot happen, and it does.

## Screening cells

`ef3` and `ef4` carry 5 seeds and are **not adjudicated**, as registered — the rule's CI criterion
is not meaningful at that depth. They are recorded as screening only.

## What this closes, and what it does not

**Closes:** Claim 3. Its only PASS is withdrawn (W11), and the grid re-scored under the corrected
source contains **no PASS in any adjudicable cell**. Claim 3 is withdrawn with no replacement, and
the withdrawal now rests on the whole adjudicable grid rather than on one re-run cell.

**Does not close:** whether a tiered arm can ever be cheaper at equal reliability. Twelve of
fifteen cells remain INCONCLUSIVE because no single-band arm meets the requirement there — and
that is the structural point already recorded against the original claim: **the cost comparison is
decidable only where tiering is least needed.** Correcting the NLOS source does not change that;
it removes the one cell that appeared to escape it.

**Does not touch Claim 9**, and does not settle **Claim 4**, which is exposed to the same lever
through the binary's `legacy` default and is explicitly a claim about switching the floor off.
