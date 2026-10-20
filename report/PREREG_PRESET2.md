# PREREG PRESET-2 — every adoption candidate against the current reference, judged on the design requirement

**Registered before any cell runs** (rule 13). Written 2026-10-20. Engine: the commit this file is committed on
(`--pcsByteScale`, N.7, gate G159), after the merge of the 2026-10-18/19 debt pass (`c8e6116d`). This commit adds this
file, `scripts/preset2_run.sh` and `scripts/preset2_analyse.py`, and nothing else. Outputs go outside the repository,
to `~/ns/catbl-work/preset2_2026-10-20/`, which does not exist at registration.

**Why.** Each candidate below was measured on its own and judged good. Several are physics corrections that are not
proposals at all. None is in `--specProfile`, so every claim still comes from a preset with known defects. This
registration decides them together, on the design requirement itself (`Architecture.md` §1.3: A ≥ 0.95 at
R ≤ 25 m), read with `scripts/goal_report.py`.

## 1. Cells (`preset2_run.sh`)

**Every cell** runs REAL-BEST's S stack in the realistic world:

    --specProfile=true --duration=1800 --trace=true --traceCovariance=true --unknownStart=true --fixedVarThreshold=12
    --imuGyroBiasSigmaDps=0.15 --imuStrideSigmaPct=5 --finderPosSigma=2 --imuFilter=ekf5 --imuMagSigmaDeg=3
    --ekf5FinderConsider=true --ekf5RelayGate=true

with all five arms.

**Configurations:**
- A: 5 km.
- B: endgame, `--boxHalf=750 --finderMode=1`.
- C: the 8-gateway ring, `--nGateways=8 --gwLayout=legacy`.
All three with `--reportAirtime=true`.

**Variants:**
- **S:** as given, the current reference.
- **N:** S plus every candidate:

| flag | what it does | evidence |
|---|---|---|
| `--ekf5Window=60` | the sliding-window smoother | EKF5-WINDOW, EKF5-WINDOW-END |
| `--t2SensitivityRowKHz=812.5` | tier 2 on its data-sheet row | D8-MAIN, N.27 |
| `--publishAfterStep=true` | entries carry their own stamp's estimate | T2, G156 |
| `--nlosRSource=detector` | no NLOS truth in the filter | DEBT-A D1 |
| `--foliageScaleBounds=true` | foliage scaled as the parameter file states | DEBT-A, N.41 |
| `--interferenceDistance=true` | interferers attenuated by distance | DEBT-A, M.1 |
| `--rssiRangeFromRxPower=true` | RSSI ranges carry foliage and fading | DEBT-A, O.9 |
| `--silenceStopsRelay=true --deadBatteryNoDraw=true` | dead nodes neither relay nor draw | N.1, N.20 |
| `--pcsByteScale=4` | the requester rule can refuse | N.7, G159 |

- **R:** N with arbitration V2, requester escalation and queue gossip off (DEBT-A's ARBOFF set,
  `--arbitrationV2=false --requesterEscalation=false --gossipQueue=false`).

**Seeds:** 101–120. **Total:** 3 × 3 × 20 = **180 cells**, one frozen binary checked before every cell.

## 2. Quantities (`preset2_analyse.py`)

- **Primary:** A(25 m), the share of all cycles within 25 m, for tag 0 and for the five-node pool, from the traces
  (`goal_report.cell`).
- **Secondary:** CEP95 tag 0 and subject J.
- **Contrasts:** N − S and R − N paired by seed (`camp_spec_analyse.ci`), and CA-TBL − each comparator under N.

## 3. Predictions, made before any cell

| # | prediction | basis |
|---|---|---|
| **P1** | N raises CA-TBL's five-node A(25 m) against S in B: interval above 0 | the window alone, post hoc: +0.070 [0.016, 0.130] (seeds 61–80), +0.031 [−0.043, 0.093] (81–100) |
| **P2** | N raises CA-TBL's tag-0 A(25 m) against S in A: interval above 0 | the window alone, post hoc: +0.115 [0.016, 0.234] |
| **P3** | N lowers CA-TBL's five-node A(25 m) in no configuration: every lower bound above −0.02 | no candidate measured a loss of within-25 m availability |
| **P4** | N raises CA-TBL's subject energy by at most 10 % in every configuration: mean J ratio N/S ≤ 1.10 | the window and D-8 lower or keep energy; interferenceDistance and foliage change the channel both ways |
| **P5** | R against N in B: CA-TBL subject J lower (interval below 0), and five-node A(25 m) not lower (lower bound above −0.02) | DEBT-A ARBOFF: −18 % endgame energy, no accuracy loss. That was measured before N.7, so under N the requester rule can refuse, and the arbitration may now earn its energy |

**Against them, stated now:**
- P1 and P2 rest on post-hoc readings of the window alone. The second endgame reading's interval spans 0.
- The other candidates' effects on A(25 m) were never measured: DEBT-A read CEP95 and fix share.
- `--rssiRangeFromRxPower` made `static_868` worse in DEBT-A (448 → 494 m).
- A bundle cannot attribute a change to one component. That is accepted here, because each component was measured
  alone before.

**Decision rules.**
- **P1, P2, P3 and P4 all hold:** N's settings become `--specProfile`'s, the references are regenerated, and every
  later campaign uses them.
- **P5 holds:** arbitration V2, requester escalation and queue gossip leave the preset; their flags stay.
- Whatever fails stays out, with its reason recorded in `docs/DESIGN_DECISIONS.md`.

## 4. Abort conditions

- not 180 cells, or a cell's `rc` not 0;
- a `run_id.txt` not `dirty 0`, or more than one source;
- a manifest mismatch;
- a seed outside 101–120;
- a variant or realistic-world line that does not match its cell;
- any window solver failure;
- an arm with fewer than 20 seeds in any configuration and variant.

## 5. Disclosure

Seeds 101–120 have not been run by this line or the merged one. Read before this commit:
- every registered campaign through EKF5-WINDOW-END;
- the merged pass's DEBT-A and INTEG2 results;
- `report/GOAL_STATUS.md`'s post-hoc A(25 m) readings.

## 6. What this cannot establish

- Each candidate's separate contribution to A(25 m).
- Real hardware.
- Wolf ToF (`--tofErrorModel`): the preset's ToF floor stays 3.0 m (N.40).
- The code defaults, which stay as they are for the null controls.
