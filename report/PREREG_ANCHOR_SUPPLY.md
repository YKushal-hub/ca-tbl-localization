# PREREG ANCHOR-SUPPLY — finder count and finder tasking at 5 km, judged on the design requirement

**Registered before any cell runs** (rule 13). Written 2026-10-20. This commit adds this file,
`scripts/anchor_run.sh` and `scripts/anchor_analyse.py`, and nothing else. Outputs go outside the repository, to
`~/ns/catbl-work/anchor_supply_2026-10-20/`, which does not exist at registration.

**Why.** N.62 diagnosed the 5 km failure as **anchor supply**, not filtering:
- tag 0 often has one precise anchor or none;
- a single precise range leaves a ring ambiguity;
- the range innovations then corrupt heading, bias and stride scale.

Round-robin tasking sends at most one finder to each subject. Two levers follow from that, and neither is a filter:
- **tasking:** `--finderTasking=pairs` (af741829, gate G160) sends finders 2k and 2k+1 to one subject, so a tasked
  subject can range two precise anchors;
- **count:** more finders. This is a deployment requirement rather than an algorithm, framed as MASTER_PLAN.md frames
  it.

The design requirement is A ≥ 0.95 at R ≤ 25 m (`Architecture.md` §1.3). This campaign asks how far each lever moves
CA-TBL toward it at 5 km and in the endgame.

## 1. Cells (`anchor_run.sh`)

**Every cell** runs REAL-BEST's S stack in the realistic world, plus the window smoother (adopted, TECH_DEBT Q):

    --specProfile=true --duration=1800 --trace=true --traceCovariance=true --unknownStart=true --fixedVarThreshold=12
    --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2 --imuFilter=ekf5 --imuMagSigmaDeg=3
    --ekf5FinderConsider=true --ekf5RelayGate=true --ekf5Window=60 --reportAirtime=true

with all five arms.

**Configurations:**
- A: 5 km.
- B: endgame, `--boxHalf=750 --finderMode=1`.

**Variants:**

| | tasking | finders |
|---|---|---|
| **R4** | roundrobin | 4, the brief's deployment and the reference |
| **P4** | pairs | 4 |
| **R8** | roundrobin | 8 |
| **P8** | pairs | 8 |

**Seeds:** 121–140, not run by any campaign. **Total:** 2 × 4 × 20 = **160 cells**, one frozen binary checked
before every cell.

The base does not wait for PRESET-2's verdict. The levers here are deployment and tasking, and the window is
already adopted. Any later preset change is carried by FIDELITY-1 and by the regenerated references, not by this
campaign.

## 2. Quantities (`anchor_analyse.py`)

- **Primary:** A(25 m) for tag 0 and for the five-node pool (`goal_report.cell`).
- **Secondary:** the share of runs meeting 0.95, CEP95 for tag 0, and subject J.
- **Contrasts:** P4, R8 and P8 against R4, paired by seed; CA-TBL against each comparator, per variant.

## 3. Predictions, made before any cell

**Basis.** The exploratory pilot, seeds 1–6, was read at 5–4–3 seeds per cell when this was written. All figures are
CA-TBL means:

| | A tag 0 | A five-node | B tag 0 | B five-node |
|---|---|---|---|---|
| R4 | 0.940 | 0.298 | 0.993 | 0.891 |
| P4 | 0.818 | 0.254 | 0.994 | 0.877 |
| R8 | 0.978 | 0.557 | 0.987 | 0.955 |
| P8 | 0.975 | 0.640 | 0.996 | 0.982 |

| # | prediction | basis |
|---|---|---|
| **Q1** | P4 does **not** raise CA-TBL's tag-0 A(25 m) against R4 in A: the paired upper bound is below +0.02 | pilot −0.12. With four finders, pairs leaves three of five subjects with no finder, and tag 0's own pair is no better than one finder plus the field. N.62's lever needs more finders, not a different split of four |
| **Q2** | R8 raises CA-TBL's five-node A(25 m) against R4 in A: interval above 0 | pilot +0.26 |
| **Q3** | R8 raises CA-TBL's five-node A(25 m) against R4 in B: interval above 0 | pilot +0.06 |
| **Q4** | P8 raises CA-TBL's five-node A(25 m) against R8 in B: interval above 0 | pilot +0.03 at three seeds; the weakest prediction here |
| **Q5** | no variant brings CA-TBL's mean five-node A(25 m) to 0.95 in A | pilot max 0.64. **The brief's 5 km geometry does not meet the requirement with up to eight finders under either tasking** |
| **Q6** | CA-TBL's subject J is below Poucet fusion's in every configuration and variant: interval below 0 | pilot about 20 J against 460 J |

**Against them, stated now:**
- The pilot is 3–5 seeds, and rule 8 applies.
- The pilot's 5 km tag-0 figure (0.94 at R4) is far above GOAL_STATUS's post-hoc 0.43–0.54 on seeds 61–100.
  Either the seeds differ that much, or the pilot's base, with the window and three arms, does. This campaign
  measures the reference fresh, so no prediction rests on either figure.

## 4. Abort conditions

- not 160 cells, or a cell's `rc` not 0;
- a `run_id.txt` not `dirty 0`, or more than one source;
- a manifest mismatch;
- a seed outside 121–140;
- a cell whose MS-1 tasking lines do not match its variant (kind and finder count);
- base or realistic-world lines missing;
- any window solver failure;
- an arm with fewer than 20 seeds anywhere.

## 5. Disclosure

**An exploratory pilot was read before this registration.** `~/ns/catbl-work/pairs_pilot` ran:
- seeds 1–6, configurations A and B, the same four variants;
- three arms, 1800 s;
- uncommitted, and not citable.

Its readings set the predictions above, which is why the seeds here are fresh.

## 6. What this cannot establish

- **The finder count that reaches 0.95.** Only 4 and 8 are run; nothing is extrapolated between or beyond them.
- **Any tasking other than these two.** A GDOP-driven tasking (plan item 23) is not built, and its exclusion is
  recorded with its reason.
- **Real hardware.**

---

# RESULT — after the registration above (`870a037e`)

**Run.** Launched 2026-10-20T12:19:33Z from `471680a4`, dirty 0, and done 14:11:14Z. 160 of 160 cells, every `rc` 0,
one source; problems none.

**Scored once** with the committed scorer. The full output is `results/anchor_supply_2026-10-20/ANCHOR_SUPPLY.md`.

| # | verdict | measured (CA-TBL, paired, 20 seeds) |
|---|---|---|
| Q1 | **FAILS** (on width) | P4 − R4 tag 0 in A: −0.039 [−0.167, 0.072]; the upper bound is above +0.02 |
| Q2 | HOLDS | R8 − R4 five-node in A: **+0.169 [0.090, 0.247]** |
| Q3 | HOLDS | R8 − R4 five-node in B: **+0.097 [0.035, 0.170]** |
| Q4 | **FAILS** | P8 − R8 five-node in B: −0.008 [−0.048, 0.018] |
| Q5 | HOLDS | best mean five-node in A is 0.584 (P8): **the brief's 5 km geometry does not meet the requirement with up to eight finders** |
| Q6 | HOLDS | CA-TBL subject J below Poucet's everywhere (worst upper bound −337 J) |

## What it says about the requirement

CA-TBL means:

| | A tag 0 | A five-node | B tag 0 | B five-node | B runs ≥ 0.95 | B subject J |
|---|---|---|---|---|---|---|
| R4 | 0.616 | 0.410 | 0.991 | 0.873 | 0.45 | 22.2 |
| P4 | 0.578 | 0.372 | 0.992 | 0.866 | 0.35 | **14.5** |
| R8 | 0.689 | 0.579 | 0.996 | **0.970** | 0.80 | 23.5 |
| P8 | 0.717 | 0.584 | 0.996 | 0.962 | 0.85 | 17.8 |

- **Eight finders meet the five-node requirement in the endgame on average (0.970).** It is the first configuration
  in the project to do so. A deployment of eight finders is the requirement-level result: four do not suffice (0.873).
- **Pairs tasking buys energy, not availability.** In the endgame, P4 − R4 is **−7.72 J [−9.34, −6.00]** (−35 %) at
  unchanged availability. At 5 km it neither helps nor clearly hurts.
- **5 km stays short.** With eight finders, the five-node pool reaches 0.58. That is anchor supply beyond what finder
  count alone fixes in this geometry. PRESET-2 shows the 8-gateway ring and the N set matter more there.
- **Against comparators.** CA-TBL beats Poucet fusion on five-node A(25 m) in every endgame variant (+0.06 to +0.17,
  every interval above 0) and at 5 km under R4 (+0.093). It does so at about 1/20 the subject energy.

## G24's contention populations (TECH_DEBT R.4)

- **Arbitration pre-emptions:** CA-TBL had 17 in 80 endgame cells, 5 of them under R4, the brief's deployment.
  The mechanism has a population. G24's zero comes from its three short seeds.
- **Grant tie-break verdicts:** 0 in all 160 cells, every arm. Not shown reachable, and not claimed structural.
