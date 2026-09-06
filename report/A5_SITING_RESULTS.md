# A5 — complementary tier-3 siting: P1 falsified, and co-location is vindicated

**2026-09-06.** Pre-registered in `report/PREREG_A5.md` **before the first cell ran**.
`--sitingMode ∈ {0,1}`, seeds 1–5, `--gwGrid=9`, `--boxHalf=3000`, `--duration=3600`.
5/5 seeds paired, no cells excluded.

**Ordering satisfied.** A4 completed first and applied no calibration, so this measures
siting against the same filter every other result in the tree used.

## Result: P1 is falsified, decisively

**Tier-3 coverage does not improve. It goes to zero.**

| metric | mode 0 (co-located) | mode 1 (complementary) | signs |
|---|---:|---:|---|
| `cyclesInUwbCoverage` (all arms) | 50.2 | **0** | `-----` |
| `static_uwb` `fixCycles` | 43.2 | **0** | `-----` |
| `static_uwb` `rmseM` | 968 m | **1498 m** | `+++++` |

Five seeds out of five, every arm. This is not a marginal effect and it is not noise.

## Why — geometry, checked rather than inferred

The facility sites sit at `±0.47 × boxHalf = ±1410 m`, i.e. radius **1994 m**. Rotating that
square 45° puts tier 3 at the same radius, so the *design intent* — same distance from the
centre, in the gaps — was implemented correctly. But the nearest rotated tier-3 site to a
facility **the walker actually dwells at** is:

```
√((1994 − 1410)² + 1410²) = 1526 m
```

against a UWB nominal range of tens of metres. **The tag is never within a thousand metres
of a tier-3 anchor.** Coverage is not degraded; it is eliminated.

The code already said this would happen. `ca_tbl_causal_sim.cc:148` ties the UWB ring
radius to the facility position specifically so that *"at a facility"* and *"in UWB range"*
cannot diverge. `--sitingMode=1` breaks exactly that coupling.

## What this establishes

**UWB coverage in this deployment is a dwell-coupled property, not a geometric one.**
Tier 3 works because its anchors are where the person goes, not because of how they are
distributed over the box. Any siting change that decouples the two removes the tier.

**The deliberate co-location decision is now vindicated by measurement rather than
asserted.** `BuildDeployment()`'s comment justifies co-location on the grounds that
separated tiers would confound a siting artefact with a decision error. That reasoning was
about *interpretability*. This result adds a second, stronger reason: at this site spacing
and this tier-3 range, separation does not trade coverage for anything — **it forfeits the
tier**.

## What is **not** established

**This does not show that no complementary siting can work.** It shows that *this* one — a
45° rotation at equal radius, tier 3 moved rather than added — fails, and why. The failure
mode is specific and diagnosable: site spacing (≈1526 m between rotated and original sites)
is two orders of magnitude larger than tier-3 range. A complementary scheme with sites
close enough to remain dwell-coupled is a different experiment and is not run here.

**The obvious variant is confounded and was deliberately not run.** Adding tier-3 sites
*in addition* to the co-located ones would very likely improve coverage — but it changes
anchor count as well as siting, so it would measure "more anchors" and not "different
siting". That is the confound A5 exists to avoid.

**P2 and P3 are not interpretable in the usual way.** With tier-3 coverage at zero, the
accuracy comparison is not "complementary siting costs accuracy" but "the arms no longer
have a tier 3". `static_uwb` `rmseM` at 1498 m is a tier with no anchors in range, not a
worse-sited tier. P3's sign test on `fixCycles` is reported for completeness and splits on
four of six arms, which is what rule 8 says is not a result.

The one number worth flagging for a follow-up rather than a claim: `ca_tbl_full` and
`ca_tbl_no_integrity` **gained ~48 J** (635 → 683) under mode 1 while their `rmseM`
changed by ~1–2 m. Energy spent escalating to a tier that is no longer reachable is the
obvious candidate, and it would be a clean demonstration of the escalation cost the energy
axis was built to measure — but it is one configuration at five seeds and is not claimed.

## Verdict

**A5 is answered in the negative, and the negative is the useful direction.** The question
was whether deliberate co-location costs coverage. It does not: at this geometry it *is*
the coverage. The recommendation is to keep `sitingMode=0` as the default — which it is —
and to treat the flag as the instrument that established this rather than as a
configuration anyone should run.
