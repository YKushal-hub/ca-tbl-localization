# Claim 3's only PASS under the corrected NLOS source — result

Registered in `CLAIM3_NLOS_SOURCE_REGISTRATION.md`, committed before the first cell. 20 seeds,
`source 1f73930 dirty 0 profile optimized`, zero failures. Criterion `b49df46` §4, applied
verbatim. No prediction was registered on the outcome.

## The confound was ruled out first

The published cell is `source 0a489df`, 2026-08-27. The binary has changed since — D1, two-ray,
Jensen foliage, the B1 additions. **Re-running the legacy cell at seed 1 on the current binary
reproduces the published `arm_summary.csv` digest byte-for-byte**
(`8fac0bafecd242431e39e024287cac831c4f057c`). So `nlosSource` is the only difference between the
two cells, and the comparison is clean. Without this the result would have been confounded by a
month of unrelated change.

## Verdict: FAIL

| | legacy (published) | propagation |
|---|---|---|
| `ca_tbl_full` meets (A=0.95, R=25 m) | 20/20 | 20/20 |
| `static_2p4` meets it | 20/20 | 20/20 |
| **M1 ratio, geometric mean** | **0.9531** | **1.1873** |
| seed-level bootstrap 95% CI | [0.9296, 0.9772] | **[1.1782, 1.1965]** |
| position vs 1.0 | entirely below | **entirely above** |

**Not INCONCLUSIVE — FAIL.** The interval does not straddle 1.0; it sits clear of it in the
opposite direction. `static_2p4` still qualifies, so there was something to compare against and
the second INCONCLUSIVE route did not apply.

## The asymmetry, confirmed exactly

The registration's concern was that the flag could move one arm's energy and not the other's.
Mean per seed over 20 seeds:

| | legacy | propagation | delta |
|---|---|---|---|
| **`static_2p4` totalJ** | 15.201 | **15.201** | **+0.000** |
| — its 868 J / 2.4 J / UWB J | 0.014 / 15.178 / 0.008 | 0.014 / 15.178 / 0.008 | all +0.000 |
| **`ca_tbl_full` totalJ** | 14.523 | **18.039** | **+3.516 (+24.2%)** |

`static_2p4` is unchanged **in every component to three decimals**. It has no tier 3, so the flag
cannot reach it. The entire movement is in the numerator.

## The mechanism, and it is not the one the census suggested

The census showed legacy inflating tier-3 *service* tenfold, and tier 3 is the most expensive
tier — so the naive expectation is that legacy makes `ca_tbl_full` **more** expensive. It does the
opposite. The breakdown says why:

| `ca_tbl_full`, mean per seed | legacy | propagation | delta |
|---|---|---|---|
| 868 MHz energy | 3.427 J | **9.093 J** | **+5.665** |
| 2.4 GHz energy | 9.758 J | 8.938 J | −0.819 |
| UWB energy | 1.339 J | 0.008 J | −1.330 |
| cycles on UWB | 553.0 | **0.0** | −553.0 |
| escalations | 5.7 | **170.9** | +165.2 |

**Under legacy the node escalates rarely (5.7 times) and then parks on UWB for 553 of 1440
cycles.** While parked it is not paying the tier-1 floor, and tier-1 energy is only 3.427 J.
**Under the corrected source it never reaches UWB at all** (0 cycles), escalates 170.9 times
without settling, and runs the tier-1 floor continuously at **9.093 J**.

So the saving was never the precision tier being cheap. **It was the floor being switched off
while the node sat on a tier the defect had sent it to.** The +5.665 J of restored floor cost is
four times the 1.330 J of UWB cost it replaced.

## Consequence, applied as registered

> M1 crosses 1.0 ⇒ the project's only PASS depends on a documented defect. It is withdrawn with
> its reason, in the W1 style, original text retained, and Claim 3 returns to
> withdrawn-with-no-replacement.

Applied. Claim 3's replacement entry is withdrawn as **W11**. The original text is retained in
`Simulation.md` §11.1 beneath the withdrawal note, and `[M-27]` is annotated rather than deleted.

**This does not touch Claim 9**, which was falsified on its own evidence and is unaffected.

## What is and is not established

**Established:** the 4.5% energy advantage that constituted Claim 3's only PASS is an artefact of
`nlosSource=legacy`. Under the corrected source the same cell shows `ca_tbl_full` **18.7% more
expensive** than a single-band arm at equal reliability.

**Not established:** that no configuration exists in which a tiered arm is cheaper at equal
reliability. This is one cell of 320, re-measured. The other 319 were adjudicated under the same
flag and **have not been re-run** — the honest position is that the grid's verdicts are now of
unknown status, not that they are all wrong.

**The five caveats that travelled with the original PASS are now moot**, and one of them reads
differently in hindsight: *"a single isolated cell of 320"*. It was isolated because it was the
only cell where the defect and the geometry combined to switch the floor off.
