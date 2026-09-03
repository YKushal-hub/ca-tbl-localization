# Quiet-window cost and multi-seed delivery — result

Registered in `B1_QUIET_WINDOW_REGISTRATION.md`, committed before the first cell. 10 cells,
`source 9c7140b dirty 0 profile optimized`, zero failures. **G16 passes on all ten traces**, and
the beacon counter tracks the condition cleanly: 1440 of 1440 UWB exchanges have a beacon present
with beacons on, 1 of 1440 with them quiesced.

## §3 — FALSIFIED. The beacon-contention account is withdrawn

Registered: adjacent intervals must **not** overlap; across and tier 2 must.

| link | beacons on | beacons off | |
|---|---|---|---|
| tier 2, 259.81 m | 0.9799 [0.9765, 0.9832] | 0.9778 [0.9725, 0.9830] | OVERLAP |
| **UWB adjacent, 11.76 m** | 0.8433 [0.7909, 0.8958] | 0.8217 [0.7795, 0.8639] | **OVERLAP** |
| UWB across, 19.02 m | 0.6511 [0.6031, 0.6991] | 0.6411 [0.6118, 0.6704] | OVERLAP |

**The registered falsifier fired on adjacent, so the beacon-contention account is withdrawn.**
The per-seed deltas show why:

| seed | on | off | delta |
|---|---|---|---|
| **1** | 0.7458 | 0.8500 | **+0.1042** |
| 2 | 0.8458 | 0.7750 | −0.0708 |
| 3 | 0.8792 | 0.8847 | +0.0056 |
| 4 | 0.8431 | 0.7736 | −0.0694 |
| 5 | 0.9028 | 0.8250 | −0.0778 |

**The sign flips, three of five are negative, and the mean is −0.0216** — beacons off is very
slightly *worse*, which is the opposite of the account. Seed 1 is the outlier at +0.1042, and
seed 1 is the seed the original single-seed test ran on, at 600 s with n = 120 per link, where it
measured +0.1833.

**And the anomaly the account was invented to explain does not exist either.** The observation was
that adjacent (0.7458) and across (0.7208) were flat across a 1.6× distance ratio, which is not
what path loss does. Over five seeds they are **0.8433 against 0.6511** — a clear monotone
decrease with distance. Seed 1 happened to combine an unusually low adjacent rate with an
unusually high across rate. **There was never a flat distance dependence to explain; there was one
seed.**

What survives is a plain statement: seed-to-seed variation in UWB adjacent delivery has a 95%
half-width of about ±0.05, which is wide enough to have manufactured a 0.18 effect from a single
draw. The earlier report labelled the −0.0667 across-ring delta as noise on n = 120 and did not
extend the same scepticism to the +0.1833 on the same n, because the effect was large. **Size is
not evidence of replication.**

The model remains slightly optimistic on both links — 0.9162 against a measured 0.8433, and
0.7296 against 0.6511 — by roughly 0.07–0.08 on each, with the correct shape.

## §2 — VACUOUS. The population the prediction needs is empty

Registered: suppressing UWB beacons degrades **tier-3-served** cycles and leaves **tier-2-served**
cycles unchanged.

Cycles that produced a fix, by the tier that supplied it, summed over five seeds:

| condition | total fixes | 868 MHz | 2.4 GHz | UWB |
|---|---|---|---|---|
| beacons on | 6030 | **5935** | 34 | **61** |
| beacons off | 6020 | **5995** | 25 | **0** |

**Tier 3 supplies 61 fixes out of 6030 — one percent — and tier 2 supplies 34.** The tier-1 floor
supplies 98.4%. There is no tier-3-served population to degrade and no tier-2-served population to
hold constant, so the registered comparison **cannot be scored**. An empty population is VACUOUS,
not a pass, and it is not a falsification either.

This is the "selection, not fusion" structure already on record (`ca_tbl_causal_sim.cc:2713-2714`,
`:2723`) reappearing: the floor supplies the fix whenever the active tier fails to solve, and here
it almost always does.

Overall availability is 0.1065 [0.0902, 0.1228] with beacons on against 0.0776 [0.0588, 0.0965]
with them off — **overlapping**, and in any case not attributable to tier 3 given the fix-tier
census above.

**What the registration got wrong**, recorded because it is reusable: it specified a comparison
stratified by served tier without first establishing that both strata were occupied. The check on
that would have cost one `fixTier` census on an existing trace and would have prevented the
campaign. **State a stratum's occupancy before registering a comparison across it** — the same
family as stating a check's population before trusting its result.

## Solvability, recomputed at the interval bounds

From the five-seed measurement with beacons on, which is the deployment as configured:

| basis | 1 round | 2 rounds | 3 rounds | rounds to 0.99 |
|---|---|---|---|---|
| lower CI bound (0.7909 / 0.6031) | 0.6641 | 0.9857 | 0.9996 | **3** |
| point estimate (0.8433 / 0.6511) | 0.7817 | 0.9952 | 0.9999 | **2** |
| upper CI bound (0.8958 / 0.6991) | 0.8802 | 0.9988 | 1.0000 | **2** |
| model, residual-corrected | 0.9191 | 0.9995 | — | 2 |

**B2's round count is 2–3 rounds to 0.99 solvability, as a range**, not the single figure the
single-seed basis would have given. At one pair per epoch and 10 pairs per site, a round is 10
survey epochs, so **calibration reaches 0.99 in 20–30 epochs, i.e. 50–75 s at a 2.5 s cycle.**

The earlier point estimate of 2 rounds sat at the optimistic end of that range. It was not wrong,
but it was not entitled to be stated without the third round beside it.
