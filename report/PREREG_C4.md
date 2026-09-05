# Pre-registration — C4 arbitration, re-measured on the fixed provider

**Committed before the first C4 run.** Rule 13.

## Why this is being run again

C4's arbiter was implemented, null-controlled and observed **granting**, but its **deny**
and **pre-empt** paths could not be exercised: `EnginePhyProvider` modelled one field
node, so the peer's `ESC_REQUEST`s were never delivered and every anchor saw exactly one
requester. Arbitration between one candidate is not arbitration.

The provider now carries a position per node (`3c95a86`), and peer frames are delivered:
`+112 325` channel deliveries at `--nTags=2`, seed 7. So the precondition C4 was blocked
on is gone, and the question is whether that is *sufficient*.

## Predictions

**P1 — the arbiter now sees more than one requester.** With `--uwbArbitration=true
--nTags=2`, anchors receive `ESC_REQUEST`s originating from **both** manifest ids, not
only from the measured tag.

*Falsifier:* every request still carries the same originator. That would mean something
above the provider is still discarding peer requests, and the provider fix was necessary
but not sufficient — which is reported as such, not worked around.

**P2 — the deny path executes.** `denied > 0` across the run.

*Falsifier:* `denied == 0` while P1 holds. That is a **real and reportable outcome**, not a
failure of the run: it would mean two requesters are visible but the scoring function never
ranks one below the grant threshold, which is a statement about `U(r)`'s calibration rather
than about delivery. If this happens the calibration is reported, not tuned until deny
fires — tuning a scoring function until a path executes is fitting the mechanism to the
desired observation.

**P3 — pre-emption.** `preempted > 0`.

*Falsifier:* `preempted == 0`. Weaker than P2: pre-emption additionally requires a
higher-scoring request to arrive *during* an active grant, which is a timing coincidence
and may simply be rare at this offered load. **A zero here is reported as "not observed at
this load", never as "the path does not work"** — rule 4: an exact zero is a control-flow
claim, and claiming the path is broken would need proof it was reachable and not taken.

## Abort conditions

- If the null control for the merged C3+C4+provider tree does not reproduce
  `arm_summary.csv` against `baseline_v7`, no C4 number is reported. The 1-ulp trace
  divergence carried by the C3/C4 branches is **expected** and is not an abort condition;
  a changed `arm_summary` is.
- If P1 fails, P2 and P3 are not interpreted at all — a deny count from a single-requester
  population is meaningless.

## Stated in advance

- **Counts are confounded across binaries.** `LoRaDistributionSampler` draws from ns-3's
  default stream with no `SetStream`, so any change in the number of `ProcessTransmission`
  calls displaces the stream for every link. Comparisons of *magnitudes* between the
  pre-provider and post-provider binaries are not admissible. P1–P3 are control-flow
  claims about which paths execute, and are chosen for that reason.
- **One configuration.** No claim is made about how deny rates vary with load, geometry or
  battery state; that is a sweep and is not run here.
