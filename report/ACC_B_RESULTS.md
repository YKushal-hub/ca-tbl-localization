# ACC-B — results: relay legs on 2.4 GHz under the dual base quadruple tag 0's relay supply and add endgame availability, but neither endgame accuracy nor the 5 km within-25 m loss to static 2.4 GHz moves

**Registered in `report/PREREG_ACC_B_HYBRID.md` (`0f68d454`) before any cell ran.** Grade:
**exploratory**. All 80 cells exit 0 and read `source 0f68d454 dirty 0`. The registered analysis,
applied unchanged, reports "Problems: 0". Tables: `results/acc_b_2026-10-09/ACC_B.md`.
Provenance: `RUN.md` there. Paired over seeds 1-20, as mean [95 % bootstrap], with tag 0's CEP
taken per seed.

## 1. What F1 (`--dualBaseRelayTier`) does

| | endgame (B): off → on | 5 km (A): off → on |
|---|---|---|
| tag 0 relay legs offered / admitted per run | 198 / 170 → **894 / 765** | 113 / 108 → 214 / 204 |
| ranges into the EKF per cycle | 7.53 → 8.50 (+0.97 [0.73, 1.21]) | 2.82 → 2.97 (+0.16 [0.02, 0.29]) |
| five-node availability | 0.562 → **0.592** (+0.030 [0.019, 0.043]) | 0.285 → 0.286 (+0.001 [−0.012, 0.014]) |
| tag 0 CEP50 (m) | 5.7 → 5.4 (−0.36 [−0.92, 0.29]) | 24.4 → 19.2 (**−5.21 [−9.95, −1.47]**) |
| tag 0 CEP95 (m) | 12.0 → 12.2 (+0.21 [−1.76, 2.11]) | 57.8 → 52.2 (−5.58 [−15.09, 2.55]) |
| five-node within 25 m | 0.968 → 0.958 (−0.010 [−0.032, 0.013]) | 0.613 → 0.619 (+0.006 [−0.034, 0.046]) |
| J per node | 22.97 → 23.63 (+0.65 [−1.35, 2.70]) | 16.69 → 17.51 (+0.82 [0.36, 1.31]) |

- **The mechanism N.35 diagnosed is what limited tag 0's relay supply.** With F1, tag 0's
  endgame legs reach 74 % of static 2.4 GHz's (894 against 1 202).
- **The extra supply buys availability, not accuracy, in the endgame.** The endgame is
  saturated: at the 3 m floor, CEP50 is 5.4 m against static 2.4 GHz's 4.9 m. At 5 km it buys
  5.2 m of CEP50.
- **Cost:** +0.82 J per node at 5 km (+5 %). In the endgame the energy change is not
  significant.
- **Isolation:** `static_2p4` is byte-identical on and off, in every file of every seed.

## 2. Against static 2.4 GHz (with F1 on)

| | A | B |
|---|---|---|
| availability | +0.256 [0.227, 0.287] | +0.321 [0.290, 0.351] |
| five-node within 25 m | **−0.064 [−0.122, −0.012]** (off: −0.071) | −0.010 [−0.030, 0.009] |
| tag 0 CEP50 | −5.22 [−23.58, 5.58] | +0.50 [−0.07, 1.13] |
| tag 0 CEP95 | −2.73 [−21.85, 15.61] | **+1.40 [0.07, 2.69]** |

The 5 km within-25 m loss is the one Part B set out to close, and it stays. It comes from the
subjects, whose median errors F1 does not reduce. That is H2 (N.35), which F1 does not address.
At the 3 m floor, static 2.4 GHz is 1.4 m more accurate on endgame CEP95. TOF-FIELD shows that
this lead reverses at measured field error.

## 3. Registered predictions

| # | prediction | verdict |
|---|---|---|
| Q1 | B relay legs at least double and admitted > 0 | **held** (×4.5; +596 [525, 672]) |
| Q2 | B ranges into the EKF per cycle up | **held** |
| Q3 | B CEP50 and CEP95 each ≥ 10 % lower | **failed** (−0.36 and +0.21 m, neither significant) |
| Q4 | A within 25 m against static 2.4 GHz: no significant loss | **failed** (−0.064 [−0.122, −0.012]) |
| Q5 | availability not down by > 0.01; J ≤ +15 % | **held** (+5 % at A, n.s. at B) |
| Q6 | static 2.4 GHz byte-identical on and off | **held** (0 differences) |

## 4. What this cannot establish

- **No `fixed_threshold` arm.** F1 applies to it too, as a hybrid arm, so its effect on the
  decision-rule comparison is not measured.
- **No per-range log (B1).** "Precise ranges" is replaced by EKF ranges and relay legs.
- **H2 (the subjects' extra tier-1 / TDoA fixes) is untested.** F2 was not built.
- **One machine, exploratory grade.** Adoption is S1's call. The evidence for adopting F1 is the
  +0.030 endgame availability at +3 % energy (n.s.). Against it: no endgame accuracy gain, and
  +5 % energy at 5 km.
