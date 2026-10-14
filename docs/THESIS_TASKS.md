# CA-TBL — thesis-side task list

**Not for the engine repository, and not for a working session working the engine.** Claude
Code's remit here is code, experiments and gates. This file is for the thesis work, which happens
in the thesis project against `outline.md`, `terminology_lock.md` and the registers.

Kept in the repo only so the two halves stay reconciled. If a working session is asked to
draft thesis prose in this repository, the answer is no — say so and stop.

---

## 1. Where the thesis actually is

All 15 chapters are `SCAFFOLD` — headings, source bindings and pending markers, **no prose**.
43 open register items: 7 provenance defects, 7 unresolved decisions, 10 calibration parameters,
12 research tasks, 5 writing tasks, 4 novelty-exposure findings. Last register update 2026-07-30.

The registers are ~5 weeks behind the repository, and that gap is the main risk: a chapter drafted
from `CA-DBL_Phase1_Blueprint.md` §6 will describe a system that no longer exists.

---

## 2. Decisions blocking drafting

| ID | Question | Status |
|---|---|---|
| **DEC-A** | System name | **ANSWERED: CA-TBL.** Log as `TERM-CHANGE-01`, propagate through `terminology_lock.md`, `glossary.md`, `acronym_list.md`, `notation_table.md`, every scaffold |
| **DEC-E** | Chapter 12 / claim 3 | **ANSWERED: build it.** Track C in `MASTER_PLAN.md`. C1 is in the 2–3 day cut; C2–C4 are not. Chapter 12 must be written against what actually lands |
| **DEC-B** | Estimation on Anchor/Gateway Node or Field Node | **OPEN. Blocks Ch. 9 and Ch. 10.** Recommend Field Node, matching the implementation — changing the implementation invalidates the energy model, the strongest validated component |
| **DEC-C** | Tier-1 observable: TDoA or RSSI inversion | **OPEN. Blocks Ch. 9 §9.3, §9.6, Ch. 6 §6.4.** Recommend RSSI inversion, matching the implementation, with TDoA moved to Ch. 8 §8.6 exclusions and justified — the same treatment AoA already gets |
| **DEC-D** | Inertial prior in the predict step | **ANSWERED 2026-10-14: build it.** CAL-05 showed the integrity check is a working detector, which is the condition this decision waited on. Built behind `--imuPrior` (C3); Ch. 10 §10.3.1 now has a substrate, and Ch. 10 must describe it as built (PDR, no magnetometer, no ZUPT — `Architecture.md` §9.6), not as specified |
| **DEC-F** | Ch. 14: specification or documentation | **OPEN.** Its scaffold says *"the existing prototype is subordinate to this specification"* — that premise has inverted. Recommend splitting: §14.1–14.9 specify, a new section documents what Phase 2.1 built and found, clearly marked out of Phase 1 scope |
| **DEC-G** | Role of the 5M-row ML dataset | **OPEN.** Not wired into the CA-TBL decision path. Simplest coherent answer: channel-model validation artefact only, ML naming dropped |
| DEC-01/02/03 | Front matter, institutional template, deadline | ADMIN-blocked |

---

## 3. Per-chapter status against the repository

Drafting order stands (9 → 10 → 11–13 → 15 → 14 → 3, 4 → 5, 2 → 6, 7 → 8 → 1). Content bindings
do not.

| Ch. | Action |
|---|---|
| 1 | Unchanged. **RES-06 is real and untouched** — nothing in the reference pool supports the Ch. 1 statistics. 7 needs, 0 candidate sources |
| 2 | Unchanged. Draftable |
| 3 | §3.13–3.16 draftable from the IMU source material — **but see DEC-D.** If no IMU is ever implemented, Ch. 3 may survey inertial sensing while Ch. 10 must not specify a prior it will never have. Decide together |
| 4 | Unchanged. Blocked on RES-02 (Table 4.1, ≈ 128 cited cells) |
| 5 | Rank-5 material still needs star → mesh reframing — **but the scope shrinks with whatever Track C does not land** |
| 6 | Add **NOV-02** to §6.5 + Table 6.13; **NOV-04** to §6.4. **NOV-03** only if DEC-C revives TDoA. See `CONTEXT_PACK.md` §3 |
| 7 | **NOV-01 forces a narrowing.** Hop-count degradation is well characterised in prior work. Gap 1 must be rewritten around *composing* it into a transported scalar — and C5 is the item that would give that substance |
| **8** | **Rewrite the novelty claims entirely, last, against what was measured.** §8.6 gains TDoA (and mesh, if C5 descopes). **§8.10 — "what Phase 1 does and does not establish" — becomes the most load-bearing section in the thesis** |
| **9** | Blocked on DEC-B, DEC-C. Otherwise draftable from `Architecture.md` §1–6 with terminology translation |
| **10** | Most affected chapter. §10.5 has no substrate; §10.6.5 weights were never determined; §10.7's status depends on A2. **Draft after A2 branches** |
| 11 | **Best-supported chapter in the thesis.** `Architecture.md` §7–8 supplies measured contention, the duty-cycle chain, computed ToA matching measurement to 5 s.f., datasheet energy reconciliation. Table 11.1 computable now. Note honestly that `ESC_REQUEST`/`ESC_GRANT` are billed not sent — **unless C1 lands, in which case update this** |
| **12** | Write against what Track C delivers. C1 alone gives claims 4 and 5 substance; claim 3 needs C2–C4 |
| 13 | No substrate. §13.7's single fault-model statement is still required and still valuable |
| **14** | Blocked on DEC-F and on Ch. 9–13 |
| 15 | Blocked on DEC-04, PROV-01, PROV-02. **Scope has outgrown "channel model"** — the engine now includes MAC, energy and estimation. Rename or rescope |

---

## 4. Research tasks — unaffected by any of the engine work

These block drafting and no amount of simulation closes them.

| ID | Task | Size |
|---|---|---|
| RES-01 | Table 3.1, observables master comparison | ≈ 84 cited cells |
| RES-02 | Table 4.1, technology master comparison | ≈ 128 cited cells |
| RES-05 | Group 4 LoRa localization literature — 6 of 21 resolved, 13 still unlocated | Medium |
| RES-06 | **Ch. 1 statistics — 7 needs, no candidate source in the pool** | Medium |
| RES-03 | Group 1 canonical works — 10 of 18 verified | Fast |
| RES-04 | Group 3 channel-parameter sources — 2 of 13 verified | Medium |
| RES-11 | Complete bibliographic fields for 11 references verified by title/venue/year only | Fast |
| RES-12 | Venue of `bouras2020tdoa` — the only located work combining TDoA + LoRaWAN + SAR | Fast |
| RES-07/08 | Verbatim 3GPP TR 38.901 equations; LoRaWAN TS001-1.0.4 ADR/TPC clause | Needs spec access |
| RES-09 | Confirm G.S.R. 853(E) unamended since Dec 2021 | Fast |
| RES-10 | Realistic responder mobility model | Medium |

---

## 5. Calibration parameters — current truth

The register defers all ten to Phase 2.1. Several are now answered, and two differently than
expected.

| ID | Parameter | Actual status |
|---|---|---|
| CAL-01 | PCS weights w₁, w₂, w₃ | **Never determined.** The implementation used an *unweighted* sum — the direct cause of the saturation defect — then v2 replaced the functional form rather than fitting weights. LOOCV was never attempted |
| CAL-03 | Escalation / de-escalation thresholds | Set by hand (1.55/1.70/2.30), then derived for v2 (0.0). **A6 exposes them for sweeping** |
| CAL-04 | Hysteresis dwell N | **Never swept** — no CLI flag existed. A6 fixes this |
| CAL-05 | Integrity threshold | Fixed at χ²₀.₉₉ = 9.21. **Precision/recall has never been computed. A2 produces it** |
| CAL-02 | Q, R covariances | Process noise 1.0 m/cycle in position under the default prior. With `--imuPrior` the IMU terms exist and are derived, not tuned: angle random walk from the data-sheet noise density (verified by an Allan-deviation check in G17), bias and stride terms labelled ours in `ca-tbl-imu-model.h` |
| CAL-06 | APC loop parameters | Implemented; not swept |
| CAL-07 | Flood control: hop limit, cache, jitter | Implemented; not swept |
| CAL-08 | Path metric α, β, γ | **No substrate** — needs C5 |
| CAL-09 | Arbitration weights λ₁–λ₄ | Scorer exists offline. **C4 produces these** |
| CAL-10 | Hop-count slack bound | **No substrate** — needs C5 |

---

## 6. Sequencing against the engine work

1. Answer **DEC-B, DEC-C** — no code, no compute, blocks Ch. 9 and 10.
2. Replace `CA-DBL_Phase1_Blueprint.md` §6 with `MASTER_PLAN.md` §2.1. **13 of its 20 rows are
   wrong.** This is the single highest-value thesis-side edit and it takes minutes.
3. Draft **Ch. 11** now. It is the best-supported chapter and depends on nothing outstanding.
4. Draft **Ch. 9** once DEC-B and DEC-C land.
5. Hold **Ch. 10** until A2 reports.
6. Hold **Ch. 12** until Track C's scope is settled.
7. **Ch. 8 last**, against what was measured — not what was specified.

---

## 7. The phase-boundary rule, restated

Phase 1 contains **no simulation results, no performance claims, no hardware observations.**

That rule now cuts both ways and the second direction is the awkward one: Phase 1 must not
*contradict* what Phase 2.1 measured either. A chapter that specifies a mechanism as a
contribution, submitted alongside a repository that has measured that mechanism as not paying for
itself, is internally inconsistent even though it contains no results.

The resolution is **§8.10** — *what Phase 1 does and does not establish* — and DEC-F's split of
Chapter 14. Specify honestly, mark clearly what is unvalidated, and let Phase 2.1 report.
