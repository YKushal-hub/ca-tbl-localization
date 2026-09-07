# The switching-point integrity check earns its place

**2026-09-07.** Eight seeds, paired. `--duration=600 --boxHalf=1000 --nFinders=4
--finderMode=1 --sendHandshakeFrames=true`, arm `ca_tbl_full`, tri-band topology.
On = `--imuCrossCheck=true --corroborationCheck=true`.

## What is being tested

Before a confidence estimate may trigger an escalation, it is cross-validated against
**independent** evidence. Two witnesses, both suppress-only — they can refuse an
escalation, never cause one.

- **The body.** Dead-reckoned displacement against the displacement the filter claims.
- **The neighbours.** For every node ranged this cycle whose own reported position is
  held in the shared table, the measured range is compared against the distance from
  this node's estimate to where that node *says* it is.

The second is the one that matters, and the reason is precise: it uses **residuals and
other nodes' reports, not the filter's covariance.** The covariance is the quantity the
confidence signal is made of, so a check built on it can only ever agree with it. The
pre-existing Mahalanobis test has exactly that problem — it validates a fix against the
filter's own prior, so a filter that has quietly gone wrong is confidently wrong and the
check concurs.

## Result

| seed | rmse off → on | Δ | energy off → on | Δ |
|---:|---|---:|---|---:|
| 1 | 6.88 → 6.23 m | −0.65 | 52.4 → 51.2 J | −1.2 |
| 2 | 6.75 → 5.86 m | −0.89 | 83.3 → 68.1 J | −15.2 |
| 3 | 6.17 → 5.17 m | −1.00 | 64.3 → 53.1 J | −11.2 |
| 4 | 6.54 → 5.34 m | −1.20 | 85.7 → 81.1 J | −4.6 |
| 5 | 6.11 → 6.00 m | −0.11 | 77.1 → 53.1 J | −24.0 |
| 6 | 6.20 → 5.47 m | −0.73 | 80.3 → 51.0 J | −29.3 |
| 7 | 7.44 → 6.04 m | −1.40 | 80.1 → 61.0 J | −19.1 |
| 8 | 6.27 → 6.28 m | +0.01 | 52.9 → 34.8 J | −18.1 |

**Energy: improved in 8 of 8 seeds**, mean −15.36 J (sd 9.44).
**Accuracy: improved in 7 of 8**, mean −0.743 m (sd 0.494).

A sign test on 8/8 gives p ≈ 0.004 one-sided for energy. Accuracy at 7/8 is p ≈ 0.035.
Neither is a substitute for a proper interval, and none is claimed below.

## Why this is the conjunction, and why that matters

**Both axes move the right way at once.** Every previous attempt in this project bought
availability with error or accuracy with energy. This does not: it removes escalations
that the confidence signal wanted and that independent evidence did not support, and
those escalations were going to be useless — so the energy is saved *and* the estimate
does not suffer for the saving.

Escalation counts fall from 34–39 to 25–38, so the mechanism is doing what it says: it
is refusing, not re-timing.

## What is **not** claimed

- **One configuration.** 600 s, 1 km box, four finders, one mobility model. Nothing is
  claimed about how this varies with density, area, finder count or casualty motion.
- ~~**No interval.**~~ **Done, 2026-09-08** -- see the section below. The sentence that
  stood here said a bootstrap CI on the paired difference was the next step and not a
  formality to skip. It was neither skipped nor a formality: both intervals exclude
  zero.
- **The IMU witness barely fires.** One refusal at seed 7 against 16 from corroboration.
  On this scenario the effect is **almost entirely the neighbour check**, and reporting
  it as "IMU cross-validation works" would be crediting the quiet witness for the loud
  one's result. The IMU model's drift constants are ours and uncalibrated; a scenario
  with real sensor degradation is where that witness would be expected to speak.
- **The corroboration constants are ours.** `corroborationK = 3`, assumed ranging sigma
  30 m, agreement floor 0.5, minimum two witnesses. None is cited, and the result's
  sensitivity to them is untested.

## The interval, added 2026-09-08

`scratch/python_handoff/analyse_integrity_ci.py` over the same eight paired cells in
`catbl_work/cc/`. No new runs: this adds an interval to differences already measured,
and it does not widen the configuration they came from.

Percentile bootstrap, B = 20 000, **resampling seeds rather than runs** -- the seed is
the unit, because off and on are the same trajectory under two flag settings, and that
pairing is the whole design. Fixed RNG seed, so the interval is reproducible.
Completeness is asserted before anything is pooled: every cell present, exactly one data
row each, seed sets matching (rule 12).

| quantity | mean paired difference | 95% CI | seeds improved | exact sign test |
|---|---:|---|---:|---:|
| energy | **-15.360 J** | **[-21.433, -9.233]** | 8 / 8 | p = 0.0039 |
| accuracy (rmse) | **-0.743 m** | **[-1.052, -0.416]** | 7 / 8 | p = 0.0352 |
| escalations | -5.625 | [-8.000, -3.000] | 7 / 8 | p = 0.0352 |

**All three intervals exclude zero.** The escalation row is what makes the other two
mechanistically legible rather than coincidental: the check is refusing escalations, and
the energy follows from the refusals.

**n = 8, and a percentile bootstrap at n = 8 is an interval, not a guarantee.** It
resamples the eight differences observed. It cannot speak to configurations not run, and
every caveat above it still stands -- one duration, one box size, one finder count, one
mobility model, uncalibrated corroboration constants, and an inertial witness that
contributes almost nothing.

## Where this sits against the brief

The project brief calls this *"your sharpest novel contribution … nobody validates the
confidence signal itself before it's allowed to drive a radio-mode decision in a
dual/tri-band system."* It was specified and, until now, not built.

It also answers a standing negative in this repository. The confidence score loses to a
monotone function of the filter's own covariance trace in 8 cells out of 8, because it
is a re-encoding of that trace and carries nothing else. The integrity check is the
first mechanism in the project that brings information the covariance does **not**
contain — what other nodes independently report — to bear on the escalation decision.
That is the direction in which the score can be made to earn its name, and this is
evidence the direction is real.
