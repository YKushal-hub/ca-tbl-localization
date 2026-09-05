# Deriving the accuracy requirement from the operation — verification and registration

Written and committed **before any re-adjudication**. The availability denominator is the most
abusable quantity available here, and the only protection is that the derivation is written, sourced
and committed first.

## 1. The figures, verified against primary sources — three of four do not survive

I was given numbers to look for, not numbers to use. Checked:

| figure | as supplied | primary-source status |
|---|---|---|
| pinpoint probe grid **25 cm** | ICAR Rec. 009 | **VERIFIED.** Rec. 009 states: *"Continue probing in a 25x25 cm expanding grid from the center point."* This is the **only** numeric value in Rec. 009 |
| coarse search ends **3–5 m** | ICAR Rec. 009 | **NOT IN Rec. 009.** The document defines the phases and gives no transition distances. The ~3 m figure is real but comes from **secondary** sources — avalanche.org and Avalanche Canada both state fine search begins at about 3 m |
| standardised **25 m** lateral test offset | ICAR | **NOT FOUND.** No ICAR document I could read states it. The DAV transceiver-test PDF hosted by ICAR could not be text-extracted (font-subset encoding). **Excluded** |
| search strip width **20–40 m** | — | **NOT A FIXED ICAR VALUE.** Rec. L 0007 (10 Oct 2008) specifies a *method* for manufacturers to determine strip width — receiver setup, ≥ 6 dB SNR — and leaves the number device-specific. It is a device property, not a requirement |

**So exactly one operational accuracy figure is primary-source verified: 0.25 m for pinpointing.**
The ~3 m fine-search onset is used and labelled **secondary**. The 25 m offset is excluded.

Sources: [ICAR Rec. 009](https://www.alpine-rescue.org/articles/1681--phases-in-an-avalanche-search-recommendation-009-2008-updated-2025) ·
[ICAR Rec. L 0007](https://www.alpine-rescue.org/system/production/article/documents/file/000/405/69232b7195bc9ab8cc223f9d53ed61b3dcac6736d555e0a26ff0edb081635f07/20081010-AVA-REC0007%20Search%20Strip%20Width%20to%20be%20Used%20with%20Transceivers.pdf) ·
[avalanche.org fine search](https://avalanche.org/avalanche-encyclopedia/human/companion-rescue/transceiver-search/fine-search/) ·
[Avalanche Canada coarse search](https://avysavvy.avalanche.ca/en-ca/coarse-search)

## 2. The per-phase triple **cannot be completed**, and the reason is structural

The requested deliverable was a triple per phase: accuracy `R`, availability `A`, and **population** —
which cycles count.

**`R` is derivable. `A` and the population are not, and the obstacle is not the literature.**

**The simulation has no phase structure.** There is no signal-search, coarse-search, fine-search or
pinpointing state in `ca_tbl_causal_sim.cc`; the node walks and dwells, and every cycle is the same
kind of cycle. There is no column, and no derivable label, that says which phase a cycle belongs to.
So "the fraction of *pinpointing* cycles with a fix" is not a quantity this corpus can produce
without **inventing** the partition — and inventing an availability denominator is precisely the
move this registration exists to prevent.

**Consequence, applied as the guardrail directs:** the per-phase `(A, R, population)` triples are
**excluded**. A two-part result derived honestly beats a three-part one with an invented denominator,
and here even the two-part version would need a partition that does not exist.

**What is retained:** `R` as an adjudication threshold, swept as a ladder, with the population held
at the registered *all 1440 cycles* and `A ≥ 0.95` unchanged. That is the original registered
criterion evaluated at other accuracies — a re-adjudication, not a new criterion.

## 3. The ladder

Rungs, from the verified and secondary figures plus the published point:

    0.25 m   pinpoint probe grid          PRIMARY-VERIFIED (ICAR Rec. 009)
    0.5 m    intermediate
    1.0 m    intermediate
    2.0 m    intermediate
    3.0 m    fine-search onset            SECONDARY (avalanche.org, Avalanche Canada)
    5.0 m    intermediate
    10.0 m   intermediate
    25.0 m   the published requirement    on the same curve, for continuity

Every arm, every rung, computed from the per-cycle errors already on disk in
`results/joint_criterion/` — 52 seeds, `source ec48e90 dirty 0`. **Losing regions reported at the
same resolution as winning ones**, per the OFAT standard.

## 4. Expected distinguishability, stated before running

Of the three outcomes:

- **A band exists** where the tiered arm meets `A ≥ 0.95` and `static_2p4` does not.
- **No band** — `static_2p4` tracks the tiered arm down the ladder.
- **Every arm fails** below some `R` — the precision tier is necessary but not sufficient.

**The third is expected to be distinguishable and the first is not, on this evidence.** Tier 3
supplies 83 UWB cycles per seed out of 1440 — 5.8%. An availability requirement of 0.95 over all
cycles cannot be carried by a tier serving 5.8% of them, whatever its accuracy. So a band in which
the tiered arm passes and the single-band arm fails would require tier 2 to be doing the work, and
tier 2's CEP50 is 2.73 m — which places any such band above 3 m, not below.

**This is a statement of what the data can distinguish, not a prediction of the outcome**, and no
prediction is registered on which outcome holds.

## 5. The limitation, stated here so it is not a footnote later

**The policy does not read `R`.** It escalates on PCS thresholds, so its behaviour is byte-identical
at every rung. The ladder shows **whether the current policy's output would meet a tighter
requirement — not what a policy tuned for that requirement would do.** A tuned policy is a separate
experiment and is not run here.

**Claim 9 is not touched.** A band, if one exists, is a new claim with its own registration.
