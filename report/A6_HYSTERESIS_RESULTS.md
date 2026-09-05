# A6 — hysteresis is a live axis, and the dwell is an energy/accuracy dial

**Written** 2026-09-05 from `results/a6_hysteresis`, 300 cells, 5 bands × 3 dwells × 20
paired seeds, **300/300 recording `dirty 0`**. Registered in
`report/PREREG_A6_HYSTERESIS.md` (`4aa38aa`) **before the campaign ran**.

**Verdict: LIVE AXIS.** Prediction 3 — which said the accuracy response would be flat —
is **falsified**. CAL-03 and CAL-04 are not merely answerable now; they are parameters
that need choosing.

## The dominant axis is `nDeescalate`, not the band

Paired per-seed differences against the built-in cell (band 0.2, dwell 5), `ca_tbl_full`:

| cell | Δ cep50M | 95 % CI | sign | Δ totalJ | Δ escalations |
|---|---|---|---|---|---|
| any band, **n=1** | **+0.64 to +0.78 m** | excludes 0 | 19–20 +/ | **−95 to −103 J** | **+204 to +334** |
| any band, n=5 | −0.03 to +0.05 m | straddles 0 | — | ≈ −5 J | ≈ 0 |
| any band, **n=10** | **−0.03 to −0.20 m** | mostly excludes 0 | 2–6 +/ | **+37 to +55 J** | **−89 to −133** |

Read down the table and the mechanism is plain: **de-escalating after one healthy cycle
makes the arm thrash.** It adds 200–330 escalations, costs 0.6–0.8 m of median accuracy,
and *saves* about 100 J — because it spends less time on the expensive precision tier.
Demoting after ten healthy cycles does the reverse: fewer transitions, slightly better
accuracy, and 40–55 J more energy.

**So the hysteresis dwell is an energy/accuracy dial, and it was never exposed.** That is
the finding. It is not a large accuracy effect — a fifth of a metre at the extremes — but
it is a real one that orders consistently, and it moves energy by 5–10 %.

## The band alone does almost nothing, and one arm proves it

`fixed_threshold` shows **exactly zero** sensitivity to the band. Every `n=1` cell is
`+0.8946`, every `n=5` cell is `+0.0000`, every `n=10` cell is `−0.3087` — identical to
four decimal places across all five bands.

That is not a null result, it is a **control**. `fixed_threshold` decides on a variance
threshold, not on PCS, so `--pcsHysteresisBand` — which sets `pcsThresholdDown` — cannot
reach it. The flag correctly does nothing to the arm that does not use the score, and
does something to the arm that does. If the band had moved `fixed_threshold`, the flag
would have been touching something it should not.

On `ca_tbl_full` the band does have a small effect, but only at the extreme: `b0.8/n5`
gives −0.21 m and `b0.8/n10` −0.20 m, against ≈ 0 for bands 0.0–0.4. A wider band helps
slightly, and only once it is four times the built-in value.

## What this means for CAL-03 and CAL-04

- **CAL-04 (`nDeescalate`)** is the parameter that matters. The built-in 5 sits between
  a thrashing regime at 1 and a slower, more accurate, more expensive regime at 10. It
  is a defensible middle, and it is now a *measured* middle rather than an observation
  about how long 141 of 189 blind runs happened to be.
- **CAL-03 (the band)** is second-order at this geometry. Worth stating as measured and
  small rather than leaving deferred.

Neither is calibrated by this campaign — a 15-point grid locates a direction, not an
optimum, and the registration says so.

## What this campaign cannot establish

- **An optimum.** Five bands and three dwells locate a direction. Choosing a value needs
  a finer grid and a criterion to choose against.
- **Anything at other geometries.** One `boxHalf`, one environment.
- **Anything about the repaired PCS forms.** `pcsForm=legacy` throughout, so this is
  hysteresis on the score the tree was built with, not on `weighted` or `gated`.
- **Whether the energy/accuracy trade is worth making.** That needs the A0 criterion
  applied to these cells, which has not been done.
