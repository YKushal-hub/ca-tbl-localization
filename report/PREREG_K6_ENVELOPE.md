# PREREG K6 — is there a deployment large enough that no single band covers it?

**Registered before the cells run.** Rule 13. This is the owner's step 1: find the
operating envelope in which tiering has to pay, or establish that there is none.

---

## 1. Why K1 could not find one

K1 swept `boxHalf` 400–3000 and `static_2p4` dominated `ca_tbl_full` in every cell. The
reason is visible in the coverage column and it is a property of the **scenario**, not of
the policy:

| `boxHalf` | `static_2p4` fix cycles | `static_868` fix cycles |
|---|---|---|
| 400 | 1414 / 1440 | 1439 |
| 750 | 1082 | 1440 |
| 1500 | 924 | 1438 |
| 3000 | 570 | 1422 |

**868 MHz covers the whole sweep** — 99% of cycles even at `boxHalf=3000` — so there is
never a region where the long-range tier is the only thing that reaches. And 2.4 GHz keeps
getting opportunities because the walker's tour of the four facility sites at
0.47·`boxHalf` **transits the centre gateway** on every diagonal leg. Coasting between
occasional excellent fixes is enough to win.

A tiering architecture cannot pay where one band already spans the mission. **The envelope
where it must pay is where no band does.**

## 2. The cells

The `k1a` configuration — corrected environment, 4 finders, evidence gate, gated PCS, floor
on demand — extended past K1's top end. `--gwGrid=3`, so gateway spacing is 0.9·`boxHalf`
against a measured 868 r80 of **1515.3 m**:

| cell | `boxHalf` | gateway spacing | spacing / 868 r80 |
|---|---|---|---|
| `k6_box4000` | 4000 m | 3600 m | 2.38 |
| `k6_box5000` | 5000 m | 4500 m | 2.97 |
| `k6_box6000` | 6000 m | 5400 m | 3.56 |

20 seeds each (7–26) as four processes of five, concatenated; the split is byte-identical
and proved in `PREREG_K4_SMALLBOX.md` §2.

**No new flag and no code change.** The envelope is reachable with the knobs that exist,
which is the cheapest way to answer the question and leaves nothing new to null-control.

## 3. Predictions

| # | prediction | how it fails |
|---|---|---|
| X1 | `static_868`'s fix coverage falls **below 90%** by `boxHalf=6000` — the long-range tier finally stops spanning the mission | it stays above 90% |
| X2 | `static_2p4`'s coverage falls below **25%**, and its rmse rises faster than `ca_tbl_full`'s, because dead reckoning over long blind stretches diverges | its rmse does not rise faster |
| X3 | `ca_tbl_full` is **non-dominated by `static_2p4`** in at least one cell — the first time in this project | it is dominated in all three |
| X4 | `ca_tbl_full` remains non-dominated by `static_868`, as it was at `boxHalf=400` | it is dominated by it |

X3 is the whole question. It is stated as a prediction that the system **wins**, which is
the opposite bet to P1 and T2, and it is registered that way so the result is not read
selectively either direction.

## 4. Falsifier

**Y1.** If `static_2p4` dominates `ca_tbl_full` in all three cells, then **there is no
deployment scale in the reachable range at which the three-tier architecture is
non-dominated by a single 2.4 GHz radio.** That is the finding, it is reported as the
answer to the owner's step 1, and the permitted follow-up is to change the *scenario
family* — gateway siting, mobility, mission profile — not to keep enlarging the box.

**Y2.** If X1 fails — 868 still covers 90%+ at `boxHalf=6000` — then box size is the wrong
axis entirely, because the long-range tier spans anything this deployment family can
build. The follow-up is siting: cluster the gateways at a base camp so that coverage is
directional rather than uniform, which is what a real search actually looks like.

## 5. Abort conditions and analysis

120 rows per cell after concatenation, counted (rule 12); all four shards of a cell at the
same commit; all `dirty 0`; new paths only. Analysis is
`scratch/python_handoff/k1_analyse.py` unchanged — same domination definition, same paired
sign test, same α — so these cells drop straight into K1's series.

**One control that K5 taught:** every cell here runs on the **current** binary, and K1's
cells did not. Cross-cell comparisons against K1 therefore carry the cross-binary caveat
`TECH_DEBT` 1.2 records; comparisons *within* K6 do not. The three K6 cells are compared
to each other and to their own arms, never to a K1 number, for anything smaller than the
static-arm drift K5 measured at 32–40 m.
