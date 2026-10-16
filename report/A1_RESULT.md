# A1 — precision-tier coverage against dwell: measured, and the verdict does not move

> **Local line, legacy topology.** Ported to `main` on 2026-10-16 as a record (`docs/LOCAL_LINE_INTEGRATION.md`). Every number below was produced on the local line's binary and the pre-correction topology `main` treats as a different deployment (`CLAUDE.md`), on commits that resolve through `SHA_MAP.md` to `archive/local-line-2026-10-16`. Flags it names (`--imuPrior`, `--intraCycleHandshake`, `--imuCoastMomentMatch`) exist on that branch, not on `main`. Its registered runner is the archive branch's `scripts/sweeps/run_a1_dwell.sh`, imported here as `scripts/sweeps/run_a1_dwell_local.sh`: `main`'s file of the original name is a different, earlier A1 sweep.

Registered in `report/PREREG_A1_DWELL.md` before any cell; run in the queue at `c620352`,
100/100 invocations, every sidecar `dirty 0`, digests verified; scored once by the registered
scorer, unedited (`report/a1/`). Configuration: `boxHalf = 3000`, `gwGrid = 5`, propagation NLOS,
evidence gate on, `pcsForm = min`, IMU off; seeds 501–520; arms `ca_tbl_full`, `static_2p4`,
`fixed_threshold`, `static_868`.

## Registered outcomes

| | prediction | outcome |
|---|---|---|
| P1 | `f = d / (d + T)`, T fitted on 90 s alone, predicts 150/245/400/800 s within the CI or 1 pp | **FAILS at 400 s** — predicted 0.1232, measured 0.1120 [0.1020, 0.1227]: 0.05 pp above the CI's upper end and 1.12 pp from the mean. Holds at 150, 245 and 800 s |
| P2 | mean `f` strictly increasing in dwell | **HOLDS** |
| verdict | not predicted | **A0 v2 FAIL at every dwell** |

### Coverage fraction (cycles inside three-anchor UWB coverage / 1440)

| dwell | measured `f` | predicted (T = 2845 s) |
|---|---|---|
| 90 | 0.0307 [0.0285, 0.0330] | fit point |
| 150 | 0.0465 [0.0427, 0.0504] | 0.0501 |
| 245 | 0.0715 [0.0651, 0.0783] | 0.0793 |
| 400 | 0.1120 [0.1020, 0.1227] | 0.1232 — **miss** |
| 800 | 0.2170 [0.1965, 0.2384] | 0.2195 |

The model over-predicts at every intermediate dwell (by 0.4–1.1 pp) and lands at 800 s, so the
miss is the largest of a consistent bias, not an outlier. Coverage does rise with dwell
(P2) and is the right order of magnitude, but a single transit time does not carry it to within
the registered tolerance. Under the registration's own wording, the work package's
dwell/transit duty-cycle mechanism is not confirmed.

### A0 v2 (primary: `A_fix`, radio energy; κ = 1)

| dwell | `D_T` / `D_B` | ρ radio | allowance | verdict |
|---|---|---|---|---|
| 90 | 10 / 0 | 2.790 [2.712, 2.873] | 1.50 | FAIL |
| 150 | 11 / 0 | 2.796 [2.712, 2.882] | 1.55 | FAIL |
| 245 | 12 / 0 | 2.797 [2.716, 2.879] | 1.60 | FAIL |
| 400 | 10 / 0 | 2.818 [2.743, 2.896] | 1.50 | FAIL |
| 800 | 14 / 1 | 2.837 [2.755, 2.921] | 1.65 | FAIL |

**κ band, derived, not printed.** `PREREG_A0_V2.md` requires the {0.25, 0.5, 1, 2} band always to
be reported; the A1 scorer prints only κ = 1 — a gap in the scorer, recorded here rather than
fixed after the first cell. The band follows from the printed numbers without reading any data:
`allow = min(2.0, 1 + κ · dD / N)` is at most 2.0 for every κ, and the lowest ρ lower bound at
any dwell is 2.712. **FAIL at every κ, every dwell.** (Allowances in the table: `dD = D_T − D_B`,
N = 20.)

## What this says

- **The tiered arm is the only one that meets the requirement here, and it costs ~2.8× to do
  it.** `ca_tbl_full` meets (A = 0.95, R = 25 m) on 10–14 of 20 seeds; `static_2p4` on 0–1. The
  registered rule caps what a reliability gain can buy at 2× energy, and the premium is above
  that cap at every dwell — so the verdict is FAIL whatever the reliability difference.
- **Dwell does not move the verdict.** Raising time in precision-tier coverage seven-fold
  (`f` 3 % → 22 %) moves `D_T` from 10 to 14 and ρ from 2.79 to 2.84. The registration's
  reading — "a PASS at long dwell says the architecture pays where the node spends a large
  fraction of its time in coverage" — does not arise: there is no crossing dwell up to 800 s.
- **This configuration has tier 2 ~6 dB deaf (D-8).** `static_2p4`'s 0/20 at `boxHalf = 3000`
  is measured with the 1625 kHz sensitivity row at 800 kHz. D-8 stage 1 re-measures exactly this
  configuration with the fix; until it is scored, `D_B` here is a lower bound on what a
  correctly configured tier 2 achieves, and the A0 comparison is against a handicapped baseline.
  **Since measured** (`report/D8_STAGE1_RESULT.md`, same flags at dwell 90 but all six arms, seeds
  701–720): with the fix, `static_2p4` meets the requirement on **16/20** seeds (0/20 without),
  `ca_tbl_full` on 19/20, ρ = 2.34 — still FAIL, but no longer because only the tiered arm can
  meet the requirement.
- **At 90 s, `f` = 3.1 %** — under the verifier's 5 % floor for the escalation decision to be
  exercised (audit D6). The 90 s and 150 s cells say little about tier 3's decision logic.

## What it cannot establish

Anything about adding facilities (dwell and site count both raise `f` and are not the same
intervention); anything at another `R`; anything under the D-8 fix.
