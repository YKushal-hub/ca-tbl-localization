# Pre-registration — the anti-entropy exchange runs both ways

**Written 2026-09-07, before the first measurement run.** Registered under
`CLAUDE.md` rule 13. Corrected tri-band topology, reference `baseline_v8`.

---

## 0. Why this exists

Two mechanisms are already built, bounded, gated and null-controlled, and **neither
fires**:

- `--finderPseudoAnchor` (`937a55a`) substitutes the position a finder *reports* for
  the true position the filter is currently handed, inflating the measurement variance
  by that position's uncertainty along the range direction. Measured on/off: identical
  rmse, energy and escalation counts.
- The hop-provenance term (`57ebb41`) reaches the confidence score in the shape
  `docs/RELAYED_RANGING_ERROR_MODEL.md` §5 specifies. Its only source of relayed
  variance is peer tags, whose filter-estimated positions are uncertain enough that
  admission condition A3 refuses 12 of 14.

Both wait on the same precondition: **a node must hold a table entry for the nodes it
ranges against.** Today it does not, for two reasons found in the code:

1. **The casualty never asks.** `ca_tbl_causal_sim.cc:3155` sends `DIGEST` only from
   `finderMac`. The responder half is complete and identity-agnostic
   (`ca-tbl-mac.cc:909`: any node hearing a `DIGEST` replies with an `ENTRY_PUSH`
   carrying its own entry), so 4 of 4 finders learn where the casualty is and the
   casualty learns nothing about them.
2. **A finder publishes its position once and never again.**
   `ca_tbl_causal_sim.cc:2880` calls `PublishSelfEntry` inside the *construction* loop,
   at the finder's spawn position. Finders then walk for the whole run. Fixing (1)
   alone would deliver a spawn-time position — the substitution would fire on data
   that is wrong by however far the finder has walked, which is worse than not firing.

This campaign closes both halves and measures whether the two waiting mechanisms
then do anything.

## 1. Question

With the anti-entropy exchange running in both directions, does the casualty hold
current finder entries, and do `--finderPseudoAnchor` and the hop-provenance term
become live rather than inert?

## 2. Population and denominator

Seed 7 first for the mechanism check, then seeds 1–8 paired for any effect claim.
`--duration=600 --boxHalf=1000 --nFinders=4 --finderMode=1 --sendHandshakeFrames=true`,
arm `ca_tbl_full`. Denominator for entry-holding is **cycles**, 240 per run at
`dt = 2.5 s`. Denominator for substitution is **finder range measurements formed**,
not cycles — a cycle with no finder in the poll set cannot substitute and must not
count against the rate.

Observation window is the whole run. Entries that arrive in the final cycle are
counted as held; there is no truncation rule to state because nothing here is a
duration.

## 3. Metric and estimator

Reported to stdout, **not** to `arm_summary.csv` — a new column would break
byte-identity against `baseline_v8` (house pattern, `:3290`).

- `finderEntriesHeld` — finder ids present in the casualty's table for the active
  tier, at end of run, out of `nFinders`.
- `finderEntryAgeS` — mean `t - entry.timestampS` over cycles where at least one is
  held. **This is the quantity that decides whether the mechanism is usable**, not
  whether it fires.
- `pseudoAnchorSubs` — count of range measurements where the reported position was
  substituted for the true one.
- `relayedVarShare` — the provenance term's input, already computed.

## 4. Prediction

Committed before running:

- **P1.** With the flag off, output is byte-identical to `baseline_v8` on
  `arm_summary.csv` and all traces. This is a null control, not a prediction about the
  world.
- **P2.** With the flag on, `finderEntriesHeld` reaches 4/4 and `pseudoAnchorSubs > 0`.
  The mechanism becomes live.
- **P3.** `finderEntryAgeS` lands in the **same order as the casualty-entry age
  already measured** (39.4 s at a 30 s digest period). The exchange is symmetric, so
  the latency should be too. A much larger age means the reverse direction is losing
  frames the forward direction does not, and that is a finding rather than a success.
- **P4, and this is the one I expect to disappoint.** Accuracy will get **worse**, not
  better, when the substitution is on. The filter is currently given finders' true
  positions; replacing them with reported positions that are stale by P3's age and
  inflating the variance accordingly is **strictly less information**. The mechanism's
  purpose is to stop the results being flattered, not to improve them.

## 5. Falsifier

- P2 fails if `finderEntriesHeld` is 0 after the change — the exchange is still
  one-way and the diagnosis in §0 was wrong.
- P4 is falsified if accuracy *improves* with substitution on. That would mean the
  true-position path was doing something other than handing the filter better data,
  and the substitution would need explaining before it could be reported.

## 6. Decision rule

**This campaign reports a mechanism, not an effect.** The claim it can support is
"the exchange is bidirectional and the two waiting mechanisms now fire, at a measured
dissemination latency". Any accuracy or energy difference is reported with its sign
and left as a single-configuration observation unless it is swept across seeds 1–8
paired, which is a separate registration.

Rule 8 applies and is the reason: a single-seed difference across two binaries is not
a result, and this change alters the number of transmissions, which displaces the
shared RNG stream for every link (`docs/TECH_DEBT.md` §1.1).

## 7. Abort conditions

- **Abort if the null control fails.** Flag off must reproduce `baseline_v8`
  byte-for-byte on all seven digests. If it does not, the change has leaked into the
  default path and nothing else measured here is admissible.
- **Abort if `duty_868` exceeds the 2.5% WPC Table-II field cap.** The casualty now
  transmits `DIGEST` frames it did not before. If dissemination breaks the regulatory
  budget, the mechanism is not deployable and the cost is not a tuning matter.

## 8. What this campaign cannot establish

- **Not whether provenance helps.** With hop depth capped at 1 (`A2`), `hopCount` is
  binary, and the provenance term is driven by relayed variance share rather than hop
  count. Making it fire is not the same as showing it carries information the
  covariance does not. That is the separate PCS campaign named in
  `docs/TECH_DEBT.md` §3.2.
- **Not the energy cost of dissemination in general.** One digest period, one finder
  count, one box size.
- **Nothing about staleness management.** Finders will still act on whatever age P3
  measures, with nothing bounding it. That is listed as debt and is not addressed here.
