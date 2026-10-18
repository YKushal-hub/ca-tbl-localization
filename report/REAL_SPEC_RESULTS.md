# REAL-SPEC results — the specified integrity threshold buys availability with endgame accuracy, out of sample

Registration: `report/PREREG_REAL_SPEC.md` (`85b7a0da`, pushed before any cell). 80 cells on **seeds 41–60, never run
before**; all `rc 0`, `source 85b7a0da dirty 0`, one frozen binary; launched 2026-10-18T18:11:45Z, done 18:27:26Z.
Scored once, unedited, "Problems: 0". Tracked: `results/real_spec_2026-10-18/`. The scorer wrote its output under the
template's file name, `REAL_BEST.md`; its content is REAL-SPEC's and it is tracked as `REAL_SPEC.md`.

S is the realistic-world stack at the preset's integrityChi2 1992.9. N is the same at the specified χ²₀.₉₉ = 9.21.
Only `ca_tbl_full` runs the integrity check.

## 1. Outcome

| # | prediction | outcome |
|---|---|---|
| P1 | N raises CA-TBL availability in B, none lost in A | **held**: B +0.012 [+0.001, +0.024]; A +0.005 [−0.007, +0.018] |
| P2 | N costs CA-TBL ≤ +10 % CEP95 (mean per-seed ratio), A and B | **not held**: B **+0.415** (3.9 → 5.3 m, +1.4 [+0.2, +3.0]); A −0.068 |
| P3 | under N, no comparator significantly higher availability | **held** |
| P4 | under N, no comparator significantly more accurate | **not held**: B `fixed_threshold` +1.3 [+0.5, +2.1], Poucet +1.7 [+0.4, +3.3] m |
| P5 | under N, CA-TBL availability above Poucet in B | **held**: +0.024 [+0.013, +0.035] |

**Decision rule: not proposed** (P2 and P4 failed). The preset's 1992.9 stays with the stack.

## 2. What it shows

- **The specified gate is a trade, not a fix.** At 9.21 CA-TBL rejects more fixes and escalates more: endgame subject
  J 20.68 → 26.90. That buys +0.012 availability and costs +1.4 m of endgame CEP95. On seeds 1–20 the accuracy cost
  was hidden (+0.5 m, n.s.); out of sample it is significant.
- **Under S on these fresh seeds** (the proposed stack): the endgame CA-TBL is 3.9 m, Poucet 3.6, `fixed_threshold`
  4.1; availability 0.577 against 0.565 and 0.576.
- **The three axes trade within CA-TBL's own settings.** The integrity threshold moves along an accuracy /
  availability / energy frontier; so do the escalation thresholds (N.60). No setting tried beats every comparator on
  both accuracy and availability. Each gain on one axis has cost the other, or energy.
