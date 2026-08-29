# Root-cause analysis — the envelope setbacks

Phase A. **Diagnosis only; no simulator or policy code was changed.** No new
simulation runs: every number below comes from `results/p10_env` (140 directories,
6 arms, 1440 cycles each) already on disk, plus the per-directory `arm_summary.csv`.

**PROVISIONAL.** Source `a059203 dirty 3`; every directory carries `ALLOW_DIRTY`.
See `SESSION_CHANGELOG.md`.

Verdict vocabulary, used strictly:

- **DEFECT** — the code does something other than what it was meant to do. Fixing it
  changes a number that was wrong.
- **ARCHITECTURE** — the code does what it was designed to do, and the cost is a
  property of the design. It stays in the report.
- **SCENARIO** — a property of this deployment or these parameters, not of the
  architecture. It moves if the scenario moves.

---

## Summary

| # | question | verdict |
|---|---|---|
| Q1 | Where do `ca_tbl_full`'s 28.15 J go at 0.78? | **ARCHITECTURE** — the tier-1 floor is 54.5% of arm energy |
| Q2 | Was the decision layer tested in a working state? | **DEFECT** — no. Legacy NLOS and legacy PCS both live at every density |
| Q3 | What fails above 1.55? | **SCENARIO** — coverage-shaped; 96–99.6% of bad fixes have `vis2p4 < 3` |
| Q4 | `static_2p4` energy non-monotonic | **SCENARIO** — search charge, not `rxTimeout`. Energy model is sound |
| Q5 | Does the integrity gate hurt at 1.55/2.07? | **NEITHER — hypothesis falsified.** Differences are noise |
| Q6 | What is the lower edge? | **SCENARIO** — tier-2 coverage is 96.8% at 0.78 |

**The FAIL at 0.78 survives and is understood.** It is not a policy error.

---

## Q1 — Where do `ca_tbl_full`'s 28.15 J go at `spacing/r80 = 0.78`?

### Evidence

Population: 20 seeds × 1440 cycles, per-seed mean of `arm_summary.csv` columns.

| arm | totalJ | j_868 | j_2p4 | j_uwb | cyc_868 | cyc_2p4 | cyc_uwb |
|---|---|---|---|---|---|---|---|
| `static_868` | 15.39 | 15.37 | 0.01 | 0.01 | 1440.0 | 0.0 | 0.0 |
| `static_2p4` | 13.37 | 0.01 | 13.35 | 0.01 | 0.0 | 1440.0 | 0.0 |
| `fixed_threshold` | 24.85 | 15.12 | 9.73 | 0.01 | 359.4 | 1080.7 | 0.0 |
| `ca_tbl_no_integrity` | 28.18 | 15.36 | 12.53 | 0.29 | 2.5 | 1336.2 | 101.3 |
| `ca_tbl_full` | **28.15** | **15.35** | 12.52 | 0.28 | **1.9** | 1339.3 | 98.8 |

`ca_tbl_full`'s `j_868` is **15.35 J against `static_868`'s 15.37 J — a 0.1%
difference** — while tier 1 is the *active* tier on **1.9 of 1440 cycles (0.13%)**.

Per-cycle, over 28800 cycles:

| quantity | value |
|---|---|
| cycles where `runFloor` is true (tier 1 transmits) | 28761 (**99.9%**) |
| cycles where the fix came from the floor (`usedFloor`) | 334 (**1.16%**) |

Hypothesis tests, all three pre-stated:

- **H1a (UWB occupancy) — FALSIFIED.** `j_uwb` = 0.28 J = **1.0%** of arm energy.
- **H1b (thrashing) — FALSIFIED.** `ca_tbl_full` escalates **0.0026/cycle** (76 in
  28800), median tier run-length **56 cycles**. For contrast `fixed_threshold` *does*
  thrash — 3585 escalations, median run-length **3** — and costs **less** (24.85 J).
- **H1c (tier-1 floor) — CONFIRMED.**

Arithmetic decomposition of the 2.105× (measured columns, **not** a simulated
counterfactual):

| component | J | share |
|---|---|---|
| tier-1 floor | 15.35 | **54.5%** |
| active tier 2 | 12.52 | 44.5% |
| UWB | 0.28 | 1.0% |
| **total** | **28.15** | 2.105× `static_2p4` |
| tier-2 + UWB alone | 12.80 | **0.957× `static_2p4`** |

### Mechanism

`scratch/ca_tbl_causal_sim.cc:2266`

```cpp
const bool runFloor = hybridArm && (active != CA_TBL_TIER_868);
```

applied at `:2468` and `:2484`, which call `rangeTier(CA_TBL_TIER_868)` on every
cycle the active tier is not already tier 1. The design intent is documented
in-place at `:2249-2262`: without the floor a hybrid arm was dead-reckoning 74% of
steps, and "a hybrid arm cannot fix less often than the pure tier-1 arm if tier 1 is
always on." The static arms deliberately do **not** get it (`:2259-2262`) — "the
point of the comparison is that they have one radio."

This also explains `fixed_threshold`'s *lower* energy: it is active on tier 1 for
25.0% of cycles, and on those cycles `runFloor` is false, so only one radio
transmits.

### Verdict — ARCHITECTURE

The tiered arm runs **two radios every cycle** where `static_2p4` runs one. The
2.105× is the cost of *having* a floor, not of using it badly: the policy barely
escalates, does not thrash, and spends 1.0% of its energy on UWB.

The honest statement of the FAIL therefore changes. It is not "the policy wastes
energy at low density". It is **"at `spacing/r80` = 0.78 the tier-1 floor is
insurance nobody needs, because tier 2 alone already meets the requirement 20/20."**
The floor supplies the fix on 1.16% of cycles for 54.5% of the energy.

**This is a real result and belongs in the report.** It is also the sharpest
possible statement of the lower edge: below the window, do not carry the floor.

*Not resolvable from existing data:* whether a floor that is *duty-cycled* rather
than continuous would preserve the availability benefit at higher densities. That
needs a run and is a Phase B question.

---

## Q2 — Was the decision layer tested in a working state?

### Evidence

Flags read from all 140 sidecars, not from memory. Every run:
`--duration=3600 --escalationRequiresEvidence=true --gwGrid=5 --trace=true`.
**`--pcsForm` and `--nlosSource` were never passed**, so both took their defaults:
`nlosSourceArg = "legacy"` (`ca_tbl_causal_sim.cc:3041`) and
`pcsFormArg = "legacy"` (`:3046`).

Population: fix cycles only (`hadFix == 1`), 20 seeds pooled per density. NLOS
fractions over fused links on those cycles. Arm `ca_tbl_full`.

| box | sp/r80 | fix cyc | NLOS operative | NLOS propagation | PCS median | PCS below thr | ρ(PCS,err) | ρ(traceSigma,err) | esc/fix |
|---|---|---|---|---|---|---|---|---|---|
| 1500 | 0.78 | 28793 | **74.9%** | 0.3% | 1.543 | 67.3% | −0.174 | +0.235 | 0.0026 |
| 2000 | 1.03 | 28718 | 75.3% | 1.3% | 1.302 | 84.4% | −0.275 | +0.363 | 0.0048 |
| 2500 | 1.29 | 28267 | 76.0% | 4.9% | 1.150 | 93.4% | −0.153 | +0.538 | 0.0220 |
| 3000 | 1.55 | 27279 | 76.7% | 7.9% | 1.127 | 96.5% | **+0.066** | +0.746 | 0.0587 |
| 4000 | 2.07 | 26112 | 77.4% | 9.6% | 1.124 | 97.1% | **+0.187** | +0.871 | 0.0918 |
| 5000 | 2.58 | 24079 | 77.5% | 10.6% | 1.098 | 96.8% | **+0.114** | +0.859 | 0.1167 |
| 6000 | 3.10 | 20396 | 78.1% | 12.8% | 1.024 | 97.7% | **+0.160** | +0.801 | 0.1590 |

Three findings, each independently sufficient:

1. **The NLOS defect is live at every density.** The operative tag fires on
   74.9–78.1% of fused links; the propagation-layer determination on 0.3–12.8%. At
   0.78 the ratio is **250×**. `nlosSigmaInflation` (3.0×) is therefore applied as
   an effectively global variance multiplier, exactly as previously diagnosed.

2. **Legacy PCS is degenerate at every density, and worsens with sparsity.** Median
   PCS falls 1.543 → 1.024 against thresholds of 1.55/1.70/2.30, so the fraction of
   fix cycles that "want to escalate" rises **67.3% → 97.7%**. Above 1.29 the
   trigger is close to unconditional.

3. **PCS carries no information, and above 1.29 it carries wrong-signed
   information.** ρ(PCS, err) is −0.17 at best and turns **positive** at 1.55 and
   above — higher reported confidence associated with *higher* realised error. The
   control, ρ(traceSigma, err), is positive and strong at every density, rising to
   **+0.871**.

### Mechanism

`src/lora-physics-engine/model/ca-tbl-policy.cc:49` sets
`pcsForm = CA_TBL_PCS_LEGACY` as the default; the legacy branch of `ComputePcs()`
computes `traceRef/(traceRef+traceSigma) + 1/max(gdop,1) + nLos/nTotal`, whose term
1 saturates, term 2 is inverted across tiers and term 3 is pinned by the NLOS tag.
`ca_tbl_causal_sim.cc:3234` defaults `nlosSource` to `CA_TBL_NLOS_LEGACY`, which
builds the operative tag as `(nlosDb > 0) || (hiddenChannelState > 0)` — and
`hiddenChannelState` is a four-state Markov *fading regime*, not an obstruction
indicator.

### Verdict — DEFECT (known, previously diagnosed, never switched on)

**The decision layer was not tested in a working state at any density.** The
comparison `ca_tbl_full` ≈ `fixed_threshold` is therefore not a test of the
confidence idea. It is a test of a known-degenerate score against a variance
threshold — and `traceSigma`, which is exactly what `fixed_threshold` thresholds,
predicts realised error at ρ up to **+0.871** while PCS manages **+0.187 of the
wrong sign**.

The honest statement of the null result must change accordingly. Handoff §1.3's
Claim 2 (`ca_tbl_full` > `fixed_threshold`, 20/20 seeds at 1.03) is unaffected as a
*measurement*, but its interpretation cannot be "the confidence model helps" — the
confidence model was not functioning.

`report/PCS_REDESIGN_RESULTS.md` Part 0 pre-registers PCS v2 and it has never been
run. Whether to run it is Phase B.

---

## Q3 — What fails above `spacing/r80 = 1.55`?

### Evidence

Population: ALL cycles, 20 seeds × 1440 = 28800 per density. Arm `ca_tbl_full`.

| box | sp/r80 | no fix | err ≤ 25 | err > 25 | bad fixes by source tier |
|---|---|---|---|---|---|
| 1500 | 0.78 | 0.0% | 99.9% | 0.0% | 868 67%, UWB 33% (n=12) |
| 2000 | 1.03 | 0.3% | 99.2% | 0.5% | 868 94%, 2.4 6% |
| 2500 | 1.29 | 1.9% | 93.5% | 4.6% | 868 94%, 2.4 6% |
| 3000 | 1.55 | 5.3% | 76.7% | **18.1%** | **868 96%**, 2.4 4% |
| 4000 | 2.07 | 9.3% | 54.2% | 36.5% | 868 98%, 2.4 2% |
| 5000 | 2.58 | 16.4% | 41.3% | 42.3% | 868 99%, 2.4 1% |
| 6000 | 3.10 | 29.2% | 25.6% | 45.2% | 868 99%, 2.4 1% |

Of the bad-fix bucket (fix present, `errM` > 25), was tier 2 geometrically
available? Denominator is the bad-fix bucket only:

| box | sp/r80 | bad fixes | `vis2p4 ≥ 3` | `vis2p4 < 3` |
|---|---|---|---|---|
| 2000 | 1.03 | 158 | 6 (3.8%) | **152 (96.2%)** |
| 2500 | 1.29 | 1336 | 41 (3.1%) | **1295 (96.9%)** |
| 3000 | 1.55 | 5200 | 96 (1.8%) | **5104 (98.2%)** |
| 4000 | 2.07 | 10505 | 57 (0.5%) | **10448 (99.5%)** |
| 5000 | 2.58 | 12196 | 54 (0.4%) | **12142 (99.6%)** |
| 6000 | 3.10 | 13013 | 68 (0.5%) | **12945 (99.5%)** |

The confirmation is one-sided and overwhelming: **Q3-A confirmed, Q3-B falsified.**
The failure is dominated by inaccurate fixes, not missing ones (at 1.55, 18.1% bad
against 5.3% no-fix), and those inaccurate fixes are tier-1-sourced while tier 2 was
geometrically unavailable.

### Mechanism

Not a code defect. `BuildDeployment()` places tier-2 anchors as four 150 m triads at
four facility sites; `t2TriadRadiusM` is **absolute** (handoff §4.5), so as `boxHalf`
grows the sites separate while each cluster stays the same size. The covered
*fraction* of the box therefore falls — measured in Q6 — and the node spends more
time where only tier 1 reaches. Tier 1 cannot meet R = 25 m: its `cep50FixedM` is
77–307 m across these densities.

### Verdict — SCENARIO (siting)

The upper edge is a **coverage limit created by clustering tier-2 anchors at four
facility sites**. Complementary siting (§2.3) attacks exactly this quantity and is
now motivated by data rather than speculation.

This also confirms the pre-registered prediction's step 3 *and* its caveat: the
upper edge is a property of co-located siting, not of tiering, and the report must
say so.

---

## Q4 — Why is `static_2p4`'s energy non-monotonic in spacing?

### Evidence

Population: 20 seeds, per-seed mean. Units J.

| box | sp/r80 | j_2p4 | **tx_2p4** | **rx_2p4** | cad | idle |
|---|---|---|---|---|---|---|
| 1500 | 0.78 | 13.347 | 8.590 | 4.719 | 0.013 | 0.025 |
| 2000 | 1.03 | 9.654 | 6.222 | 3.397 | 0.013 | 0.022 |
| 2500 | 1.29 | 7.353 | 4.602 | **2.719** | 0.013 | 0.019 |
| 3000 | 1.55 | 6.635 | 3.372 | 3.232 | 0.013 | 0.018 |
| 4000 | 2.07 | 10.833 | 2.336 | 8.468 | 0.013 | 0.016 |
| 5000 | 2.58 | 21.206 | 1.772 | 19.406 | 0.013 | 0.014 |
| 6000 | 3.10 | 32.208 | **1.275** | **30.909** | 0.013 | 0.012 |

`tx_2p4` falls **monotonically** 8.590 → 1.275 J (fewer anchors addressed). The
entire non-monotonicity is in `rx_2p4`, which bottoms at 2.719 J and rises 11× to
30.909 J.

**The proposed mechanism was wrong.** It is not `rxTimeout` on unanswered polls:
`rangeTier()` returns early when the neighbour table is empty (`:2136-2151`,
"nothing discovered on this tier this cycle: no poll is issued and no RX timeouts
are burnt"), so an out-of-coverage radio issues no poll at all.

The actual charge is the **search window**:

```
search quantum = min(dt, beacon) x searchDuty x I_rx x V
               = min(2.5, 2.0) x 1.0 x 0.007 A x 3.3 V = 0.0462 J/cycle
```

| box | sp/r80 | rx_2p4 | rx/cycle | implied f_search | `vis2p4 ≥ 3` |
|---|---|---|---|---|---|
| 1500 | 0.78 | 4.719 | 3.28 mJ | 7.1% | 96.8% |
| 2500 | 1.29 | 2.719 | 1.89 mJ | 4.1% | 50.2% |
| 4000 | 2.07 | 8.468 | 5.88 mJ | 12.7% | 37.8% |
| 6000 | 3.10 | 30.909 | 21.46 mJ | **46.5%** | 20.5% |

`f_search` stays below `(1 − vis2p4≥3)` at every density, as it must — the
neighbour TTL is `nbrTtlMult(3) × beacon(2.0 s) = 6 s`, so a node briefly out of
range retains neighbours and does not search.

`rx_2p4` is therefore two components moving oppositely: **beacon-reception
listening**, which falls with density (fewer anchors in range → fewer beacons
delivered → less `AccountListen`), and the **search charge**, which rises. They
cross at `boxHalf = 2500`.

### Mechanism

`scratch/ca_tbl_causal_sim.cc:2711-2718`

```cpp
if (nodeMac[tierIdx]->GetNeighbourCount(tr, t) == 0) {
    const double window = std::min(dt, beaconPeriodS[tierIdx]) * searchDutyFrac;
    if (window > 0.0) { energy.AccountListen(tr, window); }
}
```

charged at `ca-tbl-energy-model.cc:489` at `rxCurrentMa`.

### Verdict — SCENARIO (a documented modelling assumption), energy model SOUND

The reconciliation closes, so **the energy model has no defect and the project's
energy claims are not in question** — which was the reason this apparently minor
question was on the list.

**But one caveat must travel with the envelope's energy axis.** `searchDutyFrac`
defaults to 1.0, and the code at `:2705-2710` says explicitly: *"searchDutyFrac
exists so the bracket is reportable rather than asserted: 0.0 reproduces the
zero-charge run exactly, 1.0 is the consistent model... Sweep it and publish the
curve; do not quote a single number."*

At `spacing/r80 ≥ 2.07`, **46.5% of `static_2p4`'s energy at 3.10 is this single
assumption.** The envelope quotes single energy numbers there. Those numbers are
consistent and defensible, but they are not assumption-free, and the report must
carry the bracket at sparse densities. Below ~1.55 the effect is small (≤ 5% of
`j_2p4`) and the numbers are robust.

---

## Q5 — Why does `ca_tbl_no_integrity` beat `ca_tbl_full` at 1.55 and 2.07?

### Evidence

Population: ALL cycles, 20 seeds × 1440 = 28800 per density per arm.

| box | sp/r80 | suspects | susp/cycle | escalations forced by suspect | PCS-driven escalations |
|---|---|---|---|---|---|
| 1500 | 0.78 | 11 | 0.00038 | 11 | 65 |
| 3000 | 1.55 | 9 | 0.00031 | **5** | 1597 |
| 4000 | 2.07 | 19 | 0.00066 | **4** | 2393 |
| 6000 | 3.10 | 41 | 0.00142 | 9 | 3234 |

The gate fires on **0.03–0.14% of cycles** and forces escalation **2–11 times per
28800 cycles**. The PCS-driven escalations outnumber it by two to three orders of
magnitude.

Per-seed paired sign test, 20 paired seeds per density:

| box | sp/r80 | pos | neg | tie | mean Δ | p |
|---|---|---|---|---|---|---|
| 1500 | 0.78 | 8 | 0 | 12 | +0.00045 | **0.0078** |
| 2000 | 1.03 | 4 | 1 | 15 | +0.00132 | 0.3750 |
| 2500 | 1.29 | 2 | 4 | 14 | +0.00285 | 0.6875 |
| 3000 | 1.55 | 2 | 3 | 15 | −0.00007 | **1.0000** |
| 4000 | 2.07 | 2 | 5 | 13 | −0.00378 | **0.4531** |
| 5000 | 2.58 | 3 | 3 | 14 | −0.00094 | 1.0000 |
| 6000 | 3.10 | 7 | 8 | 5 | +0.00056 | 1.0000 |

### Verdict — HYPOTHESIS FALSIFIED; the differences are noise

At 1.55 and 2.07 the paired tests give **p = 1.0000 and p = 0.4531**. The arms are
*identical* on 13–15 of 20 seeds at every density — they diverge only when the gate
happens to fire. The only significant result is at **0.78, p = 0.0078, and it
favours `ca_tbl_full`** — the opposite direction to the hypothesis.

The proposed mechanism (`suspect` forcing futile escalation at sparse density) is
arithmetically incapable of producing the observed deltas: 4 forced escalations
cannot account for a 0.0038 availability difference over 28800 cycles except through
trajectory divergence, and the sign is not systematic across densities.

This is consistent with the N3 ablation (4 positive, 1 negative, 15 tied) and
**reinforces the standing instruction not to report the integrity ablation as a
finding.** It is still unmeasured, not measured-as-null: the gate needs S7 fault
injection to be exercised at all.

`--suspectForcesEscalation=false` exists and was **not** run, per instruction.

---

## Q6 — What does the lower edge consist of?

### Evidence

Population: ALL cycles, 20 seeds × 1440 = 28800 per density.

| box | sp/r80 | `vis2p4 ≥ 3` | good fixes by source tier |
|---|---|---|---|
| 1500 | 0.78 | **96.8%** | 2.4 GHz 93.0%, UWB 5.8%, 868 1.2% |
| 2000 | 1.03 | 71.7% | 2.4 GHz 90.3%, 868 5.8%, UWB 4.0% |
| 2500 | 1.29 | 50.2% | 2.4 GHz 75.6%, 868 22.1%, UWB 2.3% |
| 3000 | 1.55 | 40.9% | 2.4 GHz 65.5%, 868 33.0%, UWB 1.4% |
| 6000 | 3.10 | 20.5% | 2.4 GHz 76.0%, 868 20.5%, UWB 3.5% |

### Verdict — SCENARIO

The lower edge is **tier-2 geometric coverage being effectively continuous**:
`vis2p4 ≥ 3` on 96.8% of cycles at `spacing/r80` = 0.78, and 93.0% of good fixes
come from tier 2. Stated as a coverage fraction rather than inferred from pass
counts, as required.

Two observations worth recording:

- `static_2p4` achieves 99.87% availability against 96.8% geometric coverage, so the
  EKF coasts through the 3.2% of cycles with `vis2p4 < 3`. Availability slightly
  exceeds instantaneous geometry, which is the filter doing its job.
- Tier 2 remains the dominant source of *good* fixes at every density (65–93%), even
  at 3.10 where it covers only 20.5% of cycles. What collapses at the upper edge is
  not tier 2's quality but its **availability**.

---

## What could not be resolved from existing data

| question | what would resolve it | not run |
|---|---|---|
| Would a duty-cycled tier-1 floor preserve the availability benefit while removing the 54.5% cost? | A run with the floor gated on a cadence or on `traceSigma`, at 0.78 and 1.03 | Phase B |
| Does PCS v2 recover any decision information? | The pre-registered v2 form, all seven densities, ρ vs the `traceSigma` control | Phase B |
| Does complementary siting move the upper edge? | Second treatment arm alongside co-located, ≥ 1.55 | Phase B |
| Is the integrity gate ever beneficial? | S7 fault injection — it is unexercised, not null | not this session |
| How much of the sparse-density energy is the `searchDuty` assumption? | `--searchDuty` sweep {0.0, 0.5, 1.0} at 2.07–3.10 | Phase B |
