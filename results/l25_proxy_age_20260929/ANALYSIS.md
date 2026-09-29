# L.2.5 analysis — the age of what a proxy is asked about

`python3 scripts/l25_proxy_age.py results/l25_proxy_age_20260929` at `b16531f`, exit 0; the
script's output follows verbatim, then an independent recount and the reading. Registration:
`report/PREREG_L25_PROXY_AGE.md`. Runs: `RUNS.md`.

**Tier labels below are the QUERY frame's label, not the radio that heard it** — every query
travels on gateway 101's 868 MAC (`docs/TECH_DEBT.md`, found while closing L.2.5), so every
check is at an 868 radio.

### searcher_t1

`--arms=ca_tbl_full --duration=1800 --trace=true --queryProxy=true --identityQueryPeriodS=30 --boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180 --nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 --sendHandshakeFrames=true --finderDigestPeriodS=20 --identityQueryTarget=1`, seeds 7, 11, 13

| seed | queries reaching a proxy radio | checks | held | answered | p10 | p50 | p90 | max | within 120 s |
|---|---|---|---|---|---|---|---|---|---|
| 7 | 35 | 202 | 118 | 118 | 2.6 | 11.5 | 32.8 | 68.1 | 118/118 |
| 11 | 14 | 77 | 46 | 46 | 2.3 | 13.0 | 24.4 | 53.6 | 46/46 |
| 13 | 33 | 179 | 101 | 101 | 2.2 | 11.0 | 19.9 | 53.0 | 101/101 |
| **pooled** | 82 | 458 | 265 | 265 | 2.4 | 11.5 | 28.6 | 68.1 | 265/265 |

Checks / held by the class and tier of the radio asked: ANCHOR tier 0: 0/185, ANCHOR tier 1: 0/2, RESCUER tier 0: 256/262, RESCUER tier 1: 9/9.

QUERY frames, all radios, summed over seeds: transmitted 1013 (relayed 538), received 545, refused by duty 0, CAD-exhausted 9, lost to collision 308.

C1 (descriptive): mean held age at query 13.4 s; mean age of the finders' casualty entry at end of run (stdout TABLE line), seeds 7/11/13: 14.4/59.4/19.4 s.

| held-entry age at query | n | median error vs truth at query (m) | median error at its own timestamp (m) | median displacement since (m) |
|---|---|---|---|---|
| (0, 30] s | 242 | 16.8 | 4.9 | 12.3 |
| (30, 60] s | 22 | 44.0 | 5.1 | 40.1 |
| (60, 120] s | 1 | 81.1 | 5.7 | 80.8 |
| (120, 300] s | 0 | - | - | - |
| > 300 s | 0 | - | - | - |

Entry position against the trace estimate: equal to the estimate of its own timestamp's cycle 1, of the cycle before it 264, neither 0.

| lag held (s) | n cycles | median true displacement (m) | p90 (m) | median error of the held estimate vs truth then (m) |
|---|---|---|---|---|
| 0 | 2160 | 0.0 | 0.0 | 4.5 |
| 30 | 2124 | 35.6 | 35.8 | 36.5 |
| 60 | 2088 | 71.3 | 71.5 | 71.8 |
| 120 | 2016 | 142.5 | 142.8 | 142.8 |
| 300 | 1800 | 356.1 | 356.6 | 355.7 |
| 600 | 1440 | 711.9 | 712.7 | 706.2 |

**Verdict:** S-a: the bound does not bind (100.0% within 120 s)

### sparse_t1

`--arms=ca_tbl_full --duration=1800 --trace=true --queryProxy=true --identityQueryPeriodS=30 --gwGrid=2 --boxHalf=5000 --nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 --sendHandshakeFrames=true --finderDigestPeriodS=20 --identityQueryTarget=1`, seeds 7, 11, 13

| seed | queries reaching a proxy radio | checks | held | answered | p10 | p50 | p90 | max | within 120 s |
|---|---|---|---|---|---|---|---|---|---|
| 7 | 7 | 20 | 18 | 18 | 2.4 | 11.3 | 40.5 | 86.3 | 18/18 |
| 11 | 3 | 6 | 5 | 3 | 19.4 | 44.4 | 197.6 | 197.6 | 3/5 |
| 13 | 5 | 16 | 16 | 16 | 6.7 | 14.6 | 38.4 | 49.6 | 16/16 |
| **pooled** | 15 | 42 | 39 | 37 | 3.7 | 17.2 | 49.6 | 197.6 | 37/39 |

Checks / held by the class and tier of the radio asked: ANCHOR tier 0: 0/3, RESCUER tier 0: 39/39.

QUERY frames, all radios, summed over seeds: transmitted 537 (relayed 58), received 59, refused by duty 0, CAD-exhausted 1, lost to collision 13.

C1 (descriptive): mean held age at query 29.2 s; mean age of the finders' casualty entry at end of run (stdout TABLE line), seeds 7/11/13: 54.4/164.4/114.4 s.

| held-entry age at query | n | median error vs truth at query (m) | median error at its own timestamp (m) | median displacement since (m) |
|---|---|---|---|---|
| (0, 30] s | 27 | 178.4 | 177.5 | 13.5 |
| (30, 60] s | 9 | 265.2 | 251.0 | 42.1 |
| (60, 120] s | 1 | 129.6 | 164.5 | 102.9 |
| (120, 300] s | 2 | 1588.1 | 1432.3 | 215.8 |
| > 300 s | 0 | - | - | - |

Entry position against the trace estimate: equal to the estimate of its own timestamp's cycle 0, of the cycle before it 39, neither 0.

| lag held (s) | n cycles | median true displacement (m) | p90 (m) | median error of the held estimate vs truth then (m) |
|---|---|---|---|---|
| 0 | 2160 | 0.0 | 0.0 | 320.5 |
| 30 | 2124 | 35.7 | 35.8 | 336.0 |
| 60 | 2088 | 71.3 | 71.5 | 355.0 |
| 120 | 2016 | 142.5 | 142.8 | 397.1 |
| 300 | 1800 | 356.3 | 356.7 | 567.5 |
| 600 | 1440 | 712.5 | 712.8 | 864.2 |

**Verdict:** S-a: the bound does not bind (94.9% within 120 s)

### searcher_t2 (secondary: reported, decides nothing)

`--arms=ca_tbl_full --duration=1800 --trace=true --queryProxy=true --identityQueryPeriodS=30 --boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180 --nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 --sendHandshakeFrames=true --finderDigestPeriodS=20 --nTags=2 --identityQueryTarget=2`, seeds 7, 11, 13

| seed | queries reaching a proxy radio | checks | held | answered | p10 | p50 | p90 | max | within 120 s |
|---|---|---|---|---|---|---|---|---|---|
| 7 | 38 | 259 | 36 | 11 | 31.5 | 253.0 | 658.3 | 789.2 | 11/36 |
| 11 | 37 | 245 | 57 | 6 | 119.0 | 618.2 | 1331.6 | 1701.5 | 6/57 |
| 13 | 39 | 276 | 37 | 4 | 52.6 | 1007.0 | 1614.8 | 1714.4 | 4/37 |
| **pooled** | 114 | 780 | 130 | 21 | 57.4 | 493.4 | 1360.5 | 1714.4 | 21/130 |

Checks / held by the class and tier of the radio asked: ANCHOR tier 0: 0/268, ANCHOR tier 1: 0/2, RESCUER tier 0: 124/382, RESCUER tier 1: 6/20, TARGET tier 0: 0/102, TARGET tier 1: 0/6.

QUERY frames, all radios, summed over seeds: transmitted 1219 (relayed 744), received 824, refused by duty 0, CAD-exhausted 23, lost to collision 223.

**Verdict:** S-b: the bound is too tight (median 493.4 s > 120 s)

### sparse_t2 (secondary: reported, decides nothing)

`--arms=ca_tbl_full --duration=1800 --trace=true --queryProxy=true --identityQueryPeriodS=30 --gwGrid=2 --boxHalf=5000 --nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 --sendHandshakeFrames=true --finderDigestPeriodS=20 --nTags=2 --identityQueryTarget=2`, seeds 7, 11, 13

| seed | queries reaching a proxy radio | checks | held | answered | p10 | p50 | p90 | max | within 120 s |
|---|---|---|---|---|---|---|---|---|---|
| 7 | 4 | 20 | 2 | 1 | 17.1 | 17.1 | 354.2 | 354.2 | 1/2 |
| 11 | 9 | 28 | 6 | 1 | 9.9 | 286.1 | 1455.7 | 1455.7 | 1/6 |
| 13 | 9 | 32 | 12 | 3 | 92.1 | 420.9 | 1150.3 | 1216.5 | 3/12 |
| **pooled** | 22 | 80 | 20 | 5 | 17.1 | 384.9 | 1156.1 | 1455.7 | 5/20 |

Checks / held by the class and tier of the radio asked: ANCHOR tier 0: 0/3, RESCUER tier 0: 20/56, TARGET tier 0: 0/21.

QUERY frames, all radios, summed over seeds: transmitted 561 (relayed 83), received 84, refused by duty 0, CAD-exhausted 0, lost to collision 10.

**Verdict:** S-b: the bound is too tight (median 384.9 s > 120 s)

### Registered decision

- searcher_t1: S-a: the bound does not bind (100.0% within 120 s)
- sparse_t1: S-a: the bound does not bind (94.9% within 120 s)
- searcher_t2 (secondary): S-b: the bound is too tight (median 493.4 s > 120 s)
- sparse_t2 (secondary): S-b: the bound is too tight (median 384.9 s > 120 s)

Action (registered): record that the bound does not bind for the casualty in either registered regime; the default stays 120 s; L.2.5's premise is refuted for these regimes.

## Independent recount (rule 3)

From the raw `t` and `entryTimeS` columns of the proxy records with `awk` — not from the
`ageS` column and not through the Python:

| cell | checks | held | answered | held age <= 120 s | median (nearest rank) | max |
|---|---|---|---|---|---|---|
| searcher_t1 | 458 | 265 | 265 | 265 (100.0 %) | 11.5 | 68.1 |
| sparse_t1 | 42 | 39 | 37 | 37 (94.9 %) | 17.2 | 197.6 |
| searcher_t2 | 780 | 130 | 21 | 21 (16.2 %) | 493.4 | 1714.4 |
| sparse_t2 | 80 | 20 | 5 | 5 (25.0 %) | 384.9 | 1455.7 |

It agrees with the script on every figure, and "answered" equals "held age <= 120 s" in every
cell, which is the bound doing exactly what it says.

## Reading

**Predictions.** P1 held: `searcher_t1` is S-a, 265/265. P2 held, narrowly: `sparse_t1` is S-a
at 94.9 % on 39 held ages from 15 queries, and both refusals are seed 11's (3 queries, 3/5
inside). P3 held: the peer's pooled median exceeds the casualty's in both regimes, 493.4 s
against 11.5 s and 384.9 s against 17.2 s.

**Registered action.** Record that the bound does not bind for the casualty in either
registered regime; the default stays 120 s. L.2.5's premise — "at this deployment entries are
normally older" — is refuted for the casualty and true for a peer, which is what the 04dcb11
figure queried.

**Why a peer's entries age** (not registered: a debug re-run of `searcher_t2` seed 7 with
`--vizEvents=true`, whose proxy record and trace are byte-identical to the registered run's).
The finders' DIGESTs were delivered 305 times to the casualty and 13 times to the peer; the
casualty's ENTRY_PUSH reached a finder 299 times, the peer's 7. With `--digestSummary` off —
the default, and here — a DIGEST is answered with the responder's own entry and ENTRY_PUSH is
unicast to the asker (`CaTblMac::DeliverRx`), so an entry reaches a holder only as its
subject's answer to that holder's own digest. What the finders do not hear, nobody refreshes.

**What an entry at the bound is worth** (descriptive, as registered). In the searcher regime
the casualty walks a steady ~1.19 m/s (median displacement 35.6 m per 30 s, p90 35.8 m); an
estimate held 120 s is 142.8 m from it, against 4.5 m fresh, and among the entries proxies
actually held, those up to 30 s old were 16.8 m off at the query and those 30–60 s old
44.0 m. In the sparse regime the fix is the error — 320.5 m fresh — and 120 s of age adds
about 77 m (397.1 m). So 120 s costs little where fixes are poor and is generous where they
are good. That is recorded about the spec's bound, not acted on: no threshold on it was
registered.

**Where the population is lost.** Delivery, not age, limits the number of checks: in
`searcher_t1` 1 013 QUERY transmissions (538 of them relays) lost 308 to collision; in
`sparse_t1` 537 transmissions produced 59 receptions.

**Two things found on the way**, recorded in `docs/TECH_DEBT.md` §L.2 and not fixed:

- Tier labels in the tables above are the QUERY frame's. Every tier's query driver sits on
  gateway 101's 868 MAC, so every check is an 868 radio; `RESCUER tier 1` is an 868 radio
  hearing a frame labelled 2.4 GHz and evaluated with that link model.
- 264 of 265 (`searcher_t1`) and 39 of 39 (`sparse_t1`) held casualty entries equal the trace
  estimate of the cycle *before* their timestamp: `PublishSelfEntry` precedes `policy.Step`,
  so every age is understated by one cycle, 2.5 s. No verdict moves under it — adding 2.5 s to
  every age leaves `searcher_t1` at 265/265 and `sparse_t1` at 37/39, and no casualty age
  lies in (117.5, 120] s.
