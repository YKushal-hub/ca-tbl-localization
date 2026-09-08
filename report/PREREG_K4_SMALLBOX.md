# PREREG K4 — is CA-TBL non-dominated where tier 2 actually reaches?

**Registered before the cell runs.** Rule 13.

---

## 1. The question K1 could not reach

Every K1 cell sat at gateway spacing of 675–2700 m against a measured 2.4 GHz r80 of
**156 m**, so tier 2 was marginal everywhere and absent at the top. The trend across that
sweep is favourable to the architecture — at `boxHalf=750`, `ca_tbl_full` is already
**non-dominated** by `static_868`, buying 45 m of accuracy for 1.04 J on 18 of 20 seeds —
and it points into a regime the campaign could not measure.

`report/K1_DOMINATION_RESULTS.md` §7 records that regime as a **gap**, and says explicitly
that nothing in K1 extrapolates into it. This measures it.

## 2. The technique that makes it affordable, and its null control

K1's makespan was set by its slowest single cell, because a cell runs its 20 seeds
sequentially in one process and free cores cannot shorten it. `boxHalf=400` costs ~924 s
per seed, so one cell is over five hours.

**Splitting a cell across processes by `--seedStart` fixes that, and it is byte-identical.**
Seed 8 run alone, against the same seed as the second of twenty inside `k1a_box1500`:

```
static_868,8,111.8721,14.421047393        static_868,8,111.8721,14.421047393
static_2p4,8,51.8481,8.068015428          static_2p4,8,51.8481,8.068015428
static_uwb,8,703.1336,891.545343843       static_uwb,8,703.1336,891.545343843
fixed_threshold,8,193.6179,18.312645727   fixed_threshold,8,193.6179,18.312645727
ca_tbl_no_integrity,8,91.3250,15.097996989  ca_tbl_no_integrity,8,91.3250,15.09799698
ca_tbl_full,8,225.5448,17.703765545       ca_tbl_full,8,225.5448,17.703765545
```

`cmp` reports the blocks identical. **Seeds are independent of the seeds that ran before
them in the same process**, so a cell may be split without changing a number. This was
checked rather than assumed, because if RNG state carried across seeds the split would
have altered results silently — the exact class of defect rule 8 exists for.

## 3. The cell

`k4_box400` — the `k1a` configuration at `boxHalf=400`, gateway spacing 360 m against a
156 m tier-2 range, 20 seeds (7–26), split across **10 processes of 2 seeds each** and
concatenated. All other flags identical to `k1a_box*`, so the K1 cells are the comparison
series.

## 4. Predictions

| # | prediction | how it fails |
|---|---|---|
| T1 | `ca_tbl_full` is **non-dominated by `static_868`** at `boxHalf=400`, as it already is at 750 | it is dominated |
| T2 | `static_2p4` **still dominates** `ca_tbl_full` — the trend across K1 is not steep enough to reverse by 400 | it does not, which would be the first cell in the project where the architecture beats every single band |
| T3 | `ca_tbl_full` beats `fixed_threshold` on accuracy, as it does at 750 and 1000 | it does not |

T2 is registered as a prediction that the system **still loses**, for the same reason P1
was in K1: the temptation this campaign has to guard against is reading a favourable
trend as a result.

## 5. Falsifier

**U1.** If `ca_tbl_full` is dominated at `boxHalf=400` as it is at 1500 and above, the
favourable trend K1 §7 reports is **not** evidence of a regime where the architecture
wins, and that sentence in §7 is amended to say so.

## 6. Abort conditions and analysis

- **120 rows exactly** after concatenation — 20 seeds × 6 arms — counted, not tested
  (rule 12). A short cell is excluded **by name**.
- **`dirty 0`** in all ten sidecars, and all ten must carry the **same commit**. A cell
  assembled from processes at different SHAs is not one cell.
- Analysis is `scratch/python_handoff/k1_analyse.py` unchanged — the same domination
  definition, the same paired sign test, the same α.
