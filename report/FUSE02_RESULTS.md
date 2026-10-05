# FUSE-02 — results: the narrowed fuse raises availability in both configurations and brings UWB-delivered fixes to 15 endgame seeds, but tag 0's 5 km CEP95 is worse by a mean +114 % per seed; the registered rule does not adopt it, and N.14 closes without a fuse

**Registered in `report/PREREG_FUSE02.md` (`bc43d10`, engine `c10b85d`) before any cell
ran.** All 80 cells exit 0 and read `source bc43d104 dirty 0`. The outputs, commands and
digests are in results/fuse02_2026-10-04/ (`RUN.md`). The analysis is
`scripts/fuse02_analyse.py`, FUSE-01's computation unchanged, committed with the
registration and applied unchanged. Its tables are `FUSE02.md` there.

**In one paragraph.** On the finished preset (the spec's three gateways, half-duplex per
chip, both colliding frames judged), fusing a short active set with the floor only when
the floor is short too:
- **availability** rises by +0.105 at 5 km and +0.127 in the endgame, and `ca_tbl_full`
  with it is above `static_868` at 5 km by 0.106;
- **UWB-delivered fixes** come on 15 of 20 endgame seeds (65, 64 of them from fused sets),
  against none without it;
- **the cost:** tag 0's 5 km CEP95 is worse by a mean +113.9 % per seed (interval
  [−1.6 %, +273.5 %]). In the endgame it is +4.3 % [−4.6 %, +12.9 %].

The registered rule allows at most +10 % on the mean in both configurations. So **the
preset keeps the floor-first rule**, and N.14 is closed without a fuse.

---

## 1. The tables (`ca_tbl_full`, seeds 1-20, paired on − off)

| | 5 km | endgame |
|---|---|---|
| availability, off → on | 0.166 → **0.271** | 0.417 → **0.543** |
| paired difference | **+0.105** [+0.085, +0.126] | **+0.127** [+0.111, +0.143] |
| `static_868` | 0.165 | 0.371 |
| `ca_tbl_full` on − `static_868` | **+0.106** [+0.077, +0.136] | +0.173 [+0.159, +0.185] |
| tag 0 CEP50 / CEP95, off → on (m) | 89.3 / 193.8 → 52.9 / 161.8 | 8.4 / 17.2 → 8.1 / 17.7 |
| relative CEP95 change (mean per seed) | **+1.139** [−0.016, +2.735] | +0.043 [−0.046, +0.129] |
| UWB-delivered fixes, off → on (seeds > 0) | 1 (1) → 3 (2) | 0 → **65 (15)**, 64 from fused sets |
| sessions with a defined spread | 0 → 0 of 20 | 10 → 14 of 20 |
| subject energy, off → on (J) | 12.63 → 13.38 | 24.96 → 34.37 |

The CEP50/CEP95 row is the per-seed values pooled as the analysis prints them; the
registered test is the mean of the per-seed ratios below it.

## 2. Predictions

- **P1 held.** At 5 km availability rises: +0.105, interval above 0.
- **P2 held.** At 5 km `ca_tbl_full` with the narrowed fuse is above `static_868`: +0.106.
- **P3 held.** UWB-delivered fixes on 15 of 20 endgame seeds (the prediction was 5).
- **P4 failed.** In the endgame tag 0's CEP95 is +4.3 % on the mean, but the interval's
  upper end is +12.9 %, above the +0.10 the prediction set.
- The adoption rule reads the means, not the intervals. The endgame mean (+4.3 %) passes it;
  the 5 km mean (+113.9 %) fails it.

## 3. Why the 5 km mean fails (descriptive, not a result)

Tag 0's 5 km CEP95 ranges over two orders of magnitude between seeds without the fuse, from
10.6 m to 1 015 m: with three gateways at 5 km, tag 0's fix depends on where its walk takes
it relative to them. The per-seed ratio is therefore heavy-tailed:
- 9 seeds better, 11 worse;
- seeds 1 and 2 go from 17.4 m and 10.6 m to 226.1 m and 106.7 m (×13.0 and ×10.1). A seed
  whose fix was good on the floor alone gets worse when 1-2 active ranges of another tier
  and a short floor are fused into it.
- the pooled CEP95 improves, 193.8 → 161.8 m, because the large seeds improve.

So the narrowing removed FUSE-01b's endgame cost (+13.7 % → +4.3 %) and moved the cost to
5 km, where a short floor is common at three gateways. That is consistent with the mechanism
the narrowing keeps: it fuses exactly the cycles where the floor alone is short.

## 4. What follows

- The preset sets no fuse. `--fuseFloorActive` and `--fuseFloorShort` stay as flags with
  their gates (G63, G85).
- TECH_DEBT N.14 is closed: both variants were measured by registered campaigns, and both
  fail the registered accuracy bound (FUSE-01b on the endgame, FUSE-02 at 5 km), on two
  different presets.
- CAL-03 runs on the preset as it stands.
