# C3 and C4 on the fixed provider

**2026-09-06.** Both were measured while `EnginePhyProvider` modelled one field node, so
no peer frame reached any anchor. The provider now carries a position per node
(`70e9329`). This re-measures C3, whose counts were produced under that defect, and runs
C4's deny and pre-empt paths, which could not execute at all.

Tree: `wip/c34-on-provider` = main (provider fix) + `wip/c4-arbitration` (which contains
C3). Binary verified current against source mtimes before either run — a stale binary has
cost this project two rounds of wrong conclusions before.

---

## C3 — identity-scoped query, re-measured

**Invocation:** `--seed={7,8,9} --duration=3600 --nTags=2 --identityQueryPeriodS=60
--identityQueryTarget={1,2} --trace=false`. The measured tag is manifest 1; the peer is 2.

**36 cells: 3 seeds × 6 arms × 2 directions. Discrimination is perfect in every one.**

| query names | answered | correctly ignored |
|---|---|---|
| the measured tag (`target=1`) | 23 – 34 | **0** in all 18 cells |
| the peer (`target=2`) | **0** in all 18 cells | 23 – 34 |

The second row carries the finding. Between 23 and 34 queries **reached** the measured tag
in every arm and seed, and it stayed silent in every one. That is scoping suppressing the
response, not a delivery failure — the two are indistinguishable without the ignored
counter, which is why it is counted rather than dropped.

**The original result reproduces.** It reported 30 answered / 28 ignored at seed 7; the
same cell here gives 30 answered / 31 ignored. So C3's numbers were **not** an artefact of
the inaudible peer, and the earlier concern that they might be is discharged.

**Process note, stated rather than buried.** This re-run was executed **without its own
pre-registration**. It re-tests a claim that already existed rather than proposing a new
one, and the outcome was a reproduction, but rule 13 does not carve out that case and this
run does not comply with it. The C4 measurement below **was** pre-registered
(`report/PREREG_C4.md`) before its first run.

**Provenance gap in the original, worth recording.** The original C3 measurement's
duration appears in no sidecar and no results directory — only the prose "seed 7,
`--nTags=2`, one anchor per tier querying every 60 s" in a commit message. That the counts
match at 3600 s is evidence the original was a 3600 s run, not a record of it. This is the
same class of gap that withdrew §4.1: a number whose invocation was never written to disk.

---

## C4 — arbitration, with deny and pre-empt now executing

**Invocation:** `--seed=7 --duration=600 --nTags=2 --uwbArbitration=true
--sendHandshakeFrames=true --trace=false`.

Pre-registered predictions, all three confirmed.

| arm | granted | denied | pre-empted | grants by target |
|---|---:|---:|---:|---|
| `static_868` | 527 | 0 | 0 | target 2: 527 |
| `static_2p4` | 669 | 0 | 0 | target 2: 669 |
| `static_uwb` | 656 | 0 | 0 | target 2: 656 |
| `fixed_threshold` | 1356 | **248** | **201** | target 1: 711, target 2: 645 |
| `ca_tbl_no_integrity` | 885 | **148** | **75** | target 1: 303, target 2: 582 |
| `ca_tbl_full` | 885 | **148** | **75** | target 1: 303, target 2: 582 |

**P1 — the arbiter sees more than one requester. Confirmed.** The three escalating arms
show grants to **both** manifest ids. Before the provider fix every request at every anchor
carried the same originator.

**P2 — the deny path executes. Confirmed.** 148–248 denials on the escalating arms. The
path was dead in every prior run of this mechanism.

**P3 — pre-emption executes. Confirmed.** 75–201 pre-emptions. This was the weakest
prediction, since pre-emption additionally needs a higher-scoring request to arrive during
an active grant; at this offered load it is common rather than rare.

**The three static arms are an internal control, and they behave correctly.** A static arm
never escalates, so the measured tag never emits an `ESC_REQUEST` and only the peer
competes. One requester, so `denied = 0` and `pre-empted = 0` — arbitration with nothing to
arbitrate. That the zeros land exactly where a single-requester population is expected, and
the non-zeros exactly where two requesters are, is what distinguishes this from a counter
that merely started incrementing.

---

## What is not claimed

**No magnitude is compared across the provider fix.** `LoRaDistributionSampler` creates its
five random variables with no `SetStream` (RULES.md §5), so every submodule on the
`ProcessTransmission` path draws from ns-3's default stream, and any change in the number
of transmissions displaces that stream for every link in the run. All the claims above are
control-flow claims — which paths execute, and for which originators — chosen for exactly
that reason.

**`ca_tbl_full` and `ca_tbl_no_integrity` report identical arbitration counts.** Not
interpreted here. The integrity check fires once at seed 7, so the two arms are close by
construction, and rule 8 forbids reading a single-seed cross-arm difference — or a
single-seed cross-arm *identity* — as a result.

**C4 is one seed and one configuration.** Nothing is claimed about how deny rates vary
with load, geometry or battery state. `U(r)`'s calibration is not assessed: the
pre-registration committed in advance to reporting the calibration if deny never fired,
rather than tuning the scoring function until it did. Deny fired, so that branch was not
needed — but the commitment is what makes the observation worth anything.

**The peer is still not a measured subject.** No `CaTblPolicy`, no `CaTblEnergyModel`, by
design. Making it one is C2-full and is still not done.
