# Claim 2's energy ratio, re-measured post-D1 — pre-registration

Written and committed **before the first cell**. These cells do not exist.

## The search result that necessitates this

`[M-11]` cites *"`arm_summary.csv` energy columns"* — a file type, not a path. I previously
reported that **622.92 J appears in no `arm_summary.csv` on disk**, and concluded the number had no
source. **That conclusion was wrong and is corrected here.**

622.92 J is a **directory mean**, not a row:

| | directory | mean `ca_tbl_full` totalJ | n |
|---|---|---|---|
| before | `results/p10_plane` | **622.925** | 20 |
| after | `results/p10_plane_fix` | **24.260** | 20 |

622.925 / 24.260 = **25.677**, the published 25.68×. The pair is the evidence gate off against on:
`p10_plane_fix` adds `--escalationRequiresEvidence=true` and changes nothing else.

**My error was searching the wrong granularity** — rows, when Claim 4's figures had already shown me
these headline numbers are means over seeds. The literal-scan across 17 418 rows was correct and its
conclusion did not follow from it. *"Not present as a row"* is not *"has no source"*.

## Why the claim still cannot stand as measured

Three defects in the provenance, any one of which is disqualifying under rules this project already
enforces:

| | `p10_plane` | `p10_plane_fix` |
|---|---|---|
| source sha | `a059203` (2026-08-09) | `a059203` |
| **pre-D1?** | **YES** — `0d5e747` is 2026-08-26 | **YES** |
| dirty counter | **3** | **3** |
| `nlosSource` | no flag → **defaulted `legacy`** | same |

1. **Both sides predate the D1 sensitivity correction by 17 days.** The guardrail is explicit: a
   baseline that predates D1 invalidates the ratio built on it, whatever the after-number's
   provenance. W1 is the precedent. Here **both** sides are pre-D1, so the ratio is measured
   entirely on the defective engine — and D1 was a 6 dB error in the 2.4 GHz and UWB sensitivity
   tables, which are exactly the tiers the escalation gate decides to use.
2. **`dirty 3`** — the runs carry three uncommitted files, so their source state is not recoverable.
3. **Defaulted `legacy`** — the same exposure that withdrew Claim 3 and halved Claim 4.

## Design

The comparison is one flag, so it re-runs directly:

    --duration=3600 --boxHalf=2000 --gwGrid=5 --trace=true --arms=ca_tbl_full
    --escalationRequiresEvidence={false,true}  x  --nlosSource={legacy,propagation}
    seeds 1..20

20 seeds to match the published depth. Four cells rather than two, so the D1 correction and the
NLOS source are **separated** rather than confounded: the `legacy` pair isolates D1's effect, and
the `legacy → propagation` step isolates the source's, on a post-D1 binary throughout.

`--arms=ca_tbl_full` is justified by the byte-identity test recorded in
`CLAIM4_NLOS_SOURCE_REGISTRATION.md`.

## Registered

**Prediction.** The ratio changes **materially** — outside ±10% of 25.68 — on the post-D1 binary.
Mechanism: D1 was a 6 dB sensitivity error on the two tiers the evidence gate governs, so the
energy cost of an ungated escalation is measured wrongly on both sides of the published ratio.

**No direction is predicted.** D1's sign effect on this particular ratio is not established, and
guessing it would be predicting the conclusion. Direction is reported, not forecast.

**Falsifier.** The ratio lands within ±10% of 25.68 under `legacy` post-D1. Then D1 does not reach
this comparison, and the pre-D1 provenance — while still a record-keeping defect — did not change
the number.

## Consequences, fixed now

| outcome | consequence |
|---|---|
| ratio survives materially intact | Claim 2 **re-based**: same conclusion, cited to post-D1 `dirty 0` evidence, with the corrected magnitude |
| ratio shrinks but stays large | Claim 2 **re-scoped**, as Claim 4 was — direction stands, headline restated |
| ratio collapses or reverses | Claim 2 **withdrawn as W12**, W1 style, original text retained, `[M-11]` annotated not deleted |

**Claim 9 is not touched.**
