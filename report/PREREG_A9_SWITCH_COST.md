# Pre-registration — A9, the switching cost of a tiered architecture

**Written 2026-09-07, before the first cell runs.** Corrected topology (`--topology=0`).
Registered under `RULES.md` rule 13.

---

## 0. Why this campaign exists

The 2026-09-07 topology correction left the tiered arm **dominated on both axes**: at
`boxHalf` 1000–5000, 600 s, 4 converging finders, 5 seeds, `static_2p4` reaches
cep50 **1.81–2.62 m** at **1.3–3.2 J** while `ca_tbl_full` reaches **3.63–5.17 m** at
**17.5–88.3 J**. An architecture that is beaten on accuracy *and* energy by one of its own
tiers has no case, so the question is whether that is a property of tiering or of this
policy's use of it.

**A diagnostic on `demo3` (seed 7, `boxHalf` 1000) says it is the policy.** Fixes broken
down by where they came from:

| active tier | fix came from | n | median error |
|---|---|---|---|
| UWB | **UWB** | 5 | **0.23 m** |
| 2.4 GHz | 2.4 GHz | 41 | 3.59 m |
| UWB | tier-1 floor | 98 | 7.49 m |
| 2.4 GHz | tier-1 floor | 76 | **28.60 m** (p90 184.73) |
| 868 MHz | 868 MHz | 18 | 43.74 m |

Two things follow, and only the second is actionable:

1. **Tier 3 works when it is reached.** 0.23 m median is the architecture's whole premise,
   measured, and it is not in dispute.
2. **174 of 225 fixes came from the tier-1 floor** while the node was paying to sit on a
   higher tier. On the 2.4 GHz-active cycles that fell back, `vis2p4` was **3.51** against
   **3.61** on the cycles that succeeded — *the anchors were visible either way*, so this
   is not coverage. **73 of those 76 fallbacks occur within 4 cycles of a tier switch**, and
   the run contains **70 switches in 240 cycles**.

**Hypothesised mechanism:** the dominant cost of a tier switch is not energy but
**neighbour-table re-acquisition latency** — a switched-to tier has no neighbours until it
has heard beacons, and until then the node falls to the coarsest tier. A policy that
switches 70 times in 240 cycles pays that latency 70 times.

This campaign tests whether **suppressing switch thrash recovers the tiered arm**, using
two flags that already exist (A6) and therefore requires **no code change**.

## 1. Question

Does increasing switching hysteresis reduce tier switches and thereby improve
`ca_tbl_full`'s accuracy and energy, and does it close the gap to `static_2p4`?

## 2. Population and denominator

- Cells: `nDeescalate` ∈ {5 (default), 10, 20, 40} × `pcsHysteresisBand` ∈ {default, 0.5}.
- 5 seeds per cell (1–5), 8 cells, 40 runs. Six arms per run.
- `--topology=0 --boxHalf=1000 --gwGrid=3 --nFinders=4 --finderMode=1 --duration=600`.
- Denominator for every rate: all 240 decision cycles of a run. A cycle with no fix counts
  against availability. Observations truncated by the run end settle any threshold already
  crossed (rule 2).

## 3. Metric and estimator

Per arm, per cell: **mean of per-seed values over 5 seeds**, with the per-seed spread
reported beside it. Cross-arm differences are **per-seed paired** differences, never a
single-seed difference (rule 8).

Primary: `cep50M`, `totalJ`, `escalations + deescalations` (switch count), and the
fraction of fixes taken from the floor.

## 4. Prediction

1. Switch count **falls monotonically** with `nDeescalate`.
2. `ca_tbl_full` cep50 **improves** (falls) as switch count falls.
3. `ca_tbl_full` `totalJ` **falls**, because fewer escalations means less DW3000 receive time.
4. At the best cell, `ca_tbl_full` **still does not beat `static_2p4` on energy.** Tier 3
   costs 94 mA to listen; nothing in this campaign changes that.

## 5. Falsifier

If switch count falls and cep50 **does not improve**, the re-acquisition mechanism is
wrong and the fallback quality is being set by something else. That closes the
hypothesis in §0 and the tiered arm's deficit is not a tuning problem.

## 6. Decision rule

- **PASS** — switch count falls monotonically in `nDeescalate` AND `ca_tbl_full` cep50 at
  the best cell improves on the default cell by more than the per-seed spread of the
  difference, on ≥ 4 of 5 seeds.
- **FAIL** — switch count falls and cep50 does not improve, or worsens.
- **INCONCLUSIVE** — switch count does not fall (the flag did not do what it says), or the
  per-seed spread exceeds the effect.

## 7. Abort conditions

- Any cell recording `dirty != 0` — not citable, do not analyse.
- The null cell (`nDeescalate=5`, band default) must reproduce the existing corrected-topology
  numbers for seeds 1–5 (`ca_tbl_full` cep50 3.63, `totalJ` 17.47 from `catbl_work/acc`). If it
  does not, the tree moved and the comparison is void.

## 8. What this campaign cannot establish

- **It cannot make the tiered arm cheaper than `static_2p4`.** Prediction 4 says so in
  advance. Energy is dominated by UWB receive current and this campaign does not touch it.
- **It cannot show tiering is necessary.** `static_2p4` already meets the registered
  requirement (A ≥ 0.95 at R ≤ 25 m) at this geometry. A requirement that tier 2 cannot
  meet — the sub-metre terminal accuracy a buried-victim search needs, against tier 2's
  cited 3.0 m floor — is a **different registered requirement** and a different campaign.
  Nothing here should be read as evidence for it.
- One geometry, one duration, one finder count, 5 seeds.
