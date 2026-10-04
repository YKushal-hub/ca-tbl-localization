# L.2.5 pre-registration — how old is what a proxy is asked about?

Registered 2026-09-29, committed together with the observable it reads and the scripts
that run and analyse it, **before any registered cell ran**. `docs/TECH_DEBT.md` §L.2,
row L.2.5:

> The proxy answers only inside 120 s and at this deployment entries are normally older,
> so it refuses (7 refusals / 0 answers at the spec bound). … Done when: the bound is
> justified by measurement rather than inherited.

## 1. Why a count could not settle it

"7 refusals / 0 answers" (04dcb11) is a comparison of an unmeasured age against one
number. It is equally consistent with entries 121 s old and 1 200 s old, and those call
for opposite changes. Its invocation was never recorded; the commit message says only that
it ran with `--nTags=3`, the query naming a **peer** tag (manifest 2), and "propagation
enabled". So the question is re-asked with the age itself as the observable.

## 2. The observable

A **proxy check** is one arrival of an identity-scoped QUERY at a radio that is not the
target it names (`CaTblMac::ProxyAnswerFor`). This commit records every check to
`proxy_checks_<arm>_seed<N>.csv` (written only under `--queryProxy`, digested in
`run_id.txt`): the query's packet id, the radio's node, class and tier, whether that
radio's table held an entry about the named target, and if so the entry itself.

- **Age** = arrival time − the entry's timestamp, recorded **before** the freshness
  decision, so the distribution does not depend on the bound it judges.
- **Denominator** = checks. A check that held nothing is counted and carries no age; ages
  are summarised over held checks only and always reported beside the held/checks ratio.
- **Window** = the whole run, 0–1 800 s. **Truncation**: a query still in flight at
  1 800 s produces no check and is not counted; an age is observed at the instant of the
  check, so it is never censored. A radio that never receives an entry contributes
  held = 0 rows only.
- The query side is printed beside it (QUERY frames transmitted, relayed, received,
  refused by duty, CAD-exhausted, lost to collision, all radios), because a flood that
  never arrives is invisible from the checks.

Null control, observed before registration: with `--queryProxy` off, `~/catbl_ref/nc.sh`
PASS (NC1 7/7, NC2 2/2 files byte-identical to the frozen 084a7ff reference). With it
**on** (seed 5, 900 s, default deployment, four finders, target 1) `arm_summary.csv` and
the trace are byte-identical to the frozen reference as well: the record reads the table
and changes nothing.

## 3. Cells

`bash scripts/l25_proxy_age_run.sh batch <cell> 7 11 13`, into
`results/l25_proxy_age_20260929/<cell>/seed_<n>/`, single arm `ca_tbl_full`, 1 800 s,
`--queryProxy=true --identityQueryPeriodS=30`, four finders
(`--nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0
--sendHandshakeFrames=true --finderDigestPeriodS=20`).

| cell | deployment | queried target | role |
|---|---|---|---|
| `searcher_t1` | arc: `--boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180` | the casualty, manifest 1 | **primary** |
| `sparse_t1` | `--gwGrid=2 --boxHalf=5000` (the sparsest finder regime probed, ~200–227/720 no-fix cycles at seeds 7/11/13) | the casualty, manifest 1 | **primary** |
| `searcher_t2` | arc | a peer, `--nTags=2 --identityQueryTarget=2` | secondary |
| `sparse_t2` | grid 2, 5 000 m | a peer, as above | secondary |

The casualty is primary because it is what a search queries for, and a proxy matters when
the requester cannot reach the target. The two regimes differ **only** in deployment. The
30 s query period is **ours**; queries come from the first gateway per tier, as the C3
driver has always issued them. Seeds 7, 11, 13 are **populations**: no claim here compares
arms, and none compares a magnitude across binaries.

## 4. Seen before registering — disclosed, because it shaped the predictions

- `~/catbl_probe/v3/f_def` (seed 7, default deployment, finders, no proxy): stdout
  `TABLE: 4/4 finders hold the casualty entry, mean age 24.4 s` (and 14.4 s for
  `static_868`).
- Smoke run, seed 5, default deployment, 900 s, target 1: 55 checks of 7 distinct
  queries, 25 held, ages p50 12.5 s, max 37.0 s, 25/25 within 120 s. 142 QUERY frames
  transmitted, 53 lost to collision.
- Runner/analysis self-test at **seed 5** (unregistered) on `searcher_t1`: 119 checks,
  86 held, p50 11.0 s, p90 19.9 s, max 38.5 s, 86/86 within 120 s; held estimate vs truth
  5.8 m fresh, 142.7 m after 120 s at the casualty's 1.2 m/s. And on `sparse_t2`: 34
  checks, 12 held, p50 67.4 s, p90 229.9 s, max 471.3 s, 9/12 within 120 s.
- Also from that self-test: all 86 held casualty entries carried the trace's estimate of
  the cycle **before** their timestamp. `PublishSelfEntry` runs before `policy.Step` in
  the loop, so the entry is stamped `t` and holds the estimate of `t − dt` — its age is
  understated by one cycle (2.5 s). Reported, not registered, not fixed here.

## 5. Predictions and falsifiers

- **P1** `searcher_t1` → S-a. Falsified by S-b or S-c.
- **P2** `sparse_t1` → S-a, with less confidence: the box is 2.5× wider and finders can
  be further from the casualty. Falsified by S-b or S-c.
- **P3** (secondary) the peer's pooled median age exceeds the casualty's in the same
  regime, in both regimes: finders converge on the casualty, not on the peer, so fewer
  of their digests reach the peer. Falsified if it does not, in either regime.

## 6. Decision rule — supply side, applied per primary cell to the pooled held ages

- **S-a** — at least 90 % of held ages ≤ 120 s: the bound does not bind.
- **S-b** — pooled median > 120 s: the spec bound is too tight for this topology.
- **S-c** — otherwise: it binds on a tail.

Quantiles are nearest-rank (`ceil(p·n)`), the rule the simulator prints.

**Action rule.** In every outcome the default stays 120 s — it is the spec's, and moving
it would move every proxy run — and the bound is not raised to make answers appear, which
would be answering from entries the system itself classifies as stale. The flag that sets
it, `--queryStalenessBoundS`, already exists; its help text gains the measurement.

- both S-a → record that L.2.5's premise does not hold for the casualty in either regime;
- any S-b → record that the spec bound is too tight here, and file faster dissemination as
  the change that would address it — a mechanism with its own flag, registration, null
  control and gate, **not built in this item**;
- otherwise → record the tail the bound refuses.

The secondary cells decide nothing; they test P3 and are the closest re-run of the
04dcb11 configuration.

## 7. Demand side — descriptive, decides nothing

For held casualty entries (`*_t1`): error against truth at the query, error at the
entry's own timestamp, and displacement since, by age bin (0–30, 30–60, 60–120, 120–300,
>300 s), truth interpolated linearly between trace cycles. And, from the traces alone,
the error of an estimate held for 30/60/120/300/600 s against the casualty's truth then.
This is what an entry at the bound is worth; it is reported beside the verdict so the
bound is argued from both sides, and no threshold on it was chosen in advance.

## 8. Abort conditions (`scripts/l25_proxy_age.py`)

- **A1** a run without a clean optimized build (`dirty 0`), with an invocation, binary or
  outdir other than the registered one, without a digest block, or whose proxy record,
  trace or summary does not match its digest.
- **A2** no held entry in a cell: the observable is unreachable there.
- **A3** fewer than 20 pooled held ages: not classified.
- **A4** the stdout summary and the CSV disagree on checks, held, answered or distinct
  queries; an age that is not `t − entryTimeS`; a check naming another manifest.

An abort makes that cell CANNOT DECIDE; no verdict is computed from what survived. Each
of A1 (outdir, digest, dirty), A3 and A4 was observed to fire on tampered copies of the
seed-5 self-test before registration. The self-test overrides (`L25_SEEDS`,
`L25_ALLOW_DIRTY`) force exit 3 and print a banner, so nothing produced under them can
pass as the registered analysis.
