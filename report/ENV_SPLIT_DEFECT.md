# The topology was corrected; the propagation environment was not

**2026-09-08.** Found while asking why `static_2p4` dominated every other arm on both
axes. It is not a policy failure. Tiers 2 and 3 are simulated in a **different
environment and a different vegetation class from tier 1**, on nodes that under the
corrected topology are the same physical nodes.

**Status: DEFECT.** Reachable from the CLI, so it needs no code change to test — but
the DEFAULT is wrong, and every result in the tree was produced at that default.

---

## 1. What the code does

`scratch/ca_tbl_causal_sim.cc:1289` builds the three per-tier physics configurations
from two environment/vegetation pairs, not one:

| | tier 1 (868 MHz) | tiers 2 and 3 (2.4 GHz, UWB) |
|---|---|---|
| environment | `ENV_RURAL_FORESTED` (`envWide`) | `ENV_RURAL_OPEN` (`envFacility`) |
| path-loss exponent | 2.3 | 2.2 |
| shadowing σ | 7.0 dB | 5.0 dB |
| NLOS excess μ | 10.0 dB | 6.0 dB |
| vegetation | `VEG_DENSE_FOREST` — γ 0.20, A_max 25 dB | `VEG_SPARSE_SHRUBS` — γ 0.04, A_max 8 dB |

At the modelled median foliage depth of 100 m the ITU-R P.833 term
`A_max·(1 − e^(−depth·γ/A_max))` gives **13.8 dB for tier 1 and 3.2 dB for tiers 2/3** —
a 10.6 dB split, before the shadowing and NLOS differences.

## 2. Why it is there, and why that reason expired

The code says so itself, at `:1271-1274`:

> `envWide/vegWide` — tier 1 spans kilometres of search area and crosses canopy.
> `envFacility/vegFacility` — tiers 2 and 3 range against anchors **AT a facility
> site**.

That was true of the pre-2026-09-07 model, which built **three disjoint anchor
populations**: a wide-area 868 MHz mesh and separate 2.4 GHz and UWB anchors sited at
facilities. Tiers 2 and 3 really were in clearings, because they were different
hardware in different places.

The tri-band correction (`99fdf1a`) removed that. Every gateway now carries all three
radios and so does every finder. **Tier 2 transmits from the same mast, through the
same canopy, as tier 1.** The environment split is the half of the topology correction
that was never made, and nothing failed when it was left behind: the flags still
existed, the gates still passed, and the numbers still looked like numbers.

## 3. What it costs — one seed, and what that is worth

`boxHalf=2000`, seed 7, 3600 s, designed configuration, changing only
`--envFacility=1 --vegFacility=3` so tiers 2/3 join tier 1 in the forest:

| arm | metric | split (default) | unified | |
|---|---|---|---|---|
| `static_2p4` | mean 2.4 GHz neighbours | 10.463 | **4.088** | −61% |
| `static_2p4` | fix cycles of 1440 | 1437 | **863** | −40% |
| `static_2p4` | rmse | **2.28 m** | **271.96 m** | ×119 |
| `static_868` | mean 868 neighbours | 9.711 | 9.731 | +0.2% |
| `static_868` | rmse | 149.53 m | 168.29 m | +13% |

**Tier 1 barely moves; tier 2 collapses.** That asymmetry is the signature — the
change touched only the facility pair, and only tiers 2/3 read it.

**This is one seed and is therefore not a result** (rule 8). It is stated as the reason
for a registered cell, `k1e_split1000`, which measures the same contrast over 20 paired
seeds. What makes it worth acting on before that lands is not the size of the number
but the direction: it is a *configuration* difference, not a stochastic one, and it
runs the same way for every seed by construction.

## 4. Why it invalidates the comparison rather than merely biasing it

At the default, **2.4 GHz out-ranges 868 MHz** — 10.46 mean neighbours against 9.71 on
the same deployment. That is backwards on every term that should decide it:

- 868 MHz has **17 dB** more receiver sensitivity at the configured SF7
  (−123 dBm against −106 dBm);
- and **8.8 dB** less free-space path loss (20·log₁₀(2440/868));
- and 1.5 dB more transmit power (14.0 dBm against 12.5 dBm).

That is ~27 dB of link budget in tier 1's favour, and the split hands it back — mostly
through foliage — until the ordering inverts. **A three-tier architecture whose second
tier out-ranges its first cannot demonstrate anything about tiering**, because there is
no operating point where the long-range tier is the only one that reaches. The domination
question K1 was written to answer was unanswerable at the default, and the answer it
would have produced — `static_2p4` dominates, use one radio — is an artefact of a
vegetation constant.

## 5. Scope of the damage

Every tier-2 and tier-3 number in the repository was produced at this default: the
envelope study, the PCS forms campaign, A4, A5, C1, C3, C4, C5, A6, A1, the p10
surfaces, `baseline_v7`, `baseline_v8`, and the integrity-check result. `TECH_DEBT` I.6
already marks them provisional for the topology; this is a second, independent reason,
and it bites harder because the topology change moved geometry while this one moves the
link budget by tens of decibels.

**The integrity-check result is the one to re-run first** — 8/8 seeds on energy, 7/8 on
accuracy, the strongest finding in the repository — because it is a comparison *within*
tier-2 behaviour and its sign could move.

## 6. What is NOT claimed here

- Not that `ENV_RURAL_FORESTED` is the right environment for a SAR deployment. That is
  a scenario choice and it is arguable. What is not arguable is that **one node cannot
  be in two environments at once**.
- Not that this explains G11 (`TECH_DEBT` J.1). G11 compares the engine against a
  closed-form model that reads the *same* configuration, so both sides carry this split
  and it cancels. The 2.4 GHz margin gap that grows to +11.69 dB at 6 km is a separate,
  still-open question.
- Not that tier 2 is useless. It is more accurate than tier 1 wherever it reaches; the
  correction moves *where* that is, which is the whole point of having tiers.

## 7. Recommended resolution

1. **Run the K1 domination campaign at the unified environment** — done, this is what
   `report/PREREG_K1_DOMINATION.md`'s amendment registers.
2. **Change the default** so `envFacility`/`vegFacility` follow `env`/`veg` unless
   explicitly set, and keep the split reachable for anyone reproducing an old number.
   That is a code change with a null control and it is not made here, because it must
   not land in the middle of a campaign.
3. **Re-run the integrity-check result** at the unified environment before it is cited
   again.
