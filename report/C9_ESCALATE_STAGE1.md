# A2 stage 1 — detect-and-escalate under injected fault

**Written** 2026-09-05 from `results/c9_escalate`, 160 runs, 8 cells × 20 seeds, `dirty 0`.
Decision rule applied verbatim from `report/PREREG_C9_ESCALATE.md`, committed before the
first cell ran.

## Verdict: INCONCLUSIVE, by construction

The registered rule needs **more than one bias magnitude** for either PASS or FAIL. Stage 1
is one magnitude (40 m). It cannot return a verdict on its own, and reporting one would be
reading the rule backwards. What stage 1 does is decide where stage 2 should spend compute.

## 1. The gate changes availability by nothing at all

Mean of per-seed differences, `ca_tbl_full` − `ca_tbl_no_integrity`, 20 seeds, sign test
beside it, F1 and F3 separate.

| policy | fault | mean ΔA | sign | mean Δcep50 (m) |
|---|---|---|---|---|
| none | F1 | +0.00000 | 0+/0− | +0.015 |
| inflate | F1 | +0.00000 | 0+/0− | +0.005 |
| bounded | F1 | +0.00000 | 0+/0− | +0.013 |
| reinit | F1 | +0.00000 | 0+/0− | +0.013 |
| none | F3 | −0.00014 | 0+/1− | +0.090 |
| inflate | F3 | +0.00003 | 1+/0− | −0.016 |
| bounded | F3 | −0.00003 | 0+/1− | −0.002 |
| reinit | F3 | −0.00007 | 0+/1− | −0.006 |

On every F1 cell the sign test has **no non-zero seeds at all** — all 20 per-seed
differences are exactly zero. That is not a small effect, it is the absence of one on this
axis.

**Availability cannot resolve this question.** Both arms obtain a fix on essentially every
cycle, so a metric whose denominator is "cycles with a fix" is saturated before the
treatment is applied. The pre-registration anticipated this and put it in the INCONCLUSIVE
branch explicitly — *"including the case where the gate fires too rarely for a difference to
be resolvable."* The accuracy differences are the same story at 0.002–0.090 m, which on a
tier ranging at ~1.5 m median is noise.

This is a finding about the **metric**, and it repeats the structural result of §7: a
criterion was registered that cannot express the effect the mechanism produces.

## 2. CAL-05, produced for the first time

Detection precision and recall against injected ground truth. The register defers this to
Phase 2.1; the project has never computed it.

| policy | fault | precision | recall | TP | FP | FN |
|---|---|---|---|---|---|---|
| none | F1 | 0.9659 | 0.4422 | 2547 | 90 | 3213 |
| inflate | F1 | 0.9807 | 0.2913 | 1678 | 33 | 4082 |
| bounded | F1 | 0.9799 | 0.3885 | 2238 | 46 | 3522 |
| reinit | F1 | 0.9765 | 0.3826 | 2204 | 53 | 3556 |
| none | F3 | 0.8425 | 0.7158 | 4123 | 771 | 1637 |
| inflate | F3 | 0.9750 | 0.4410 | 2540 | 65 | 3220 |
| bounded | F3 | 0.9739 | 0.6101 | 3514 | 94 | 2246 |
| reinit | F3 | 0.9766 | 0.6021 | 3468 | 83 | 2292 |

**Denominators.** The fault window is contiguous and seed-derived,
`biasStartS = durationS × (0.25 + 0.5·(seed mod 7)/7)`, 720 s at fraction 0.2 — 5760
injected cycles per cell across 20 seeds. Placed mid-run so it never straddles EKF
convergence and never runs past the end; no window was truncated, checked rather than
assumed. Recall was computed against both all injected cycles and only those where the gate
could fire (`hadFix == 1`); **the two are identical here**, because a fix was available on
every injected cycle.

### What the numbers say

The integrity check is **precise and insensitive**. It almost never cries wolf — precision
0.84–0.98 — and it misses **56–71 % of injected fault cycles on F1** and 28–56 % on F3.
χ²₀.₉₉ = 9.21 is a threshold chosen for a 1 % false-alarm rate, and it delivers exactly
that; what it does not deliver is detection. At a 40 m common-mode bias the mechanism sees
fewer than half the faulted cycles on the canonical single-anchor RAIM model.

That is a **calibration result, not a failure**: it is the precision/recall curve CAL-05
asks for, measured at one operating point, and it identifies the lever. A lower threshold
trades precision for recall, and nothing in the project has ever swept it.

F1 and F3 differ as expected and are reported separately, as registered: common-mode bias on
every link (F3) is easier to see than one displaced anchor (F1), and `none` on F3 pays for
its higher recall with 771 false positives — nearly an order of magnitude more than any
other cell.

## 3. What stage 2 should run

Stage 1 says the detection curve, not the availability difference, is where the information
is. Stage 2 should sweep bias magnitude `{10, 20, 80, 160}` against the existing 40 m point
and report precision/recall as a curve — that is a CAL-05 deliverable regardless of which
way claim 2 resolves, and it is the axis the registered rule needs for a verdict.

## 4. What this campaign cannot establish

Whether the integrity check helps. It measures detection at one bias magnitude and shows the
registered availability metric cannot resolve the treatment. A verdict needs stage 2.
