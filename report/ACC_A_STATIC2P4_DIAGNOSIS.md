# ACC-A static 2.4 GHz diagnosis — what the hybrid buys over static 2.4 GHz, and why it loses within 25 m at 5 km

**Post-hoc and exploratory, not registered.** Written 2026-10-09 from ACC-A's traces (M cells,
k = 10, seeds 1-20). No cell was run for this. Script:
`scripts/acc_a_static_diag.py <ACC-A outdir>`. It reads only and prints every number below except
the scoreboard, which is ACC-A §5 and `results/acc_a_2026-10-09/ACC_A.md`.

## 1. Scoreboard (ACC-A, k = 10, `ca_tbl_full` − `static_2p4`, at the 3 m tier-2 floor)

| | 5 km (A) | endgame (B) |
|---|---|---|
| five-node availability | 0.278 vs 0.024: **+0.254** | 0.575 vs 0.268: **+0.307** |
| five-node within 25 m | 0.638 vs 0.705: **−0.067 [−0.128, −0.015]** | 0.977 vs 0.969: tie |
| tag 0 CEP50 / CEP95 | tie / tie | tie / tie |
| J per node | 17.29 vs 14.28: +3.01 | 26.01 vs 5.67: **+20.34 (×4.6)** |

Under Mueller's measured outdoor ToF error (TOF-FIELD, rural), static 2.4 GHz's endgame CEP95 is
**30.7 m [22.6, 39.2] worse** than the preset's (70.0 against 39.3 m). At 5 km the two do not
separate on accuracy in any setting.

## 2. Every estimate starts at, or next to, the true position

- **Tag 0** is seeded at base camp (`scratch/ca_tbl_causal_sim.cc:6709`), which is also where it
  starts. Initial error: median 3.0 m.
- **Subjects 2-5** are seeded at their true position (`:7162`, `:8758`). Initial error: median
  0.1 m.

The scenario never asks either arm to *acquire* an unknown position.

## 3. At 5 km, static 2.4 GHz's within-25 m share is dead reckoning from that start

| subjects 2-5, 5 km | `ca_tbl_full` | `static_2p4` |
|---|---|---|
| fix share | 0.186 | **0.008** |
| no fix in the 1 800 s run | 9 of 80 | **42 of 80** |
| first fix, median / p90 | 215 / 848 s | 515 / 1 003 s |
| within 25 m | 0.612 | 0.690 |
| median error coasting < 60 s after a fix | **13.0 m** | 5.4 m |
| median error coasting 300-900 s | 29.2 m | 15.3 m |
| fixes leaving the estimate worse | 868 MHz-solved 76 %, 2.4 GHz-solved 60 % | 48 % |

**Static 2.4 GHz:**
- On 42 of 80 subject-runs it never gets a fix.
- Even so, it is within 25 m 69 % of the time. That is the seeded start plus the IMU model,
  which carries a per-run gyro bias and a 2.66 % step-length error.
- Its few fixes are close-range 2.4 GHz solves that neither help nor hurt (48 % worse).

**The hybrid:**
- It fixes 23× more often.
- Its subjects sit further from truth between fixes: 13.0 m against 5.4 m within a minute of a
  fix.
- Each fix moves the estimate by a fraction of a metre, but more often away from the truth than
  toward it (60-76 %). The frequent fixes do not correct the estimate; they hold it slightly off.

**Hypothesis (not tested):** the subjects' tier-1 and dual-base updates are over-weighted against
their true error. ACC-A §2 measured tag 0's NIS > 9.21 share at 0.42, where a consistent filter
gives 0.01. That fits, but the subjects' NIS was not measured here.

## 4. What the hybrid buys, on this evidence

- **A position at all, at range.** Subjects without a fix in 30 min at 5 km: 9/80 against 42/80.
  Under a known start this barely shows in within 25 m. Under an unknown start it would be the
  difference between a position and none, but that case is not simulated.
- **Faster first fix in the endgame.** Subjects' median 42.5 s against 95 s; p90 160 s against
  527.5 s.
- **Robustness to field 2.4 GHz ranging error** (TOF-FIELD §1).
- **Not:** accuracy at the 3 m floor, or energy. At 5 km it costs +3.0 J per node; in the endgame
  it costs 4.6× more.

## 5. What this cannot establish

- **Post-hoc, one machine, k = 10 only.**
- **The unknown-start case is not simulated.** §4's first point is an inference from fix counts,
  not a measurement of within 25 m under it.
- **The IMU model is the project's own** (2.66 % step-length error is OURS). A worse IMU would
  shrink static 2.4 GHz's coasting advantage, but by how much is not measured.
