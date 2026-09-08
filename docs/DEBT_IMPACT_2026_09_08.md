# How much is the tech debt actually affecting this?

Not a survey of the register — an attribution of what went wrong **today**, item by item,
with what each one cost.

---

## 1. The debt that produced wrong answers

| # | item | in the register before today? | what it cost |
|---|---|---|---|
| 1 | **Environment split** — tiers 2/3 simulated in open scrub, tier 1 in dense forest | **No** | 2.4 GHz got **7.4× its true range** and out-ranged 868. Made the domination question *unanswerable*, and invalidated every tier-2 and tier-3 number in the tree. |
| 2 | **Casualty inside the gateway footprint** — the scenario is an enclosure, not a search | **No** | K1, K4 and K6 — 16 cells, ~6 hours of compute — all answered a question that could not test the claim. |
| 3 | **`mahalUseR=false`** — integrity gate's innovation covariance omits its R term | **No**, though the source called the alternative "the correctly specified integrity gate" | Suspect rate **32.3% against a 1% design rate**; up to 209 m and 5.7 J of accuracy and energy. |
| 4 | **`escalationRequiresEvidenceT2` broken** | **Yes**, fully, with the fix specified | ~18 J at large scale — the entire energy gap to `static_868`, which is what dominated CA-TBL there. |
| 5 | **Foliage depth ignores link length** | **No** | ~12.8 dB on UWB, a 3.6× range factor. Tier 3 obtained **zero** fixes in every cell of every campaign. |

**Four of these five were not in the register.** That is the finding. The register is
detailed, honest and well maintained, and it was tracking the wrong things — it had 40+
items about *known* gaps and missed five defects that were each individually decisive.

## 2. The pattern, which is sharper than the list

Every defect that cost something today is a **default that was known to be wrong and left
on for reproducibility**:

| default | what its own comment says |
|---|---|
| `envFacility` = RURAL_OPEN | "facility sites are clearings" — true of a topology deleted in 2026-09-07 |
| `mahalUseR` = false | "false reproduces the pre-existing baseline" |
| `escalationRequiresEvidence` = false | (worth **324 J**, off) |
| `escalationRequiresEvidenceT2` = false | "BROKEN — see the header. Never enable." |

The discipline that produced these — never change a default without a null control — is
**right**, and it is why every fix today landed safely with a 3-of-3 byte-identical
control. But it has no counterpart: nothing ever ages a default out. So four wrong
defaults accumulated, and the *combination* is what made the system look beaten.

**The missing rule is not "change defaults faster". It is that a default kept for
reproducibility needs an expiry condition** — the run it reproduces, and what has to be
true before it flips. `escalationRequiresEvidenceT2` is the model: it named its
precondition ("only matters where tier 2 fails"), K6 made that true, and the fix followed
in an hour. None of the other three named one.

## 3. Debt that cost time rather than correctness

| item | cost |
|---|---|
| **Stale derived defaults** — `boxHalf=1000` commented "868 r80 = 1000 m" (measured 1515 m); `t2TriadRadiusM=150` commented "2p4 r80 ≥ 900 m" (measured 156 m) | Sized K1's sweep from them; the envelope had to move **twice**, both moves recorded in Amendment 1 |
| **Cost per seed uncharacterised** | Discovered mid-campaign that `boxHalf=200` costs 13× `boxHalf=2500`. **Seven cells killed**, ~1 hour of compute |
| **`TECH_DEBT` 1.2** — rule 8 does not cover the cross-binary case | Primed me to mis-attribute K5's static-arm drift to a binary change when the real cause was that static arms run the same EKF. One wrong paragraph, corrected by K7 |

**Total compute wasted: roughly two hours** across three discarded launches and seven
killed cells. Against ~9 hours of campaign time, that is ~20%.

## 4. Debt that did NOT bite, and it is worth saying so

- **`LoRaDistributionSampler` ignores its seed** (§5, recorded as a decision not to fix) —
  determinism held throughout; no result is known to be affected.
- **G11's model-versus-engine disagreement** (J.1) — stayed red, blocked nothing, and the
  discipline it protects is exactly what found defect 1.
- **The SHS/IMU port** — checked directly today. It has its own explicitly-seeded stream,
  creates **zero** random variables of its own, and its verdict is double-guarded. It costs
  compute and nothing else.
- **The attribution rewrite and `SHA_MAP`** — irrelevant to any question asked today.

## 5. The honest bottom line

**The debt did not slow this work down much. It made the answers wrong.**

Roughly 20% of compute was wasted on scoping mistakes, which is survivable. What was not
survivable is that the headline verdict — "the system does not outperform" — was produced
on a broken link budget in a scenario that could not test it, and the project's strongest
prior result (the integrity check, 8/8 seeds) **reversed sign** once one of the five was
fixed.

**None of the five was hard to fix.** Two were one-line configuration changes, one was a
flag that already existed with a correct implementation behind it, one was a predicate the
header had already specified, and one is a `std::min`. Each took under an hour. They were
expensive because they were **unknown**, not because they were difficult — and four of five
were invisible to a register, a 15-gate suite and a 141-check verifier that were all
working correctly.

**What would have caught them:** every one surfaced from a *measurement that refused to
reconcile* — the probe's r80 table, a suspect rate 32× its design value, an arm with 59 fix
cycles. Not from a gate. `TECH_DEBT` J.1 already says this about the analytic model, and it
generalises: **the instruments that find defects here are the ones that compute a quantity
two ways and compare.** The project has one of those and should have more.
