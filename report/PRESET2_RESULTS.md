# PRESET-2 — result (Amendment 1), seeds 101–120

**Registration:** `report/PREREG_PRESET2.md` (`05e8417f`); Amendment 1 (`23f68767`).

**Run.** Launched 2026-10-20T11:25:57Z from `23f68767`, dirty 0, and done 13:24:52Z. 180 of 180 cells, every `rc` 0,
one source, and no window-solver failure: the scorer's problem list is empty.

**Scored once** with the committed `scripts/preset2_analyse.py`. Its full output, every arm in every configuration,
is `results/preset2_a1_2026-10-20/PRESET2.md`. Per-cell summaries and sidecars are tracked. Traces are not; the
`raw_manifest.sha256` names every raw file by its SHA-256.

The first launch (`05e8417f`) was aborted by its registered abort condition: every N and R cell refused to start.
Its `RUN.md` is kept in `aborted_first_launch/`.

## The registered decision: NOT MET

| # | prediction | verdict | measured (CA-TBL, paired, 20 seeds) |
|---|---|---|---|
| P1 | N raises five-node A(25 m) in B | **FAILS** | +0.035 [−0.044, 0.103] |
| P2 | N raises tag-0 A(25 m) in A | HOLDS | **+0.443 [0.281, 0.601]** |
| P3 | N lowers five-node A(25 m) nowhere (lower bounds > −0.02) | **FAILS** | A +0.236 [0.168, 0.306]; **B +0.035 [−0.044, 0.103]**; C +0.153 [0.040, 0.253] |
| P4 | N's subject J within +10 % | HOLDS | ratio A 0.833, B 0.957, C 0.830: **energy falls** |
| P5 | R against N in B: J lower, A(25 m) not lower | **FAILS** | J **+2.06** [0.56, 3.68]: arbitration off costs energy |

**By the registered rule, N does not become `--specProfile`, and arbitration V2 stays in it.**

**P1 and P3 fail on one interval:** the endgame's five-node contrast, mean +0.035, which spans 0 in both
directions. Nothing was measured to fall. P3's lower bound of −0.02 was not met because the endgame interval is
wide, not because N lowered availability anywhere. That is what the rule was written to require, and it is applied
as written.

**P5's failure reverses DEBT-A's ARBOFF reading.** DEBT-A found −18 % endgame energy with arbitration off. With the
N set, which includes `--pcsByteScale=4` so the requester rule can refuse, arbitration off **costs** 2.06 J
[0.56, 3.68]. Arbitration earns its energy once the requester rule works.

## What N does to the design requirement (A ≥ 0.95 at R ≤ 25 m), reported

CA-TBL means over 20 seeds, S → N:

| config | A(25 m) tag 0 | A(25 m) five-node | CEP95 tag 0 | subject J |
|---|---|---|---|---|
| A, 5 km | 0.386 → **0.829** | 0.268 → **0.503** | 700 → **303 m** | 19.4 → **16.2** |
| B, endgame | 0.994 → 0.995 | 0.856 → 0.891 | 3.9 → 3.3 m | 21.2 → 20.3 |
| C, 8-gateway ring | 0.790 → **0.959** | 0.505 → **0.658** | 229 → **47 m** | 17.8 → **14.7** |

**Under N, CA-TBL against each comparator** (paired; positive = CA-TBL higher):
- **five-node A(25 m):**
  - static_868: +0.50 / +0.62 / +0.44 in A / B / C;
  - static_2p4: +0.24 / +0.13 / +0.21;
  - poucet_fusion: +0.01 / +0.06 / −0.03, every interval spanning 0;
  - fixed_threshold: −0.003 / −0.028 / −0.023, every interval spanning 0.
- **subject energy:** CA-TBL uses **383–446 J less than Poucet fusion** (about 16–20 J against 403–462 J). Against fixed_threshold
  it is level or lower (−0.15, −1.87, −0.68 J).

**Where the requirement stands under N:**
- **met for tag 0** in the endgame (0.995) and on the 8-gateway ring (0.959);
- **not met** for the five-node pool anywhere (0.50–0.89);
- **not met** for tag 0 at the brief's 5 km three-gateway geometry (0.83).

## Recommendation, not a decision

**The rule failed on the endgame's width.** Every measured effect of N on the requirement is upward, the largest
are at 5 km (+0.44 tag 0, +0.24 five-node), and energy falls.

Adopting N anyway would be an **owner override**, as FIXES-N26 was (DESIGN_DECISIONS 9). It is recorded here as the
recommendation, with these numbers, for the owner, and is **not applied**. Every later campaign therefore still
passes its stack explicitly. FIDELITY-1 ran on base W by its registered rule.
