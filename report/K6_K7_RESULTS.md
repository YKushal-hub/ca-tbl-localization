# K6 and K7 — the envelope exists, and the integrity gate is fixable

Both registered before their cells ran. 120 rows each, single commit per cell, all
`dirty 0`.

---

# K7 — the missing R term was the defect. Suspect rate 32.3% → 5.7%.

`--mahalUseR=true`, one flag, nothing else changed. Control `k1a_box1500`.

| | control | cell | median Δ | sign test | 95% CI |
|---|---|---|---|---|---|
| **suspects / 1440** | **465 (32.3%)** | **81.5 (5.7%)** | **−387.5** | **0+/20− p<0.0001** | [−408.5, −353.5] |
| `rmseM` | 99.756 m | **72.721 m** | −21.894 m | 6+/14− p 0.115 | [−51.10, 0.00] |
| `totalJ` | 15.936 J | 15.459 J | −0.035 J | 10+/10− p 1.000 | [−0.74, +0.52] |
| `escalations` | 99 | 92 | −2.5 | 9+/11− p 0.824 | [−18.5, +2.0] |

**The displacement floor is exactly zero** — `static_868` and `static_2p4` are
byte-identical between control and cell, `0+/0−`. This is a pure treatment effect.

| # | prediction | outcome |
|---|---|---|
| Z1 | suspects below 200 | **HELD** — 81.5 |
| Z2 | further than K5's best (310.5 at Q ×4) | **HELD** — by a factor of 3.8 |
| Z3 | closes at least half the 30.6 m gap to `ca_tbl_no_integrity` | **HELD on magnitude** — 27.0 m of 30.6 m, 88% of the gap, though the sign test does not reach α |
| Z4 | energy does not rise | **HELD** — unchanged |

**A Mahalanobis distance is `dᵀ S⁻¹ d`. The default omits `(JᵀR⁻¹J)⁻¹` from `S`.** Dropping
a positive-definite term makes `S⁻¹` larger and inflates every distance, so the test fires
32× too often. Restoring it takes the rate to 5.7% — within a factor of six of the 1%
design rate, against a factor of 32 before — and recovers 88% of the accuracy the integrity
check was costing, **for no energy**.

This was never a tuning problem. The correct estimator was already written, already
described in the source as "the correctly specified integrity gate", and switched off.

**Not landed as a default in this session.** `mahalUseR` also changes the fix point to
weighted multilateration, so it moves accuracy for two reasons at once; the registration
bars flipping it in the same breath, and a default change needs the null control,
reference regeneration and gate run the other two got.

## A correction to K5, which K7 forces

`report/K5_PROCESS_NOISE_RESULTS.md` attributed the static arms' 32–40 m movement to a
**cross-binary** effect, citing `TECH_DEBT` 4.1/4.2, and on that basis called V3
unanswerable. **That attribution is wrong**, and K7 disproves it: K7 ran on the same binary
against the same control and its static arms moved by **exactly zero**.

The real explanation is simpler and is a flaw in K5's design. **The static arms are not
policy-free — they run the same EKF, pinned to one tier.** So `processNoiseStd*` reaches
them directly. They cannot serve as a displacement floor for a process-noise sweep, because
the treatment applies to them too.

Two consequences:

- K5's floor was **invalid**, not merely large.
- V3 is therefore **answerable after all, and it goes the other way**: raising `Q` made
  `ca_tbl_full` *worse* — +9.07 m at ×2 (15+/5−, p 0.041) and +24.2 m at ×4. I recorded
  that as "not resolvable" when it was a real effect pointing away from `Q` as the fix.

K5's headline — process noise is implicated but explains only a third of the excess —
survives, and K7 supersedes it: the missing R term explains far more.

---

# K6 — the envelope exists, and the long-range tier owns it

`boxHalf` 4000–6000, `gwGrid=3`, the `k1a` configuration. Gateway spacing 3600–5400 m
against 868 r80 of 1515 m.

| | box4000 | box5000 | box6000 |
|---|---|---|---|
| `static_868` fix cycles | 1346 (93%) | 1264 (88%) | **1020 (71%)** |
| `static_2p4` fix cycles | **94 (6.5%)** | **10 (0.7%)** | **0** |
| `static_2p4` rmse | 2110 m | 2050 m | 2148 m |
| `ca_tbl_full` rmse | 559 m | 530 m | 735 m |
| `ca_tbl_full` vs `static_2p4` | **DOMINATES** | **DOMINATES** | **DOMINATES** |
| `ca_tbl_full` vs `static_868` | DOMINATED | DOMINATED | DOMINATED |

| # | prediction | outcome |
|---|---|---|
| X1 | `static_868` coverage below 90% by box6000 | **HELD** — 71% |
| X2 | `static_2p4` coverage below 25%, rmse rises faster | **HELD** — 0–6.5% |
| X3 | `ca_tbl_full` non-dominated by `static_2p4` in at least one cell | **HELD** — it **dominates** in all three |
| X4 | `ca_tbl_full` remains non-dominated by `static_868` | **FAILED** — dominated in all three |

**X3 is the answer to step 1: the envelope exists.** For the first time in this project,
`ca_tbl_full` beats a single 2.4 GHz radio — by 1032–1522 m of accuracy, at equal or better
energy. Above `boxHalf=4000` the mid-band is simply not a system: 0–94 fix cycles of 1440.

**And it does not help, because X4 failed in the same cells.** `static_868` gets
statistically indistinguishable accuracy for **13.4–13.7 J against CA-TBL's 31–34 J** —
a third of the energy, 0+/20− on every cell.

## The squeeze, stated plainly

**There is no cell in this project where `ca_tbl_full` is non-dominated by every claim arm.**

- Below `boxHalf≈3000`, `static_2p4` dominates it.
- Above `boxHalf≈4000`, `static_868` dominates it.

The architecture is squeezed from both ends, and the crossover between the two single-band
regimes is not a window where tiering wins — it is where they hand over.

## Why, and it is one specific and fixable thing

At `box6000` CA-TBL escalates **199 times** into a tier with **zero** fix cycles, and pays
~18 J for it. The evidence gate should prevent exactly this — and it cannot, because of
what its own help text says:

> "Does NOT gate 868→2.4 — see `escalationRequiresEvidenceT2` for why that one does not
> work."

**The evidence gate only guards the 2.4→UWB step.** The 868→2.4 escalation is ungated, so
where tier 2 has no coverage the policy escalates into it repeatedly and is charged every
time. That is the whole of the energy gap to `static_868`, and it is the single change most
likely to make CA-TBL non-dominated at large scale.

`escalationRequiresEvidenceT2` exists and is recorded as not working. **Making it work is
the next item**, and it is now the highest-value one in the register: K6 says the envelope
is there, and this is what stops the system from owning it.
