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
