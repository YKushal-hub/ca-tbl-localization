# D-8 stage 1 — tier 2 on its own bandwidth's sensitivity row: measured

> **Local line, legacy topology.** Ported to `main` on 2026-10-16 as a record (`docs/LOCAL_LINE_INTEGRATION.md`). Every number below was produced on the local line's binary and the pre-correction topology `main` treats as a different deployment (`CLAUDE.md`), on commits that resolve through `SHA_MAP.md` to `archive/local-line-2026-10-16`. Flags it names (`--imuPrior`, `--intraCycleHandshake`, `--imuCoastMomentMatch`) exist on that branch, not on `main`. The defect it measures is present on `main` too (`docs/TECH_DEBT.md` N.27); a campaign on `main`'s configuration has not been run, and must be registered before it is (`docs/LOCAL_LINE_INTEGRATION.md` §4).

Registered in `report/PREREG_D8_SENSITIVITY.md` (`c620352`) before any cell; flag
`--t2SensitivityRowKHz` landed at `395cd64` with its null control. Run in the registered queue at
`c620352`: 2 probes + 80 campaign cells, every sidecar `dirty 0`, digests verified, **80/80
usable**; scored once by the registered scorer, unedited (`report/d8/`).

**Deviation, recorded.** The machine shut down at 2026-10-15 11:26:27 (system log), four minutes
into this campaign. Both probes had completed and were kept. The 11 cells then running
(`ref_off_s701`–`s711`) had no digest, so none had produced a result; they were moved aside
unmodified (outside the repository, with a note) and re-run at the same commit, binary and
arguments. No completed cell was replaced. The VM was later suspended for several hours with three
cells in flight; suspension pauses a process and does not change a discrete-event simulation's
output, and those cells completed with verified digests.

## Registered outcomes

| | prediction | outcome |
|---|---|---|
| abort check | fix-off probe reproduces `probe_v7` r80 to 0.1 m | **reproduced** — 1515.3 / 1149.5 / 12.8 m |
| P1 | tier-2 r80 ratio on/off in [1.4, 2.1] | **FAILS** — 1149.5 → 1421.6 m, **×1.237** |
| P2 | tiers 1 and 3 r80 move < 1 % | **HOLDS** — both ×1.000 |
| P3 | `static_2p4` `A_fix` rises at `boxHalf = 3000`, CI above 0 | **HOLDS** — +0.159 [+0.134, +0.183] (0.798 → 0.957) |

### Why P1 failed — the basis was wrong, not the fix

The fix delivers its full 6 dB: the probe's mean link margin rises **+6.0 to +6.4 dB at every
distance** from 600 to 3000 m. What the registration got wrong is the slope it converts through.
It assumed an effective path-loss exponent between 2 and 3.5. In the engine, **96–98 % of tier-2
samples at these distances are two-ray**, past the breakpoint where loss goes as d⁴: mean path
loss rises 4.9 dB from 1200 to 1600 m and 3.9 dB from 1600 to 2000 m, about **40 dB/decade**.
At that slope, 6 dB buys 10^(6/40) = **1.41×** — already outside the registered band.

The rest (1.41 → 1.24) is consistent with the margin needed for PDR 0.8 rising by about 1 dB
between the two r80 points (≈ 7.9 dB at 1149.5 m, ≈ 8.9 dB at 1421.6 m, interpolated), as the
spread widens (shadow σ_eff 5.28 → 5.42 dB) and the NLOS fraction grows (1.9 % → 3.1 %). That
attribution is plausible, not measured term by term. The registered falsifier stands: **P1
fails.** Tier 2's corrected r80 is 1421.6 m.

## Every arm, paired on − off (n = 20)

**`C_ref`, `boxHalf = 2000`**

| arm | `A_fix` (off) | Δ `A_fix` | Δ cep50 | radio J | Δ escalations |
|---|---|---|---|---|---|
| `static_868` | 0.0854 | +0.0004 [−0.0067, +0.0069] | −1.61 m | ×0.9995 | 0 |
| `static_2p4` | 0.9973 | +0.0025 [+0.0015, +0.0037] | −0.11 m | ×1.2094 | 0 |
| `static_uwb` | 0.0603 | **−0.0037 [−0.0056, −0.0019]** | +22.1 m | ×1.0052 | 0 |
| `fixed_threshold` | 0.9974 | +0.0016 [+0.0010, +0.0022] | −0.13 m | ×1.0640 | +2.5 |
| `ca_tbl_no_integrity` | 0.9990 | +0.0000 [−0.0006, +0.0006] | −0.08 m | ×1.0967 | +0.2 |
| `ca_tbl_full` | 0.9977 | +0.0011 [−0.0005, +0.0030] | −0.06 m | ×1.0955 | +0.3 |

**`C_sparse`, `boxHalf = 3000`**

| arm | `A_fix` (off) | Δ `A_fix` | Δ cep50 | radio J | Δ escalations |
|---|---|---|---|---|---|
| `static_868` | 0.0426 | +0.0022 [−0.0039, +0.0081] | −1.53 m | ×1.0002 | 0 |
| `static_2p4` | 0.7984 | **+0.1585 [+0.1342, +0.1833]** | −0.67 m | ×1.3775 | 0 |
| `static_uwb` | 0.0290 | −0.0019 [−0.0048, +0.0005] | +32.7 m | ×1.0040 | 0 |
| `fixed_threshold` | 0.9279 | +0.0570 [+0.0366, +0.0835] | −0.75 m | ×1.0968 | +5.0 |
| `ca_tbl_no_integrity` | 0.9474 | +0.0435 [+0.0239, +0.0694] | −0.78 m | ×1.1329 | −15.3 |
| `ca_tbl_full` | 0.9422 | +0.0444 [+0.0239, +0.0701] | −0.75 m | ×1.1326 | −15.6 |

(`A_fix` (off) is the fix-off mean over the 20 paired seeds, as the scorer prints it; the on value
is off + Δ.)

A more sensitive tier-2 receiver hears more, so it ranges more: tier-2 radio energy rises on every
arm that uses tier 2, by ×1.21 at ref and ×1.38 at sparse on `static_2p4`.

## A0 v2 under the fix (primary: `A_fix`, radio energy; T = `ca_tbl_full`, B = `static_2p4`)

| cell | `D_T` / `D_B` | ρ radio | allowance (κ = 1) | verdict |
|---|---|---|---|---|
| ref, fix off | 20 / 20 | 2.151 [2.116, 2.187] | 1.00 | FAIL |
| ref, **fix on** | 20 / 20 | 1.948 [1.925, 1.973] | 1.00 | **FAIL** |
| sparse, fix off | 13 / 0 | 2.848 [2.781, 2.926] | 1.65 | FAIL |
| sparse, **fix on** | 19 / 16 | 2.341 [2.284, 2.410] | 1.15 | **FAIL** |

**κ band, derived.** The scorer prints κ = 1 only, as A1's does. The band follows from the printed
numbers: at ref `dD = 0`, so every κ allows 1.0; at sparse with the fix, `dD = 3` allows at most
1 + 2 × 0.15 = 1.30 (κ = 2); fix off, at most 2.0. Every ρ lower bound is above its largest
allowance. **FAIL at every κ, in all four cells.**

**What the fix changes is the reason for the FAIL at sparse.** With tier 2 6 dB deaf, `ca_tbl_full`
met the requirement on 13 seeds and `static_2p4` on none: the tiered arm was the only way to meet
it, and failed on price. With tier 2 configured correctly, `static_2p4` meets it on **16 of 20**
seeds by itself, the tiered arm on 19, and the premium for those three seeds is 2.34×. The
architecture's reliability advantage at this density was, in large part, the defect.

The ratio falls (2.15 → 1.95, 2.85 → 2.34) because the baseline's own energy rises more than the
tiered arm's — a better baseline is also a more expensive one.

## The static-arm identity question (reported, not asserted)

The registration expected `static_868` and `static_uwb` rows might differ through the shared MAC
stream, and asked for the difference to be reported with its mechanism. They differ in **every**
seed (0/20 identical rows). Measured:

- **The energy invariant holds.** `static_uwb` is charged a constant 0.014256 J on tiers 1 and 2
  in both conditions (0 cycles, 0 tier-2 tx/rx, 0 tier-2 neighbours); all of its change is in UWB
  energy, and UWB energy rose on 12/20 seeds at ref and 10/20 at sparse — noise.
- **Collisions do not cross bands.** `CaTblMac`'s overlap set skips any transmitter on another
  tier, so the extra tier-2 traffic (`mac_tx` in `static_uwb`'s row, 152 815 → 188 775 on seed 701;
  `mac_collisions` 20 263 → 32 297) cannot collide with UWB.
- **But `static_uwb`'s `A_fix` falls at ref with a consistent sign**: 15 of the 19 seeds that
  changed lost fix cycles (≈ 5 of 1440 each; sign test p ≈ 0.02). Shared-stream perturbation alone
  predicts no sign. At sparse it is 11 against 9.

**The route, traced** (seed 706 at ref, the largest change, read from the two traces): the
runs diverge at **cycle 0** (t = 2.5 s) in one column only — `macEvals`, the MAC's engine
evaluations, 537 off against 393 on. The per-tier engines are separate objects, so tier-2 link
evaluations do not consume UWB's channel draws; but **one `macRng`** (seeded once per run,
`ca_tbl_causal_sim.cc` ~:2269) supplies backoff and rebroadcast jitter to every tier's MAC and
every anchor. A tier-2 reception that now succeeds changes how many draws are taken, which moves
UWB transmission times, which moves UWB neighbour discovery: over the run `nbrUwb` and `polled`
differ on 187 cycles, `nAnchors` on 122, `hadFix` on 27; the geometric label `visUwb` on none.
That is the registered mechanism (working rule 8), now traced rather than assumed.

**Still open: the sign.** A shared-stream perturbation predicts no direction, yet at ref 15 of 19
changed seeds lost fix cycles (p ≈ 0.02; ≈ 0.04 allowing for the two configurations examined;
sparse shows 11 against 9). That is weak evidence of a direction and is left at that. Magnitude
−0.0037 `A_fix`, against effects of +0.04 to +0.16 on the arms this campaign is about; it changes
no verdict above.

## What follows

- **Stage 2 is warranted**, as registered: every tier-2 coverage figure moves, and at the sparse
  density the qualitative picture moves with it. Its registrations are written next.
- `report/A1_RESULT.md`'s sparse reading — the tiered arm the only one meeting spec — was measured
  with tier 2 deaf; this campaign shows that at `boxHalf = 3000` the baseline meets it on 16/20
  seeds once corrected. A1's coverage-fraction findings are unaffected (geometric).
- The registration's own physics was wrong in a checkable way: **convert link budget to range
  through the engine's propagation regime, not a textbook exponent band.** The probe reports the
  two-ray fraction; it should have been read first.

## What this cannot establish

Whether the headline claims survive (stage 2). Tier 3's sensitivity, still unchecked against a
data sheet (audit D5).
