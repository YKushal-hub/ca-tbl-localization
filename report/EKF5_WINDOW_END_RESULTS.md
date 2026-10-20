# EKF5-WINDOW-END results — the endgame accuracy gain replicates; the availability bound is met only at its edge

Registration: `report/PREREG_EKF5_WINDOW_END.md` (`d3e9b581`, pushed before any cell). 40 cells on **seeds 81–100,
never run by this line before**. All `rc 0`, `source d3e9b581 dirty 0`, one frozen binary. Launched
2026-10-20T09:03:52Z (14:33 IST), done 09:15:14Z (14:45 IST). Scored once, unedited, "Problems: 0". Tracked:
`results/ekf5_window_end_2026-10-20/`.

## 1. Outcome

| # | prediction | outcome |
|---|---|---|
| P1 | W lowers CA-TBL's endgame tag 0 CEP95 against S | **held**: 3.9 → 3.1 m, −0.8 [−1.4, −0.2] |
| P2 | under W, CA-TBL's coverage ≥ 0.93 | **held**: 0.965 |
| P3 | W costs CA-TBL no availability: lower bound above −0.01 | **not held**: +0.000 [−0.010, +0.010]; the lower bound sits on the threshold, not above it |
| P4 | under W, CA-TBL ties Poucet's CEP95 and beats its availability | **held**: CEP95 +0.1 [−0.3, +0.5]; availability +0.030 [0.014, 0.044] |
| P5 | W lowers `fixed_threshold`'s CEP95 | **held**: −0.6 [−1.1, −0.1] |

**Decision rule (P1 + P3): not met, as written.** The availability change is zero to three decimals, and its interval
reaches exactly the registered bound. It is not read past the bound after the fact. `--ekf5Window` stays default off.
What the owner may decide on, with both registrations in hand:
- **Accuracy:** the window's endgame gain has replicated twice, out of sample: −1.5 [−3.3, −0.2] on seeds 61–80 and
  −0.8 [−1.4, −0.2] on 81–100.
- **Availability:** no detectable effect either way, within ±0.01.
- **5 km:** not supported (EKF5-WINDOW P1).
- **Energy:** unchanged; subject J −0.78 [−2.11, +0.54].

## 2. Per seed, CA-TBL tag 0 CEP95 (m), S → W

81 4.1 → 2.9, 82 2.6 → 3.2, 83 4.8 → 3.7, 84 4.6 → 2.9, 85 2.5 → 4.5, 86 4.1 → 2.5, 87 5.3 → 3.1, 88 3.6 → 3.4,
89 4.8 → 2.7, 90 4.9 → 4.1, 91 7.5 → 3.4, 92 2.5 → 2.5, 93 6.9 → 3.4, 94 2.5 → 3.1, 95 4.9 → 2.9, 96 2.2 → 2.7,
97 2.4 → 3.1, 98 3.6 → 3.3, 99 2.2 → 2.2, 100 2.2 → 2.7.

The window helps most where S was worst (91, 93: about 7 → 3.4 m), and costs a little on seeds already near the
~2.5 m floor (82, 85, 94, 96, 97, 100). The result is a narrower spread: worst seed 7.5 → 4.5 m.

## 3. What this cannot establish

The 5 km behaviour; the window length; real sensors; the ring; Wolf ToF; compute cost on a device.
