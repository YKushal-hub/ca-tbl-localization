# PREREG K8 — the base-camp scenario: a search, not an enclosure

**Registered before the cells run.** Rule 13.

---

## 1. Why every previous verdict is suspect

K1, K4 and K6 all measured a deployment in which:

- the casualty starts within 0.2·`boxHalf` of the centre and tours corners at
  0.47·`boxHalf` — **always inside the gateway footprint**;
- the 3×3 gateway grid has **one gateway at the origin**, in the middle of that tour;
- **finders spawn in the same central region** — the searchers begin beside the subject.

That is asset tracking in a covered yard. The condition the architecture exists for — no
single band spanning the mission — **never occurs in it**, so a single good radio wins by
construction. The domination verdicts were correctly computed on a scenario that could not
test the claim.

`--baseCampFrac` (`8103c09`+) puts the infrastructure and the finders' start at
(f·`boxHalf`, f·`boxHalf`) and leaves the casualty touring the centre. At f = 0.9,
`boxHalf` 3000, the camp is at (2700, 2700) and the tour corners are **1824 m to 5813 m
away**, mostly beyond 868's measured r80 of 1515 m. Reaching the casualty requires a
searcher to walk out — which is what a search is.

## 2. Cells

`--baseCampFrac=0.9`, the repaired configuration, plus the two defect fixes landed today:
`--mahalUseR=true` (K7: the innovation covariance's missing R term) and
`--escalationRequiresEvidenceT2=true` (the rebuilt tier-2 predictor). 20 seeds (7–26) as
four processes of five.

| cell | `boxHalf` | camp → casualty |
|---|---|---|
| `k8_camp_box3000` | 3000 | 1824–5813 m |
| `k8_camp_box5000` | 5000 | 3040–9689 m |

## 3. Predictions

| # | prediction | how it fails |
|---|---|---|
| C1 | `static_2p4` obtains **< 10%** fix cycles — it is not a system in this scenario | it exceeds 10% |
| C2 | `ca_tbl_full` is **non-dominated by every claim arm** in at least one cell — the first time in this project | it is dominated in both |
| C3 | `ca_tbl_full` beats `static_868` on **accuracy** at significance | it does not |
| C4 | It does so **without spending more energy** — the tier-2 predictor suppresses escalation into a tier that cannot reach | energy is significantly higher |

The one-seed smoke test that motivated this showed `ca_tbl_full` best on both axes
(353.1 m / 13.27 J against `static_868`'s 436.7 / 13.32 and `static_2p4`'s 938.5 / 36.5).
**That is one seed and rule 8 makes it a motivation, not a result.** These predictions are
registered so the 20-seed answer can contradict it.

## 4. Falsifier

**D1.** If C2 fails in both cells, the base-camp geometry does not rescue the architecture
either, and the honest position is that CA-TBL is dominated in every scenario family tested
— enclosure and search alike. That would be reported as the answer, and the follow-up would
be mechanism, not geometry.

## 5. A prediction about the MECHANISM, registered because it may surprise

The smoke test recorded **0 escalations** for `ca_tbl_full`, and `ca_tbl_no_integrity` at
439.9 m against `static_868`'s 436.7 m. So on that seed the advantage came **not from tier
switching** but from the integrity check rejecting bad tier-1 fixes now that its R term is
restored.

| # | prediction | how it fails |
|---|---|---|
| C5 | The gap between `ca_tbl_full` and `ca_tbl_no_integrity` is **larger** than the gap between `ca_tbl_no_integrity` and `static_868` — i.e. the win is the integrity check, not the tiering | the tiering gap is larger |

If C5 holds, the honest headline is that **the measurement-rejection machinery is what
pays in a search, and the tier switching is dormant** — which is a different claim from the
one the project set out to make, and has to be stated as such rather than folded into
"CA-TBL wins".

## 6. What this campaign cannot measure

`arm_summary.csv` carries availability, accuracy and energy. It does **not** carry
**time-to-first-fix** or **terminal accuracy in the final approach**, and those are the two
metrics a search actually cares about. They need per-cycle traces and are owed. Fix
availability (`fixCycles`/1440) is reported here as the closest available stand-in and is
not the same thing.

## 7. Abort conditions and analysis

120 rows per cell after concatenation, counted; all four shards at one commit; all
`dirty 0`; new paths. `scratch/python_handoff/k1_analyse.py` unchanged. Every cell runs on
the current binary and is compared to cells from this campaign, never to a K1 number.
