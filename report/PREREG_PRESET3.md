# PREREG PRESET-3 — the preset after the owner's decisions of 2026-10-21, measured on the requirement

**Registered before any cell runs.** Written 2026-10-21. This commit adds this file, `scripts/preset3_run.sh` and
`scripts/preset3_analyse.py`, and nothing else. Outputs go to `~/ns/catbl-work/preset3_2026-10-21/`, which does not
exist at registration.

## Why

The owner's decisions of 2026-10-21 changed the preset:

| DESIGN_DECISIONS | change |
|---|---|
| 20 | the realistic world |
| 21 | PRESET-2's set, by override |
| 19 | the integrity check off |
| 22 | eight finders in pairs |

The pinned preset reproduces the old one, 274/274 files in four preset cases. This campaign measures **the size of
the decisions** and states where the requirement now stands. No adoption rule follows from it: the decisions are
taken.

## Cells

- **NEW:** the preset as it stands at this commit.
- **OLD:** NEW with decisions 19, 21 and 22 pinned back:

      --t2SensitivityRowKHz=0 --publishAfterStep=false --nlosRSource=operative --foliageScaleBounds=false
      --interferenceDistance=false --rssiRangeFromRxPower=false --pcsByteScale=1 --ekf5Window=0
      --integrityCheck=true --nFinders=4 --finderTasking=roundrobin

  The realistic world (DD 20) and the receiver corrections (DD 18) are in both, so the contrast is the decisions, not
  the world.

**Configurations:**
- A: 5 km.
- B: endgame, `--boxHalf=750 --finderMode=1`.
- C: the 8-gateway ring, `--nGateways=8 --gwLayout=legacy`.

All five arms; `--duration=1800 --trace=true --traceCovariance=true --reportAirtime=true`. Seeds 241–260, fresh:
120 cells.

## Predictions

Basis: PRESET-2 (N against S), ANCHOR-SUPPLY (P8 against R4) and INTEG3, on other seeds and other bases.

| # | prediction | basis |
|---|---|---|
| P1 | NEW raises CA-TBL five-node A(25 m) in B: interval above 0 | ANCHOR P8 − R4 +0.089; PRESET-2 +0.035 |
| P2 | NEW raises CA-TBL tag-0 A(25 m) in A: interval above 0 | PRESET-2 +0.443 |
| P3 | NEW's CA-TBL mean five-node A(25 m) in B ≥ 0.95 | ANCHOR P8 0.962 |
| P4 | NEW lowers CA-TBL five-node A(25 m) nowhere: lower bounds above −0.02 | every contributing contrast was non-negative |
| P5 | NEW's CA-TBL subject J ≤ 1.10 × OLD's in every configuration | PRESET-2 J ratio 0.83–0.96; pairs −24 % |

**If any prediction fails, it is reported as a property of the adopted preset.** The decisions are not reopened by
this campaign, and the owner sees the result.

## Abort conditions

- not 120 cells, or a cell's `rc` not 0;
- a sidecar not `dirty 0`, or more than one source;
- a manifest mismatch;
- a seed outside 241–260;
- a cell whose window or pairs lines do not match its variant;
- any window solver failure;
- an arm with fewer than 20 seeds.

---

# RESULT — after the registration above (`3013e182`)

**Run.** Launched 2026-10-20T20:54:48Z from `3013e182`, dirty 0. Done 21:55:59Z. 120 of 120 cells, every `rc` 0, one
source, no window solver failure; **problems none**. Scored once: `results/preset3_2026-10-21/PRESET3.md`.

| # | verdict | measured (CA-TBL, NEW − OLD, paired, 20 seeds) |
|---|---|---|
| P1 | HOLDS | endgame five-node +0.090 [0.052, 0.131] |
| P2 | HOLDS | 5 km tag 0 +0.358 [0.197, 0.528] |
| P3 | HOLDS | endgame five-node mean **0.982** |
| P4 | HOLDS | five-node A +0.472 [0.365, 0.582], B +0.090, C +0.350 [0.264, 0.440] |
| P5 | HOLDS | subject J ratio A 0.752, B 0.781, C 0.784: **energy falls 22–25 %** |

## The requirement under the adopted preset (CA-TBL, NEW)

| configuration | tag-0 A(25 m) | five-node A(25 m) | runs ≥ 0.95 (five-node) | CEP95 tag 0 | subject J |
|---|---|---|---|---|---|
| A, 5 km (3 gateways) | **0.979** | 0.811 | 0.15 | 8.3 m | 13.4 |
| B, endgame | **0.997** | **0.982** | **1.00** | 2.5 m | 16.9 |
| C, 8-gateway ring | **0.985** | 0.935 | 0.80 | 5.9 m | 12.9 |

- **Tag 0 meets A ≥ 0.95 in every geometry, the brief's 5 km included.**
- **The five-node pool meets it in the endgame, in every run.**
- **Short:** the five-node pool at 5 km (0.81) and on the ring (0.94).

## Against the comparators under NEW (paired, CA-TBL − comparator)

| configuration | five-node A(25 m) | tag-0 A(25 m) | subject J |
|---|---|---|---|
| **fixed_threshold** | | | |
| A | +0.002 [−0.071, 0.064] | +0.016 [0.000, 0.048] | level |
| B | −0.001 [−0.003, 0.001] | level | level |
| C | −0.012 [−0.031, −0.000] | level | level |
| **poucet_fusion** | | | |
| A | +0.077 [−0.011, 0.168] | level | **−445 J** |
| B | **+0.054 [0.030, 0.083]** | level | **−395 J** |
| C | **+0.047 [0.003, 0.091]** | level | **−445 J** |
| **static_2p4** | | | |
| A | +0.025 [−0.055, 0.101] | +0.036 [0.012, 0.067] | +7.2 J |
| B | +0.009 [−0.000, 0.022] | level | +10.2 J |
| C | +0.047 [0.004, 0.089] | +0.014 [0.006, 0.028] | +7.3 J |
| **static_868** | +0.68 to +0.80 everywhere | +0.13 to +0.96 | +4.5 to +8.4 J |

**Read plainly:**
- CA-TBL is **level with `fixed_threshold`**. On the ring's five-node pool it is marginally below (−0.012).
- It **beats Poucet fusion** on the five-node pool in the endgame and on the ring, at about 1/30 of Poucet's subject
  energy.
- It **beats both single-band arms on tag 0**.
- Against static 2.4 GHz the five-node lead is small, because eight finders now give that arm precise anchors too.

## G24's contention populations (R.15)

In the endgame NEW cells: **2 grant tie-break verdicts** and 3 arbitration pre-emptions. Under OLD: 0 and 7.

The tie-break path is reachable, and pairs tasking produces it. It is not structural.
