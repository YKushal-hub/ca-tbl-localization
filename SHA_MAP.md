# SHA map — attribution rewrite of 2026-09-01

The repository owner directed that an assistant attribution trailer be removed from
commit messages entirely, not merely avoided going forward. A history rewrite was
authorised **once, for that purpose only**, overriding the standing never-rewrite
rule — that rule was adopted as hygiene around a settled vendored import, and an
owner instruction about attribution in his own private repository outranks it.

**Metadata only.** `git diff <old-tip> <new-tip>` was empty: no file content changed,
commit count unchanged at 83, order and dates preserved, linear no-merge shape intact.
Author and committer identity were already correct on all 83 commits — only 27 message
bodies carried a trailer.

30 commits were rewritten; the 53 before them are untouched and keep their SHAs.

| old | new | subject |
|---|---|---|
| `aeeb1cfd7c12` | `e02e45ad9984` | fix: EXCLUDED.md lists handoff v5, and the HIDDEN assertion coun |
| `f3aa980eb7c6` | `7e91d73f726d` | docs: cross-document consistency pass, and withdraw a wrong defe |
| `5d590e4fc1bd` | `f87aaf9c7836` | docs: mark the data pack as a historical record of superseded ca |
| `4ee3c0c3b8d0` | `da328d19d0ba` | docs: restructure the report around what stands |
| `5e616d735a86` | `068e5d5f05ed` | evidence: recovery-policy ablation on clean provenance -- the la |
| `22ee9f90297a` | `ccaedb0746c5` | pre-register: abl_clean, the recovery-policy ablation on clean p |
| `6906ad50880e` | `e44e8e227308` | chore: untrack 12 traces and record the deliberately-ignored fil |
| `27ed203cb141` | `9f2aff65b938` | evidence: Claim 9 re-measured on citeable provenance -- falsific |
| `57dcadf8ea7a` | `cd594c90c362` | pre-register: c9_clean, Claim 9 re-measured on citeable provenan |
| `2f0f0ac55852` | `3e1a270a0dd9` | fix: degeneracy screen is per-mechanism; propagate two withdrawn |
| `2ffa0cc90196` | `55aee233d77e` | docs: withdraw a wrong finding -- the trigger pair WAS reproduci |
| `1601cfb935f9` | `1ac680259c03` | fix: G6 compared a real file against a path that never existed |
| `a0ccb4fd1c51` | `683416334b12` | evidence: baseline_v7 -- regenerate under the corrected engine,  |
| `72bcb7dcb4ea` | `c651743cb48a` | docs: record the provenance gap in the cited evidence base |
| `4ba6f334d655` | `3ce0618955b0` | docs: track the two report documents the citation audit reads |
| `2ca905c50a65` | `cb02c32f516e` | tools: run every gate, and close the three cited-script gaps |
| `4a717a1b56d1` | `b9459c23a418` | fix: track Simulation.md, untrack the third report, and gate bot |
| `a3690c21b374` | `3edfec8fb486` | fix: make the must-stay-untracked rule mechanical, and record wh |
| `aa8328bbdfdc` | `03c15dd607a8` | chore: untrack CA_TBL_HANDOFF_v4.md, which must stay untracked |
| `35466c019062` | `2ab38c2e71fe` | docs: withdraw W1 and N3, correct r80, and document the delibera |
| `302d32f92694` | `1057f6f811dc` | tools: commit the analysis scripts the gate suite requires |
| `e21666dc4b51` | `d34eeb7c474a` | fix: gate suite must not overwrite the evidence it verifies |
| `13a62bb788f9` | `0a489df392d7` | docs: withdraw the reliability and Claim 3 entries after the v7  |
| `7c0c5e513753` | `63bba4fd2c5e` | chore: land the gate, manifest and report state from phases 9-10 |
| `6de0049507ea` | `bd9d76bb2127` | fix: NaN sentinel for undefined mean SNR |
| `846dd80cab81` | `c0313f3f214f` | diag: emit unconditional SNR alongside the conditioned accumulat |
| `7ac9cd58b057` | `dd07cf7efa78` | feat: Phase 10 evidence predicate and suspectRecovery ablation |
| `60605e4fc786` | `976bbefaa369` | fix: record the blast radius of a4e23ad (D1 SF index) |
| `a4e23adc2930` | `0d5e74794d37` | fix: correct SF index base in 2.4GHz and UWB sensitivity tables |
| `b9e650f6b1aa` | `26721ef444d7` | study: add the closed-form propagation module and its defect rec |

## Where old SHAs still appear

**`run_id.txt` sidecars are evidence and were NOT edited.** Rewriting them would be
falsification. 1283 sidecars cite six SHAs that changed:

| old SHA | sidecars | new SHA |
|---|---|---|
| `13a62bb` | 938 | `0a489df392d7` |
| `7c0c5e5` | 181 | `63bba4fd2c5e` |
| `22ee9f9` | 120 | `ccaedb0746c5` |
| `57dcadf` | 40 | `cd594c90c362` |
| `72bcb7d` | 3 | `c651743cb48a` |
| `4ba6f33` | 1 | `3ce0618955b0` |

Those sidecars resolve through this table. The verifier's four-outcome resolution
already has a *does not resolve (history rewritten)* branch; these land in it, and this
file is what turns that outcome from a dead end into a lookup.
