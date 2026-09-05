# C3 and C4 re-measured on the fixed provider

**2026-09-06.** Both mechanisms were previously measured while `EnginePhyProvider`
modelled one field node, so the peer tag was inaudible to every anchor. The provider fix
(`3c95a86`) changes that substrate, which re-opens both results. This records what
survived, what got stronger, and what is measured for the first time.

Tree: `wip/c34-on-provider` — `main` (provider fix) merged with `wip/c4-arbitration`,
which already contained `wip/c3-identity-query`.

---

## Null control — the registered abort condition

`report/PREREG_C4.md` set the abort condition: a changed `arm_summary.csv` stops the
report; the 1-ulp trace divergence carried by the C3/C4 branches does not.

Against `results/baseline_v7`'s own recorded invocation:

```
MATCH   arm_summary.csv
DIFFER  all six traces
```

**Not aborted.** The trace divergence was then quantified rather than assumed benign,
against the provider-only binary's traces — which reproduce `baseline_v7`'s seven digests
exactly and are therefore a byte-exact stand-in for traces the repository does not track:

| | |
|---|---|
| differing cells | **161 of 63 360** |
| largest relative difference | **3.3 × 10⁻¹³** |
| columns affected | `meanSigmaM`, `meanSigmaRawM`, `estXhp`, `estYhp`, `mahalanobis` |
| row count, header | identical |

That is last-digit rounding in columns printed to 13 significant figures. No reported
quantity moves; `arm_summary.csv` is byte-identical.

---

## C3 — identity-scoped query: the finding survives, and is now much better evidenced

The original result was **one number pair at one seed**: 30 answered when the query named
the measured tag, 0 answered and 28 correctly ignored when it named the peer.

Re-measured across **3 seeds × 6 arms × 2 directions = 36 cells**, 3600 s,
`--nTags=2 --identityQueryPeriodS=60`:

| query names | answered | correctly ignored |
|---|---|---|
| the measured tag (`--identityQueryTarget=1`) | 23 – 34 | **0 in all 18 cells** |
| the peer (`--identityQueryTarget=2`) | **0 in all 18 cells** | 23 – 34 |

**Discrimination is perfect in both directions, with no exceptions.** The zeros are the
result: never a spurious answer when another tag is named, never a missed answer when this
tag is named.

The ignored counter is what makes the second row mean anything. A query that reaches the
tag and is deliberately not answered is indistinguishable from a query that never arrived,
unless the silence is counted — which is why it is counted rather than dropped.

**Why this needed re-running.** The original was measured while the peer could not be
heard by any anchor. The mechanism under test is at the *responder*, so the finding was
never likely to invert; but the counts were produced on a substrate with a structural
defect in it and were not citable until reproduced without it. They now are, and the
single-seed exposure that rule 8 warns about is gone.

**Process note, recorded rather than glossed:** this re-measurement was run **without its
own pre-registration**. It re-tests a claim that already existed rather than proposing a
new one, and its predictions were fixed by the original result, but rule 13 says
"before the first cell runs" without that exemption. C4 below *was* pre-registered.

---


**A provenance gap in the original, worth recording.** The original C3 measurement's
duration appears in **no sidecar and no results directory** — only the prose "seed 7,
`--nTags=2`, one anchor per tier querying every 60 s" in a commit message. That the counts
match at 3600 s is *evidence* the original was a 3600 s run, not a *record* of one. This is
the same class of gap that withdrew §4.1: a number whose invocation was never written to
disk. It costs nothing here because the finding reproduced, and it would have cost the
result if it had not.

## C4 — arbitration: deny and pre-empt execute for the first time

Pre-registered at `b877030` before the run. `--nTags=2 --uwbArbitration=true
--sendHandshakeFrames=true`, seed 7, 600 s.

| arm | granted | denied | pre-empted | grants by target |
|---|---:|---:|---:|---|
| `static_868` | 527 | 0 | 0 | t2: 527 |
| `static_2p4` | 669 | 0 | 0 | t2: 669 |
| `static_uwb` | 1356 | **248** | **201** | t1: 711, t2: 645 |
| `fixed_threshold` | 885 | **148** | **75** | t1: 303, t2: 582 |
| `ca_tbl_no_integrity` | 885 | **148** | **75** | t1: 303, t2: 582 |
| `ca_tbl_full` | 885 | **148** | **75** | t1: 303, t2: 582 |

**P1 — more than one requester. CONFIRMED.** Both target ids appear. Previously every
grant carried target 1 and the arbiter was choosing between one candidate.

**P2 — the deny path executes. CONFIRMED.** 148–248 denials on the arms that reach UWB.

**P3 — pre-emption executes. CONFIRMED.** 75–201 pre-emptions. This was the weakest
prediction, since pre-emption additionally needs a higher-scoring request to arrive during
an active grant; at this offered load it happens readily.

### Two consistency checks that pass

- **`static_868` and `static_2p4` show target 2 only, and zero denials.** Those arms never
  escalate to UWB, so the measured tag never issues a UWB request and only the peer does.
  One requester, nothing to arbitrate. The mechanism is inactive exactly where it should be.
- **`ca_tbl_no_integrity` and `ca_tbl_full` are identical** (885/148/75, 303/582). Expected:
  the integrity check does not fire in 600 s at seed 7, so the two arms have not yet
  diverged. This is a check on the run, not a result about the arms.

### What is not claimed

- **One seed, one configuration, 600 s.** These are control-flow claims — which paths
  execute — and they were chosen to be control-flow claims for that reason. No deny *rate*
  is claimed, and nothing here says how denials vary with load, geometry or battery state.
  That is a sweep and it has not been run.
- **Magnitudes are not comparable to the pre-provider measurement.**
  `LoRaDistributionSampler` draws from ns-3's default stream with no `SetStream`, so any
  change in the number of `ProcessTransmission` calls displaces the stream for every link
  in the run. The earlier "granted 661, denied 0" and these numbers are not two points on
  a common scale.
- **The peer is still not a measured subject.** It has no policy and no energy model. Its
  requests are real and arbitrated; its localisation is not solved and its energy is not
  accounted. That is C2-full.

---

## Status change

C4 was recorded as **Partial — implemented and null-controlled, but cannot be exercised**.
Its two unexercised paths now execute under a pre-registered test. The remaining gap is a
sweep, not a blocker.
