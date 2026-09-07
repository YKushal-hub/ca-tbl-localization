# The gate that could not decline to enter, and the exchange that ran one way

**2026-09-08.** Eight seeds, paired. `--duration=600 --boxHalf=1000 --nFinders=4
--finderMode=1 --sendHandshakeFrames=true`, arm `ca_tbl_full`, corrected tri-band
topology, reference `baseline_v8`. Analysis: `scratch/python_handoff/analyse_gate8.py` over
`catbl_work/gate8/`.

Commits: `40e3258`, `5262c59`, `cf25341`, `f88e224`, `edf73a5`. Pre-registration for the
exchange half: `78edc67`, committed before any of it ran.

---

## 1. The headline, and who it belongs to

| config | rmse | energy | escalations | UWB cycles |
|---|---:|---:|---:|---:|
| base — no gate | 6.543 m | 72.03 J | 36.9 | 176.5 |
| `--escalationRequiresEvidence` (static map) | **3.213 m** | **5.69 J** | 1.5 | 0.0 |
| + `--uwbEvidenceFromNeighbours` | 3.425 m | 5.84 J | 2.4 | **5.4** |
| + bidirectional exchange, pseudo-anchor, GPS error | 3.603 m | 5.81 J | 2.6 | 8.8 |

Paired by seed, 20 000-sample percentile bootstrap.

**base → gate on (static map):**

- accuracy **−3.330 m**, 95% CI **[−3.720, −2.966]**, 8/8, p = 0.0039
- energy **−66.340 J**, 95% CI **[−74.475, −57.248]**, 8/8, p = 0.0039

**static map → live-range form (`--uwbEvidenceFromNeighbours`):**

- accuracy **+0.212 m**, 95% CI [+0.025, +0.508] — slightly worse
- energy **+0.156 J**, 95% CI [−0.016, +0.352] — **includes zero**
- UWB cycles **0.0 → 5.4**

## 2. A correction, and it is the reason this section exists

**An earlier version of this report attributed the 66 J to
`--uwbEvidenceFromNeighbours`. That was wrong.** The campaign compared *neither flag*
against *both flags* and could not separate them. Checking the third arm shows the
entire saving belongs to `--escalationRequiresEvidence`, which was already in the tree.

Worse, the first implementation of `--uwbEvidenceFromNeighbours` **could not have had
any effect at all**: it read `tierEvidenceMask`'s UWB bit, and a tier not in use is put
in deep sleep with its neighbour table cleared every cycle, so that bit can only be set
while already on UWB. A gate demanding UWB evidence before entering UWB is a permanent
veto. It produced output byte-identical to the static map on all 8 seeds — two
different permanent refusals agreeing exactly, which reads as agreement and is not.
Fixed in `c236834`; the flag now predicts from measured range on the radio that is
actually awake.

This is rule 5 — check every arm before claiming uniqueness — and it cost a withdrawn
claim once before. The corrected contribution of the flag is narrow and worth stating
plainly: **it makes tier 3 reachable again (0 → 5.4 cycles) at +0.212 m and no
measurable energy cost.** It does not deliver the energy saving.

## 3. What the 66 J actually is, and why it is not a victory

**UWB cycles go from 176.5 to 0.0 the moment the gate is switched on at all.** The gate
does not make tier 3 cheaper. It stops the node using tier 3.

Under the corrected topology the node was escalating to UWB for 176 of 240 cycles and
getting **zero UWB fixes**, against mean UWB neighbours of 0.01–0.22. It was paying
tier-3 rates to sit in an empty tier, and that waste was **92% of the energy budget**.
Accuracy improved for the same reason: the node stays on 2.4 GHz, where it can range,
instead of coasting on a tier that answers nothing.

So the finding is *the escalation policy was spending almost all of its energy on a tier
that was not there.* It does **not** say tiering works, and it does not say the
confidence score works. The live-range form recovers 5.4 UWB cycles of the 176.5 — the
ones where something really is in range — which is the shape the architecture wants, at
a hundredth of the energy.

**Making tier 3 properly reachable — finders that converge close enough, for long
enough — remains the open question this exposes, not one it answers.**

## 4. The exchange half: mechanism established, effect not claimed

Pre-registration `78edc67` predicted (P4) that accuracy would get **worse** once finders
stopped being handed to the filter as perfectly-known points. Measured, gate → full:
**+0.177 m, 95% CI [+0.006, +0.351]**, improved in 2 of 8 seeds. Energy **−0.034 J**,
CI [−0.270, +0.211], **including zero**.

**The prediction holds, and the direction is the point.** Replacing a finder's true
position with the position it reported — stale by the dissemination latency, inflated to
match — is strictly less information. A tenth of a metre is what removing the flattery
costs. It was registered in advance precisely so this could not later be read as a
failed change.

Mechanism, seed 7: casualty sends 20 digests, holds **3.3 of 4** finder entries per cycle
at 79 s mean age (from 0 before), **1519 pseudo-anchor substitutions** (from 0), relayed
variance share **0.20 over 180 cycles** (from 0.00).

## 5. Three defects this surfaced, each found by measurement rather than reading

1. **Knowledge was tier-scoped.** Each tier owns its own `CaTblMac` and table, so an
   entry learned on 868 was invisible to a lookup made while on UWB. With the digest
   alone the casualty held 0.6 of 4 entries and fired 4 substitutions in 240 cycles.
   Reading the union of its own tables took that to 0.8 and 231. Nothing goes on the air
   to achieve it — it is a read. The one-hop neighbour table is deliberately **not**
   unioned; it drives the poll set and must stay per-radio.

2. **A stale anchor carried a fresh anchor's variance.** The pseudo-anchor substitution
   inflated by the reporter's uncertainty *at the time it reported* and never by the
   entry's age. 238 substitutions on entries averaging 242 s old drove rmse from 7.44 m
   to **86.71 m**. Inflating by `(walkSpeed × age)²` restored it to 7.09 m with the
   mechanism firing as often. Omitting age is not a conservative simplification; it
   understates uncertainty, which is the dangerous direction.

3. **The relayed-variance accumulator was cleared between its two contributors.** The
   reset sat after `rangeTier()` and before the C5 block — correct while C5 was the only
   source, silently wrong once the pseudo-anchor became a second one upstream. 1513
   substitutions produced a share of 0.00 and byte-identical output at
   `--hopProvenanceForm` 0 and 1. **A mechanism that cannot change its own output is not
   enabled, whatever its flag says.**

4. **The evidence gate asked a sleeping radio.** Covered in §2. Found by the same test
   defect 3 taught — asking whether a flag can change its own output — and the answer
   was no, on 8 seeds, silently.

Defects 3 and 4 are the ones worth keeping. Two counters each looked right and disagreed about
the same cycle: the substitution counter said the mechanism was firing, and the term it
fed was reading a zero. Nothing in the gate suite tests for that.

## 6. Pre-registered abort conditions

- **Null control.** Every change is byte-identical at its default. `40e3258`: 7/7 against
  `baseline_v8`. `5262c59`: 7/7 against `baseline_v8` **and** 2/2 with finders present,
  because `baseline_v8` runs `nFinders=0` and cannot see finder code at all. `cf25341`,
  `f88e224`, `edf73a5`: 2/2 with finders present; `edf73a5` additionally 2/2 on the
  pre-existing `--relayedRanging` path it could have disturbed. `c236834`: 7/7 against
  `baseline_v8`.
- **Duty cycle.** `duty_868` is 0.02260 in every config against the G.S.R. 853(E)
  Table-II field cap of 0.025. The casualty's new DIGEST frames do not breach it. Not
  triggered.

## 7. What this does not establish

- **Nothing about tier 3 being worth having.** The gate removes tier-3 use; it does not
  make tier 3 work. See §2.
- **Nothing about whether provenance carries information the covariance does not.** That
  is the separate PCS campaign in `docs/TECH_DEBT.md` §3.2. A2 still caps hop depth at 1,
  so `hopCount` is binary. This establishes only that the term now has an input.
- **One configuration.** 600 s, 1 km box, four finders, one mobility model, one digest
  period. Nothing is claimed about density, area, finder count or casualty motion.
- **Stream displacement is not undone by pairing.** Every config differs in transmission
  count, and the sampler draws from the shared default stream
  (`docs/TECH_DEBT.md` §1.1). Pairing controls the trajectory, not the stream.
- **Dissemination latency is unmanaged.** Finders act on a position 79–240 s old and
  nothing bounds it. Staleness is now priced into the *consumer's* variance; it is not
  bounded at the source.
