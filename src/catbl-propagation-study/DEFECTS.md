# Defects found by the propagation study

Simulator defects surfaced while validating the analytic model against the
engines. Each carries a `file:line`, the evidence that it fires, the fix, and the
results it invalidates.

**Numbering note.** An earlier draft numbered these D-1/D-2/D-3 in discovery
order. This file uses the work-order numbering, which is canonical:

| here | earlier draft | subject |
|---|---|---|
| D1 | D-2 | SF7 reads the SF5 cell (2.4 GHz, UWB) |
| D2 | D-1 | `mean_snr_db` accumulated only over delivered packets |
| D3 | (sub-item of D-1) | `0.0` sentinel collides with the data domain |
| D4 | D-3 | `PassesReceiverSensitivity` is a logistic, not a hard threshold |

---

## D1 — SF7 read the SF5 cell on the 2.4 GHz and UWB sensitivity tables

**Site.** `lora-2p4-physics-engine.cc:147-151`, `lora-uwb-physics-engine.cc:147-151`.

```cpp
int sfIndex = std::min(5, std::max(0, static_cast<int>(sf) - 7));
```

`sf - 7` is correct for the 868 table, which declares
`double sensitivityDbm[6];  // index 0=SF7, 5=SF12` (`lora-physics-parameters.h:467`).
Both other tiers declare an **eight**-entry table based at SF5
(`lora-2p4-physics-parameters.h:15`, `lora-uwb-physics-parameters.h:15`). The
accessor was copied without re-basing, so SF7 resolved to index 0 and indices 6
and 7 were unreachable.

**Fix direction — A (accessor wrong, tables right), decided on two independent
grounds.**

1. *Arity, decisive on its own.* An eight-entry SF table based at SF7 would span
   SF7–SF14. SF stops at 12. An eight-entry LoRa SF table can only be SF5–SF12.
2. *Datasheet.* Semtech SX1280 Rev 2.2/3.0 puts SF7 near −106 dBm; −100 dBm sits
   at index 0. Row 2 (−106.0, labelled SF7) is the datasheet-plausible SF7 value.

Coding rate cannot be the culprit: `codingRateDenom = 5`
(`lora-physics-parameters.cc:425`) selects `crIndex = 0`, and every offset in all
three tables is `<= 0`, so no CR can reduce a positive discrepancy.

**Fix.** `lora-2p4-physics-engine.cc:155`, `lora-uwb-physics-engine.cc:155`:

```cpp
int sfIndex = std::min(7, std::max(0, static_cast<int>(sf) - 5));
```

Applied outright, not behind a flag: a wrong table index is not behaviour worth
preserving. 868 MHz is untouched.

**Effect.** 2.4 GHz sensitivity at SF7 moves −100.0 → −106.0 dBm. UWB's table is
flat at −104.0 across all eight entries, so its value is unchanged and only the
index arithmetic is corrected.

**Null control — PASSED.** Probe re-run at identical flags, pre- and post-fix,
same tree:

| tier | result |
|---|---|
| 868 MHz | **byte-identical** (`97aed05f73ac0463`) |
| UWB | **byte-identical** (`4996a73a5e42dfe2`) |
| 2.4 GHz | moved, as intended |

The two index-insensitive tiers confirm the edit reached exactly as far as
intended and no further *within the probe*, where each tier is run in its own
loop with the RNG stream reset.

**Behavioural delta — the 6 dB landed exactly.** `mean_margin_db` shifts by
`+6.00 dB` at both ends of the sweep (mean `+6.114`, spread `0.470` dB, the
mid-range excess being second-order shadowing).

**Registered prediction MISSED, and the miss is explained.** The work order
registered tier-2 r80 at 1500–1700 m. Measured:

```
tier-2 r80   909.5 m -> 1149.5 m   (1.26x, predicted 1.75x)
```

The prediction applied the **path-loss** exponent (`n = 2.476`, 24.76 dB/decade).
r80 is set by the **link-budget** slope, which includes the excess loss that
accumulates with distance (NLOS fraction, foliage, terrain, atmospheric —
`lora-physics-engine.cc:598-604`). Measured local slope at r80 is
**58.2 dB/decade**, giving `10^(6/58.2) = 1.27x` against a measured `1.26x`. The
criterion is not relaxed: the prediction was wrong, for a reason that is itself
measurable, and the same excess-loss mechanism explains the collapse-test result.

**No arm is isolated from this fix — including tier-1-only arms.** `static_868`
moved (`cep50M 102.0031 -> 91.9184`) **when run as the only arm**, so this is not
cross-arm contamination. All three engines are constructed, stepped for mobility
continuity (`ca_tbl_causal_sim.cc:2285-2290`) and beaconed on every cycle of every
arm (`ca_tbl_causal_sim.cc:2137-2144`, `ca_tbl_causal_sim.cc:2175-2179`), and they share an RNG stream. Changing how many
tier-2 beacons are received changes how many draws are consumed, which offsets the
stream every later consumer reads.

**Consequence for interpretation.** The 868 engine is byte-identical and its probe
output is byte-identical, so tier-1 physics did not change. A single-seed shift on
a tier-1-only arm is therefore **a different random realisation, not a physical
effect** (`cycles_868` 1440 → 1440 unchanged, `totalJ` +0.4%, while `cep50M` moves
9.9%). Only multi-seed aggregates carry meaning on those arms.

**Measured deltas, D1 isolated** (same tree, same flags, pre- vs post-fix binary;
verified equal to the comparison against the Aug-10 `a059203` sidecars, so nothing
else in the working tree affects these cells):

`p10_plane_fix seed 1` — note `p10_env/b2000_seed1` carries an **identical**
invocation apart from `--outdir`, so one run covers both cells:

| arm | cep50M | rmseM | totalJ | cycles_uwb | accuracyPerJoule |
|---|---|---|---|---|---|
| ca_tbl_full | 2.5473 → 2.4574 (−3.5%) | 4.3269 → 3.2152 (−25.7%) | 24.881 → 28.162 (+13.2%) | 86 → 56 (−34.9%) | +18.9% |
| fixed_threshold | 2.9543 → 2.8071 (−5.0%) | −2.5% | +9.4% | 0 → 0 | −6.2% |
| static_2p4 | 2.7354 → 2.3410 (−14.4%) | −22.7% | 10.419 → 13.556 (+30.1%) | 0 → 0 | −0.5% |
| static_868 | 102.00 → 91.92 (−9.9%) | −13.9% | +0.4% | 0 → 0 | +15.6% |
| static_uwb | 552.12 → 168.46 (−69.5%) | −16.3% | −0.5% | 1440 → 1440 | +20.0% |

`campaign_recovery none_F1 seed 1`:

| arm | cep50M | totalJ | suspects | trig_tp | trig_recall | trig_f1 |
|---|---|---|---|---|---|---|
| ca_tbl_full | 3.0944 → 2.5629 (−17.2%) | +13.1% | 97 → 86 | 29 → 10 | 0.302 → 0.104 | 0.464 → 0.189 |
| ca_tbl_no_integrity | 3.0273 → 2.4453 (−19.2%) | +13.2% | 0 → 0 | 25 → 5 | 0.260 → 0.052 | 0.413 → 0.099 |

`trig_precision` stays exactly 1.0 and `trig_fp` exactly 0 on both arms — the
detector's precision is unaffected; its **recall** collapses because a more
sensitive tier 2 removes the coverage holes the trigger was firing on.

**INVALIDATED — every result produced before this fix, by directory:**

- `results/p10_plane_fix/` (all seeds)
- `results/p10_env/` (all box sizes, all seeds) — including `ENVELOPE_TABLE.txt`
  and `PREDICTION.md`, whose `spacing/r80` axis rescales by 1.26× at every point
- `results/campaign_recovery/` (all 12 cells × all seeds)
- `results/causal/`, `results/baseline_v*`, and every `results/` directory whose
  sidecar predates this fix
- `results/probe_audit/probe_range_sweep.csv` and the tier-2 **r80 = 909.5 m**
  quoted in `Architecture.md:71`, `CA-TBL_Technical_Report_SelfContained.md:62`
  and `CA_TBL_SIMULATION_REPORT.md:72` — now **1149.5 m**

Claim 3's window (1.0–1.3 in `spacing/r80`) and the coverage-shaped upper edge of
the envelope both move. Claim 9's falsification rested on the absence of a
qualifying fallback rather than on tier-2 range, and is not obviously disturbed —
but it was measured on invalidated runs and must be re-measured to be quoted.

---

## D2 — `mean_snr_db` was accumulated only over delivered packets

**Site.** `scratch/ca_tbl_causal_sim.cc:1393-1400` — the SNR accumulator sat
inside `if (received)`, while every sibling channel diagnostic accumulates under
`diagN`, over all trials.

**Convention violated.** Stated in the file's own comment eight lines above
(`ca_tbl_causal_sim.cc:1391-1395`): *"Channel diagnostics are accumulated over ALL trials, not only
received ones. Conditioning them on reception would select exactly the favourable
draws and hide the mechanism being looked for."* `snrSum` was the one channel
diagnostic outside the block that comment guards.

**Fix.** An unconditional accumulator (`snrAllSum`/`snrAllN`) added **alongside**
the conditioned one, not replacing it, emitted as `mean_snr_all_db` behind the new
`--traceDiagnostic` flag (default **false**, so default output is byte-identical
to every prior run — verified). This makes the bias measurable before any
consumer is switched over.

**Magnitude — now measured, not inferred.**

| tier | worst bias | where |
|---|---|---|
| 868 MHz | **+22.70 dB** | 6000 m, pdr 0.261 |
| 2.4 GHz | +13.96 dB | 2500 m, pdr 0.032 |
| UWB | +7.42 dB | 130 m, pdr 0.036 |

Positive everywhere and monotone in falling PDR — the survivorship signature. It
is exactly `+0.00` at every point where `pdr = 1.0`, which is the internal check:
conditioning is vacuous when nothing is dropped.

**A previous claim of "+9.61 dB measured bias" is withdrawn** — that came from the
2.4 GHz 6000 m row where `pdr = 0.0000`, i.e. the D3 sentinel, not a bias. The
difference between the column and a `pathLossDb`-based reconstruction cannot
isolate conditioning either, because `snr_db` is built from `totalLoss` and
post-EVM (`lora-physics-engine.cc:631-632`, `lora-physics-engine.cc:598-605`).

**Still open — a design question, deliberately not fixed here.** `meanSnr` feeds
the CRLB gate at `ca_tbl_causal_sim.cc:1450-1452`. The comment immediately below
identifies survivorship bias in `mae` and adds a PDR floor for exactly that
reason; whether `meanSnr` should get the same treatment, or switch to the
unconditional column, changes what that gate decides and is not a bug fix.

**Blast radius in analysis.** The UWB preamble-gain hypothesis argued from
`mean_snr_db = −14.52 dB` at 10 m. The measured unconditional value is
**−15.43 dB** (bias +0.91 dB), so that argument survives on the clean axis.
`src/catbl-propagation-study/validate.py` never reads the column and is
unaffected. `mean_snr_db` also appears in `run_final_validation.py` and
`average.py`, which write their own CSVs from a different accumulator and were not
examined.

---

## D3 — the `0.0` sentinel collided with the data domain

**Site.** `scratch/ca_tbl_causal_sim.cc:1437-1438`. `meanSnr` fell back to `0.0` when
`snrN == 0`; 0 dB SNR is a legitimate measurement, so the sentinel was
indistinguishable from data. `mae`, `twoRayFrac` and `nlosFrac` use `-1.0` in the
same block.

**Fix.** Emit `std::numeric_limits<double>::quiet_NaN()`. Every consumer is
already guarded on `snrN > 0`, so NaN reaches only the CSV, and only where no
trial was delivered and nothing was measured.

**Footprint — verified confined.** Pre/post diff at default flags is exactly one
field on one row:

```
2.4GHz,6000.0,0.0000,-1.000,0.00,...  ->  2.4GHz,6000.0,0.0000,-1.000,nan,...
```

868 MHz and UWB byte-identical; no other tier reaches `pdr = 0.0000`.

---

## D4 — `PassesReceiverSensitivity` is a logistic, not a hard threshold

**Site.** `lora-physics-engine.cc:135-146`. Reception is drawn against
`1/(1 + exp(-margin/width))` with `softThresholdWidthDb = 0.35`
(`lora-physics-parameters.cc:389`).

Not a simulator defect — a correction to how it had been described in this
module's analysis. At 0.35 dB the width is immaterial to every result here, but it
would matter to any experiment sweeping toward that width. **No fix; recorded so
the description does not recur.**

---

## Post-D1 regeneration — findings that belong with the defect record

### W1 — the 19/20-vs-14/20 deployment-reliability claim is WITHDRAWN

The headline "`ca_tbl_full` meets the requirement on 19/20 seeds where `static_2p4`
meets it on 14/20" was measured with tier 2 six decibels deaf (D1), at
`boxHalf = 2000` — a density at which the report's own conclusion says tier 2
should be pinned. It attributed a defect's effect to the design.

Regenerated at `63bba4f dirty 0` (`results/p10_plane_v7`, 20 seeds, digests
verified against files on disk):

    static_2p4    14/20 -> 19/20     M1 12.679 J
    ca_tbl_full   19/20 -> 20/20     M1 27.094 J

A five-seed reliability gap became one seed, at 2.1x the energy. The claim does
not survive and must be removed from `Simulation.md` §11, the technical report,
and `CA_TBL_HANDOFF_v4.md` §1.3.

**What replaces it** (`results/p10_env_v7`, 160 cells, all `dirty 0`, all digests
verified): the advantage is real but lives at a *lower density* than the claim
stated, and `boxHalf = 2000` is not in it.

Verdicts below are the registered rule of `b49df46` §4, not a seed-count
comparison. An earlier draft of this note labelled 2500 and 3000 **PASS** on
"+9 seeds" and "+11 seeds"; that was `seeds_meeting_spec` used as a threshold
after its withdrawal, and it also inverted the registered criterion, which decides
on M1 and not on seed counts. Corrected here.

M1 ratio is `ca_tbl_full / static_2p4`, geometric mean of per-seed ratios,
seed-level bootstrap 95% CI, 10 000 resamples, 20 seeds paired.

| boxHalf | s/r80 | full meets | 2p4 meets | M1 ratio [95% CI] | verdict |
|---|---|---|---|---|---|
| 1500 | 0.61 | 20/20 | 20/20 | 1.924 [1.910, 1.940] | **FAIL** (resolved) |
| 2000 | 0.82 | 20/20 | 19/20 | 2.139 [2.112, 2.170] | FAIL or INCONCLUSIVE — rule silent |
| 2500 | 1.02 | 19/20 | 11/20 | 2.415 [2.365, 2.463] | FAIL or INCONCLUSIVE — rule silent |
| 3000 | 1.23 | 12/20 | 1/20 | 2.806 [2.756, 2.859] | FAIL or INCONCLUSIVE — rule silent |
| 4000 | 1.64 | 0/20 | 0/20 | 3.121 [2.977, 3.271] | **INCONCLUSIVE** (resolved) |
| 5000 | 2.04 | 0/20 | 0/20 | 2.327 [2.053, 2.664] | **INCONCLUSIVE** (resolved) |
| 6000 | 2.45 | 0/20 | 0/20 | 1.509 [1.361, 1.704] | **INCONCLUSIVE** (resolved) |
| 7500 | 3.07 | 0/20 | 0/20 | 1.073 [0.987, 1.183] | **INCONCLUSIVE** (resolved) |

**No box size is PASS, and none can be.** PASS requires `ca_tbl_full`'s M1 to be
*lower* with the CI entirely below 1.0. The CI is entirely **above** 1.0 at every
density where `ca_tbl_full` meets the requirement at all — it costs 1.9x to 2.8x
more. How the unregistered "arm meets (A,R)" is binarized changes FAIL vs
INCONCLUSIVE; it cannot produce a PASS.

**The architecture has no density at which it pays under `(A = 0.95, R = 25 m)`.**

### W2 — WITHDRAWN. `trig_precision` 0.9343 *is* reproducible; I used the wrong estimator

**The finding below was mine and it was wrong. Kept with its reason, not deleted.**

> ~~The ledger publishes `trig_precision = 0.9343`. Recomputed over all 20 seeds of
> `results/p10_plane_fix` — the directory the ledger cites — the mean is **0.9279**.
> No population over that directory was found that yields 0.9343. This is a
> provenance gap, not rounding.~~

**What was actually compared.** The ledger's own text specifies *"20 seeds pooled"*.
Pooled means summing the counts across seeds and dividing once —
`tp / (tp + fp)` over `tp = 1280`, `fp = 90`. I computed the **mean of the twenty
per-seed `trig_precision` values** instead, and compared that against a pooled figure.

    pooled over 20 seeds     precision 0.9343   recall 0.7041   <- the published pair
    mean of per-seed ratios  precision 0.9279   recall 0.7005   <- what I computed

Two different estimators, not a failed reproduction. "No population over that
directory was found that yields 0.9343" was false: the population was stated in the
ledger and I did not read it.

**There was never a provenance gap here.** Any provisional marking derived from this
entry is void.

### W3 — the "recall falls" prediction is UNTESTED, not missed

Registered: `trig_recall` falls after D1, because a more sensitive tier 2 removes
the coverage holes the trigger fires on. Measured mean rose +0.029, but the
per-seed sign is **11 up / 9 down** at 20 seeds, which establishes no direction.
Record it as untested.

The analysis output that first reported this printed "Direction is not marginal."
That string was hardcoded in the script, not derived from the sign split. It was a
script artefact and is corrected here.

### W4 — Claim 3's ledger entry "PROVEN, SCOPED — window 1.0–1.3" is WITHDRAWN

Independent of D1. Read `results/p10_env/ENVELOPE_TABLE.txt` as it stands: the
pre-registered verdict was **FAIL** at `spacing/r80` 0.78 and **INCONCLUSIVE** at
1.03, 1.29, 1.55, 2.07, 2.58 and 3.10. **No box size was ever PASS.**

The INCONCLUSIVE entries at and above 1.29 read "ca_tbl_full does not meet (A,R)"
— i.e. nothing met the requirement there, the tiered arms included. A window was
inferred from a region in which no verdict was a pass and the proposed system did
not meet its own requirement.

`p10_env_v7` is the first campaign in this project to resolve any geometry to a
registered verdict on clean provenance, and the resolved verdicts are FAIL at 0.61
and INCONCLUSIVE from 1.64 out. The corrected reading is that Claim 3 was
overstated before the fix and is not rescued by it.

### W5 — the rule is silent where the behaviour is

`b49df46` §4 decides on "`ca_tbl_full` meets (A,R)" and "single-band arm that also
meets (A,R)", both arm-level binary predicates. The only registered way to turn
20 per-seed outcomes into an arm-level "meets" was `>=15/20`, **withdrawn as
unregistered** (F1/F5/PC3). Nothing replaced it.

The rule therefore resolves only the geometries where an arm is unanimous — 20/20
or 0/20. It is silent at `boxHalf` 2000, 2500 and 3000, which is exactly where the
arms separate. This is a gap in the registered rule, recorded rather than closed:
choosing a binarization now would be the same move the pre-registration exists to
prevent.

What can be said without binarizing: the M1 CI is entirely above 1.0 at all three,
so none of them can be PASS under any binarization.

### W6 — the window is bounded on both sides, and the upper bound is not about tiering

Between `spacing/r80` 1.23 and 1.64 every arm drops to 0/20, tiered included.
That is the coverage-shaped upper edge surviving D1: it is a property of
**co-located siting** — all tiers anchored at the same facilities — not of the
tiering policy. Adding radios cannot reach a casualty outside every anchor's
range, so no policy over those radios can either.

The observable that does transfer: the density at which `static_2p4` loses
availability while the tiered arms retain it sits at `spacing/r80` 1.02-1.23,
unchanged across a 26% shift in `r80` (the same region moved from `boxHalf`
~1935-2515 to 2500-3000, ~29% in physical units). `spacing/r80` transferred, which
is what a scale-free axis is chosen for. This is a statement about an
**observable**, not a verdict, and it does not make Claim 3 pass.

---

## D5 — PCS unweighted-sum term saturation

**Site.** `ca-tbl-policy.cc:666-671`, threshold `ca-tbl-policy.cc:57`.

```cpp
term1 = traceRef / (traceRef + traceSigma);   // filter uncertainty, -> 0 as P grows
term2 = 1.0 / std::max(gdop, 1.0);            // geometry
term3 = nLos / nTotal;                        // LOS fraction
pcs   = term1 + term2 + term3;                // unweighted sum
// escalate when pcs < pcsThresholdUp[tier], = 1.55 on tier 1
```

Three terms each bounded by `[0,1]` are summed without weights against a threshold
of 1.55. `term1` tends to **0** as the covariance grows, so once it saturates,
`term2 + term3` alone can hold PCS above threshold no matter how uncertain the
filter is. The uncertainty term cannot veto; it can only fail to contribute.

**Measured, D1 config E** (`RURAL_OPEN propagation dwell=90
--suspectForcesEscalation=false`, seed 1):

```
term1 0.0006   term2 1.0000   term3 0.9167   -> pcs 1.9179 > 1.55
0 escalations, 1440/1440 cycles on tier 1, cep50 89.91 m
traceSigma p50 16797 m^2 (sigma ~ 130 m), |e| p50 89.92 m, NEES 1.95
```

**The filter is correctly calibrated.** NEES 1.95 against a 2-DOF expectation of
2.0 — the covariance tracks the error accurately. The estimator knows it is ~130 m
uncertain; PCS structurally cannot express it.

**Control, D1 config A** (`legacy`): `term3` 0.25, sum 1.19 < 1.55, escalation
fires (4 escalations, cep50 2.42 m).

**Incidence, measured across 10 populations** (fraction of cycles where `term1 <
0.01` *and* `term2 + term3 > threshold` simultaneously):

| population | seeds | term1 p50 | term3 p50 | both-hold | escalations | NEES | \|e\| p50 |
|---|---|---|---|---|---|---|---|
| D1-A RURAL_OPEN legacy dw90 | 1 | 0.2933 | 0.2500 | 0.000 | 4 | 0.94 | 2.42 |
| D1-B RURAL_OPEN propagation dw90 | 1 | 0.4018 | 1.0000 | 0.100 | 150 | 1.37 | 3.11 |
| D1-C RURAL_OPEN legacy dw800 | 1 | 0.4631 | 0.2500 | 0.000 | 3 | **10.14** | 1.39 |
| D1-D RURAL_FOREST legacy dw800 | 1 | 0.1608 | 0.2500 | 0.001 | 19 | 0.68 | 2.89 |
| **D1-E RURAL_OPEN propagation sFE=false** | 1 | 0.0006 | 0.9167 | **0.998** | **0** | 1.95 | 89.92 |
| D1-F URBAN_DENSE propagation dw90 | 1 | 0.0006 | 0.9091 | 0.763 | 138 | 1.82 | 68.12 |
| GRID RURAL_OPEN legacy dw90 | 20 | 0.2788 | 0.2500 | 0.000 | 62 | 1.13 | 2.47 |
| GRID RURAL_OPEN propagation dw90 | 20 | 0.4127 | 1.0000 | 0.054 | 2948 | 1.35 | 2.85 |
| GRID RURAL_FOREST propagation dw90 | 20 | 0.1743 | 1.0000 | 0.123 | 2536 | 1.21 | 4.27 |
| GRID URBAN_DENSE propagation dw90 | 5 | 0.0005 | 0.9090 | 0.741 | 678 | 1.96 | 78.22 |

**Scope, and it is narrower than first reported.** Saturation is common only where
the error is already large (E, F, URBAN: `|e|` 68-90 m, both-hold 0.74-1.00) and
rare where it is small (both-hold 0.05-0.12). It is therefore **coupled to
divergence, not independently causing it** — and in nine of the ten populations
escalation is supplied by the *suspect* path, not by PCS, so PCS's contribution is
unobservable there.

**Config E is the only population in which PCS is the sole escalation trigger**,
and it is the only one with zero escalations. The defensible claim is: *where PCS
is the only trigger and the covariance term has saturated, escalation cannot fire
however large the error becomes.* Anything broader is not supported by these data.

**Not tested, and therefore not claimed:** that a weighted form, a `term1` gate, or
`min` instead of sum would fix it. All three are plausible; none was run. A
corrected PCS belongs in future work with this measurement as its motivation.

**Status: RECORDED, NOT FIXED.** A fix changes the engine and invalidates every
result in the tree.

**Incidental:** D1-C — the H_cost PASS cell — shows **NEES 10.14** against an
expectation of 2.0, the only population that is not calibrated. Not investigated.

---

# Withdrawal log — 2026-08-29

Every entry records what was removed, why, and what replaced it. A withdrawn number
is withdrawn **with its reason and its mechanism**, never silently deleted.

## W1 — the 19/20-vs-14/20 deployment-reliability table

Removed from `CA_TBL_HANDOFF_v4.md` §1.3. Measured with tier 2 six decibels deaf
(D1, fixed by `0d5e747`). Replaced by `results/p10_plane_v7` at `63bba4f dirty 0`:
`ca_tbl_full` 19/20 → 20/20, `static_2p4` 14/20 → **19/20**. A five-seed reliability
gap became one seed at 2.1× the energy, and `static_2p4` wins M1 (12.679 J against
27.094 J). The argument does not survive.

## N3 — the integrity ablation (positive 4, negative 1, tied 15)

Withdrawn from `CA_TBL_HANDOFF_v4.md` §1.3 as **pre-D1, not as wrong**.

It reproduces **exactly** from `results/p10_plane_fix`. That established provenance,
never validity. Withdrawn because the 6 dB defect reaches the quantity it measures:
the arms differ only at `ca-tbl-policy.cc:930-931`, whose trigger is
`mahal > threshold && nNlos == 0`; tier-2 occupancy in that campaign is **99.6%**
(27 322 vs 108 cycles), so `nNlos`, the geometry and the residuals all come from the
links the error changes. Gate eligibility measured at **151/28 718 = 0.53%** all-LOS
fix cycles, yielding **9 suspects over 20 seeds**. Not a common-mode shift; it does
not cancel in the difference.

## W4 — Claim 3's old ledger entry, "PROVEN, SCOPED — window 1.0–1.3"

Withdrawn, and **independently of D1**. Every verdict in the old
`results/p10_env/ENVELOPE_TABLE.txt` was FAIL or INCONCLUSIVE, and the INCONCLUSIVE
entries read "ca_tbl_full does not meet (A,R)". A window was inferred from a region
in which the proposed system did not meet its own requirement.

## Trigger metrics — WITHDRAWN: the published pair WAS reproducible, and I used the wrong estimator

**This entry previously claimed `trig_precision = 0.9343` and `trig_recall = 0.7041`
could not be reproduced from `results/p10_plane_fix`. That claim was wrong and is
withdrawn.**

They reproduce **exactly**, under the estimator the ledger's own text specifies —
*"20 seeds pooled"*:

    pooled over 20 seeds     precision 0.9343   recall 0.7041   <- matches published
    mean of per-seed ratios  precision 0.9279   recall 0.7005   <- what I computed

I computed the mean of per-seed ratios and compared it against a pooled figure. Those
are different statistics, not a failed reproduction. The population was stated in the
ledger and I did not read it.

**There was never a provenance gap here.** The "provisional pending re-measurement"
marking is removed.

**A second claim in the withdrawn entry was also wrong.** It said the trigger metrics
"come from injection campaigns only; in non-injection configurations `trig_tp +
trig_fp = 0`". `p10_plane_fix` is a **non-injection** campaign and has `tp + fp =
1370` over 20 seeds, pooled.

**The correct scope, stated with its population.** The trigger metric has no
population in the six `p10_grid`-derived configurations D1 tested (configs A-F, one
seed each, `boxHalf` 1500-2000, `envFacility` 0/1/4). It has a substantial population
in the `p10_plane_fix` / `p10_plane_v7` design (`boxHalf=2000`, `gwGrid=5`,
`escalationRequiresEvidence=true`, no injection): `tp + fp = 1370` and `1459`
respectively over 20 seeds. Whether a configuration exercises the trigger is a
property of that configuration, not of whether it injects faults.

## Trigger metrics on the corrected engine

Already measured, and no new campaign was needed. `results/p10_plane_v7`, 20 seeds,
`source 63bba4f dirty 0` on all twenty — the post-D1 regeneration of
`p10_plane_fix` carried the `trig_*` columns:

| estimator | pre-D1 (`p10_plane_fix`) | post-D1 (`p10_plane_v7`) |
|---|---|---|
| **pooled, 20 seeds** | 0.9343 / 0.7041 | **0.9164 / 0.7354** |
| mean of per-seed | 0.9279 / 0.7005 | 0.9152 / 0.7296 |

Precision falls ~0.018, recall rises ~0.031. **No direction was predicted for either**
— D1 changed which anchors are heard, which changes `nNlos`, which changes the gate's
eligible population, and there was no basis for guessing the sign. These are recorded
as *measured values*, not as a hit or a miss.

The pre-D1 pair keeps its reason and its source: it is what the defective engine
produced, and it is superseded rather than deleted.

## D5 addendum — config C's NEES anomaly, resolved

`DEFECTS.md` D5 recorded config C (the H_cost PASS cell) at **NEES 10.14**, flagged
"not investigated". Investigated:

| population | tier | cycles | NEES mean | NEES p50 | frac > 5.99 |
|---|---|---|---|---|---|
| config C, 1 seed | 2.4GHz | 964 | 0.99 | 0.59 | — |
| config C, 1 seed | UWB | 473 | 28.86 | 0.51 | 0.055 |
| **grid twin, 20 seeds** | 2.4GHz | 17 703 | **1.01** | 0.59 | 0.0089 |
| **grid twin, 20 seeds** | UWB | 11 061 | **6.08** | 0.55 | 0.0717 |
| **grid twin, 20 seeds** | all | 28 800 | **2.95** | **0.58** | 0.0330 |

**The 10.14 was a single-seed heavy-tail artifact of using the mean.** At 20 seeds the
same configuration gives mean **2.95** against a 2-DOF expectation of 2.0, median 0.58.
The filter in the PASS cell is calibrated comparably to the other populations;
the overconfidence reading taken from the single-seed mean does not survive the seeds.

Registered prediction — **two hits, one miss**:

- *anomaly is tier-3-specific* — **HIT**. UWB mean 6.08 against tier-2's 1.01.
- *config C's tier-2 NEES in the 0.7–2.0 band* — **HIT**. Mean 1.01.
- *mechanism is an over-shrunk covariance rather than an error excursion* — **MISS**.
  Both contribute: in the tail `traceSigma` is 3.1× smaller **and** `|e|` is 3.3×
  larger, and since NEES goes as `e²/σ²` the error term dominates. Only 5/26 tail
  cycles lie near a tier transition, so transitions are not the driver.

**Consequence for Claim 3:** the PASS is adjudicated on ground-truth error and energy,
neither of which reads the covariance, so its arithmetic is unaffected. The caveat that
attaches is about *how the policy got there*, not whether it got there.

---

## Incident — CA_TBL_HANDOFF_v4.md was tracked, and the rule was enforced by memory

`2ab38c2` added `CA_TBL_HANDOFF_v4.md` to the index for the first time, violating a
standing rule stated in 19 project documents: **the handoff stays untracked**. The
content edits in that commit were correct; the staging was not.

`03c15dd` removed it with `git rm --cached`. The file and its edits remain on disk.

**History was not rewritten, deliberately.** Two rules collided and the stronger one
won: *never rewrite history* is absolute and was settled when the vendored-import
structure was adopted, while *the handoff stays untracked* is hygiene about a working
document. The repository is private, every commit is authored by Kushal, and the file
contains no credential — so the content sitting in one historical commit costs almost
nothing, against a history rewrite costing a re-run of the entire import and a
re-verification of every ref. `03c15dd` is the complete remedy.

**The real fix is that the rule is no longer enforced by recall:**

- `.gitignore` now lists `CA_TBL_HANDOFF_v4.md` and `SESSION_CHANGELOG.md`, enumerated
  from the project's own guardrail blocks rather than from memory.
- **G8** in `scripts/catbl_gates.sh` asserts neither is tracked, and names
  `git rm --cached` as the remedy in its failure message. It catches the case
  `.gitignore` cannot: a path already in the index, or added with `git add -f`.
- G8 has a negative fixture (`/tmp` repo, path force-added, confirmed to fail, then
  confirmed to pass again after `git rm --cached`).

**Side effect, flagged rather than resolved.** `2ab38c2` also first-tracked
`Architecture.md` and `CA_TBL_SIMULATION_REPORT.md`. See the note below.

---

## Provenance gap — the cited evidence base is largely `dirty`, and there is no clean alternative

Asked of `campaign_20260810` (280 cells carrying `ALLOW_DIRTY`): does any published
number depend on it alone, and can `campaign_recovery` establish the same thing on
clean provenance?

**Exactly one tracked-document citation reaches it** — `[M-24]` in `Simulation.md:667`,
citing `results/campaign_20260810/stage1/` (121 cells) for the integrity-detection
figures: separation 7.73× (F1) / 16.45× (F3), F0 suspect rate 0.0046, latency median 0.

**The repointing cannot be done, because the premise is false.**

    campaign_20260810   340 cells   source a059203 dirty 4   ALLOW_DIRTY markers: 341
    campaign_recovery   240 cells   source a059203 dirty 4   ALLOW_DIRTY markers:   0

`campaign_recovery` is **equally dirty**, and arguably worse: it carries the same
`dirty 4` provenance **without** the `ALLOW_DIRTY` markers that acknowledge it. A
reader checking for the marker would conclude it was clean.

**No campaign anywhere under `results/` has both `dirty 0` and fault injection.**
Searched exhaustively; zero hits. So `[M-24]` cannot be repointed to clean evidence
because none exists.

**The three counts:**

| outcome | count |
|---|---|
| citations repointed to clean provenance | **0** — repointing gains nothing |
| resting uniquely on dirty evidence with no clean alternative | **1** (`[M-24]`) |
| unaffected | all others — only one citation reached `campaign_20260810` |

**The larger finding, which the question surfaced but did not ask for.** Across every
`results/` directory cited by a tracked document:

    recording dirty 0        3
    recording dirty != 0     8   campaign_20260810/stage1, campaign_recovery, causal,
                                 det_a, det_b, p10_env, p10_plane, p10_plane_fix
    no readable sidecar     24

**8 of the 11 cited directories that have a readable sidecar record a dirty tree.**
This is not a `campaign_20260810` problem; it is the state of the evidence base. It
also reaches **Claim 9's falsification**, which rests on `campaign_recovery` via
`[M-25]`/`[M-26]`.

**Cost of closing it.** Re-running `campaign_recovery` alone is 240 cells at roughly
1.9 min/cell wall at 11-way — about **7.6 hours**. `campaign_20260810/stage1` is a
further 121 cells, about 3.8 hours. Neither is started here, and neither is a
correctness problem: the numbers are what the runs produced. It is a
**reproducibility** problem — a reader cannot rebuild the binary those runs used,
because no commit corresponds to it.

**Not deleted, not quietly kept unqualified.** `[M-24]` stays, with this entry as its
qualification.

---

## The population rule now has an estimator clause

The project's standing rule is *a count is not an observable until its population is
stated*. Two findings withdrawn in this file share its shape with a different
unstated quantity:

- **W2** — the unstated thing was the **estimator**. Pooled and mean-of-per-seed are
  different statistics over the same population, and comparing one against the other
  produced a false "cannot be reproduced".
- **The `trig_tp + trig_fp = 0` scope claim** — the unstated thing was **which
  directories the claim ranged over**. Six configurations were generalised to "outside
  injection campaigns", and a non-injection campaign with 1370 trigger events refutes it.

**The rule as it should now read:** *a count is not an observable until its population
**and its estimator** are stated.* Wherever a pooled or averaged figure appears in
this project, the estimator is named beside it.

A third instance of the same shape, for completeness: two withdrawn findings in this
file were **glob-depth errors** — a survey that globbed one directory level too
shallow reported 0 paired cells for a campaign holding 280, and a sidecar census
missed 770 sidecars by looking two levels down instead of at any depth. **A glob is a
population claim.**

---

## Claim 9's falsification, re-measured on citeable provenance

`results/c9_clean`, 40 cells, **`source cd594c9 dirty 0` on all 40**, 120 digest
entries verified against files on disk, 0 failures. Flags read from
`campaign_recovery`'s own sidecar, not inferred.

| | c9_clean (`dirty 0`) | campaign_recovery (`dirty 4`) |
|---|---|---|
| **F1** availability, `full − no_integrity` | **−0.0305**, 1 pos / 18 neg / 1 tie | −0.0491, 1 / 19 / 0 |
| **F1** cep50M difference | +0.011 m | +0.180 m |
| **F1** suspects (median) | 120 | 136 |
| **F3** availability, `full − no_integrity` | **−0.0540**, 1 pos / 19 neg | −0.0509, 0 / 20 |
| **F3** cep50M difference | +0.078 m | +0.257 m |
| **F3** suspects (median) | 206 | 200 |
| arms differ | 20/20 both sets | 20/20 both sets |

**Registered prediction: HIT.** The falsification reproduces on the corrected engine.
`ca_tbl_full` does not beat `ca_tbl_no_integrity` — availability is *lower* by 0.031
(F1) and 0.054 (F3), the sign holds on 18–19 of 20 seeds in each set, and `cep50M` is
worse by a small positive margin. The arms differ in every cell, so the comparison is
live throughout.

**Not predicted, and reported as measured values only:** suspect counts moved 136 → 120
(F1) and 200 → 206 (F3) — opposite directions. No direction was registered for these
because D1 changed which anchors are heard, hence `nNlos`, hence the gate's eligible
population, and there was no basis for guessing the sign. **These are not scored as
hits or misses.**

**Consequence applied.** Claim 9's ledger verdict is unchanged and now has a citeable
source. `results/campaign_recovery` is **superseded** by `results/c9_clean` for the
`suspectRecovery=none` comparison, with its reason: `dirty 4`, no `ALLOW_DIRTY`
markers, no commit reproduces its binary. It is **not** superseded for the
recovery-policy ablation (`inflate` / `bounded` / `reinit`), which `c9_clean` did not
re-measure — that remains on dirty provenance and is recorded here as still open.

### What `suspectForcesEscalation=false` means for how Claim 9 reads

That flag is in `campaign_recovery`'s own flag set, and therefore in `c9_clean`'s.
**Both campaigns already isolate rejection from escalation** — the thing D7 was built
to do and could not, because with the flag false `ca_tbl_full` stopped escalating
entirely in the D1 config-E pre-check.

So Claim 9's falsification is a statement about **rejection alone**: detecting a
suspect fix and discarding it, with no escalation to a better tier following. It is
**not** a statement about the combined detect-and-escalate mechanism, which under
`propagation` NLOS moves `cep50M` from 92.15 m to 3.10 m (D1 configs B/E).

That is a narrower and stronger reading than "the integrity check does not pay", and
the ledger should carry it that way.

---

## The recovery-policy ablation on clean provenance — and the last gap closed

`results/abl_clean`, 120 cells, **`source ccaedb0 dirty 0` on all 120**, 360 digest
entries verified against files on disk, 0 failures. Pre-flight
(`degeneracy_check.py <dir> integrity`, `suspectRecovery=inflate`, seed 1): **LIVE**,
71 suspects, 37 fields differing.

Availability, `ca_tbl_full − ca_tbl_no_integrity`. Negative means the policy **loses**.
Estimator: mean of per-seed differences over 20 seeds, with the sign test beside it.

| policy | set | mean diff | pos/neg/tie | suspects (median) |
|---|---|---|---|---|
| `none` | F1 | −0.0305 | 1/18/1 | 120 |
| `none` | F3 | −0.0540 | 1/19/0 | 206 |
| `inflate` | F1 | **−0.0053** | 7/13/0 | 84 |
| `inflate` | F3 | −0.0085 | 3/17/0 | 128 |
| `bounded` | F1 | −0.0152 | 5/15/0 | 122 |
| `bounded` | F3 | −0.0136 | 2/18/0 | 180 |
| `reinit` | F1 | −0.0164 | 5/15/0 | 118 |
| `reinit` | F3 | −0.0172 | 2/18/0 | 180 |

**Registered prediction: HIT.** None of the three recovery policies beats
`no_integrity` on availability. Every cell is negative in the mean and the sign test
runs against the policy in all eight.

**Not predicted, and therefore not scored:** the magnitude, and the ordering
`inflate` < `bounded` ≈ `reinit` < `none` in loss. No ordering was registered because
there was no basis for one. It is reported as a measured value: `inflate` loses least
(−0.005 to −0.009) and `none` loses most (−0.031 to −0.054), roughly a 6× spread. That
is consistent with a bounded response subtracting less than an unbounded rejection,
but **no mechanism was measured to establish it** and it is not claimed as one.

## No published number rests on dirty provenance

With `abl_clean` landed, **`results/campaign_recovery` is superseded in full**, with
its reason: `source a059203 dirty 4`, no `ALLOW_DIRTY` markers to acknowledge it, and
no commit that reproduces its binary. Its four recovery levels × three fault sets are
replaced by `c9_clean` (`none`) and `abl_clean` (`inflate`, `bounded`, `reinit`), both
at `dirty 0` with digests verified.

The claim holds only where it is scoped: **every number this project publishes as a
result now traces to a `dirty 0` source.** Two qualifications travel with that
sentence and are not to be dropped:

- `results/campaign_20260810/stage1` still backs `[M-24]` (integrity detection
  separation, F0 suspect rate, latency) and remains `dirty 4`. It is qualified in
  place in `Simulation.md` and was never repointable, because no clean injection
  campaign existed at the time. **`c9_clean` and `abl_clean` do not re-measure those
  particular statistics**, so `[M-24]` is the one published figure still on dirty
  provenance.
- `publication_ready.sh`'s `DIRTY` assertion scans the directories the *report* cites,
  and the report still cites pre-D1 paths. That is a citation-repointing task, not a
  provenance one.

---

## WITHDRAWN: `publication_ready.sh`'s DIRTY assertion does NOT match by substring

**This entry was mine and it was wrong.** Kept with its reason.

> ~~Its scan uses `grep -rhoE 'results/[A-Za-z0-9_.-]+'` with no word boundary, so a
> citation of `results/p10_plane_v7` also registers as a citation of
> `results/p10_plane`.~~

`grep -oE` with a greedy character class **extracts the whole token**. Run against the
technical report alone it returns exactly:

    results/abl_clean   results/baseline_v7   results/c9_clean
    results/campaign_20260810   results/p10_env_v7   results/p10_env_v8
    results/p10_grid   results/p10_plane_v7

`results/p10_plane` is **not** among them. The check is correct.

**What I actually did.** I diagnosed the assertion using `grep -l "results/p10_plane"`,
which *is* a substring match, and attributed my own command's behaviour to the
assertion's. The two use different greps. This is the fourth finding in this file
withdrawn for describing a tool's behaviour without running the tool's own command —
after two glob-depth errors and one estimator mismatch.

**The residual `DIRTY` count is therefore entirely genuine.** All 43 dirty sidecars
come from `report/REPORT_DATA_PACK.md`'s real citations of `results/p10_plane`,
`results/p10_plane_fix`, `results/causal`, `results/det_a` and `results/det_b` — the
superseded campaigns that document describes on purpose, and which are marked as such
at the top of the file. Nothing is overcounted and there is nothing to fix.

---

## The attribution rewrite of 2026-09-01, and what it supersedes

**An earlier entry in this file records the `CA_TBL_HANDOFF_v4.md` incident with the
reasoning "history was not rewritten, deliberately". That reasoning is superseded —
for a different reason, and by an owner instruction, not by a change of mind.**

The original entry is not deleted. It keeps its reason: at the time, the collision was
between *never rewrite history* and a hygiene rule about a working document, and the
stronger rule won.

**What the audit found.** Across all 83 commits:

| field | affected |
|---|---|
| author name / email | **0** |
| committer name / email | **0** |
| message body | **27**, all carrying one identical trailer line |

Author and committer identity were already correct throughout. Only message bodies
were affected, and the oldest was `b9e650f` (2026-08-26) — so 30 commits were
rewritten and the 53 before them keep their SHAs.

**What was authorised, and how narrowly.** A rewrite, **once, for attribution only**.
The standing never-rewrite rule was adopted as hygiene around a settled vendored
import; an owner instruction to remove attribution from his own private repository
outranks it. No other history change is authorised, ever.

**What changed.** Metadata only, verified four ways before the push:

- `git log --all --format='%an|%ae|%cn|%ce' | sort -u` → exactly one identity
- attribution grep across local branches → **0**
- `git diff <old-tip> <new-tip>` → **empty**, no file content altered
- commit count **83 → 83**, order and dates preserved, linear no-merge shape intact

A fresh clone of the remote returns **0** attribution and one identity — the check that
matters, since local refs can be stale.

**Backup.** A new bundle, `ca-tbl-pre-attribution-rewrite-20260901-224445.bundle`
(51 MB), verified to list the pre-rewrite tip before the rewrite began. The four
pre-existing bundles in `~/` were not touched.

**The cost, and how it is carried.** 1283 `run_id.txt` sidecars cite six SHAs that
changed. **Sidecars were not edited** — they are evidence, and rewriting them would be
falsification. They resolve through `SHA_MAP.md`, which is the old→new table. 24 SHA
references in tracked documents *were* repointed, with `SHA_MAP.md` as the record.

**Enforcement.** The rule was violated because nothing enforced it. **G12** now asserts
no commit reachable from `HEAD` carries attribution in author, committer, or message
body, with a negative fixture covering both fields independently. `user.name` and
`user.email` are set explicitly in this repository; there is no commit template and no
active hook that could reintroduce a trailer.

**The handoff content that sat in the rewritten `35466c0`** is carried through
unchanged — the tree diff was empty, so no file content moved in the rewrite.

---

## `validate.py` rebuilt: the model now computes the link budget, and disagrees for a named reason

**The defect.** The previous `validate.py` read `mean_margin_db` from the probe CSV and
applied a shadowing distribution to it. The link budget — the quantity §7 exists to
validate — was imported from the subject. Detuning `K_BOLTZ_DBM_HZ` by 14 dB and a
tier's TX power by 16 dB both produced **byte-identical output**, because neither
constant was on the path.

**The rebuild.** `linkbudget.py` computes `rx_power → noise → margin → PDR → r80` with
**33 `file:line` citations** to the engine sources. The separation is structural, not a
convention: the file imports only `math`. The probe cannot be in scope because the
probe is not in scope, so a later edit cannot silently reintroduce the defect without
first adding an import that does not belong.

Both engine behaviours that a naive model gets wrong are built in from the start:
NLOS excess loss with distance (`lora-nlos-submodule.cc:47,84-93`) and the logistic
reception decision at `softThresholdWidthDb = 0.35` (`lora-physics-engine.cc:141-143`).

**Fixture B — four constants, four paths.**

| constant | path | movement |
|---|---|---|
| path-loss exponent `n` 2.2/2.3 → 3.2 | distance scaling | r80 868 4014.7→1048.6, 2.4 2169.6→205.0, UWB 30.9→10.6 |
| sensitivity → −90.0 dBm | receiver threshold | r80 868 4014.7→669.3, 2.4 2169.6→428.7, UWB 30.9→6.9 |
| 868 TX power 14.0 → 30.0 dBm | link budget | r80 868 4014.7→beyond the swept range |
| `K_BOLTZ_DBM_HZ` −174 → −160 | **noise floor** | r80 **unchanged**; ranging sigma **×5.01** on all three tiers |

**The noise constant not moving `r80` is correct, not a gap.** The engine's reception
decision is `rxPower >= effectiveSensitivity`
(`lora-physics-engine.cc:135-146`) — sensitivity-thresholded, not SNR-thresholded — so
the noise floor cannot enter `r80`. It enters the **CRLB ranging** path instead, where
detuning it moves sigma by 5.01×. The constant is wired; it is wired to the path the
engine actually puts it on.

**§7 reconciliation, at the registered 15% / 3 dB, against a fresh probe**
(`results/probe_v7`, `source fbf2153 dirty 0`, digest verified):

| tier | model r80 | engine r80 | rel | PDR worst | verdict |
|---|---|---|---|---|---|
| 868 MHz | 4014.7 | 1515.3 | 164.9% | 4.06 dB | FAIL |
| 2.4 GHz | 2169.6 | 1149.5 | 88.7% | 52.07 dB | FAIL |
| UWB | 30.9 | 12.8 | 140.8% | 3.57 dB | FAIL |

**The model was not tuned to match.**

**The differing term, isolated.** Computed margin minus engine margin, per distance:

    868 MHz   +12.32 dB at 100 m (nlos_frac 0.013)  rising to +17.92 dB at 6000 m
    2.4 GHz    +2.64 dB at  10 m  falling to -2.00 dB at 430 m

A **constant ~12 dB offset on 868 at essentially-LOS distances**, plus ~6 dB that
accumulates with range. The constant part is not NLOS — `nlos_frac` is 0.013 where the
gap is already 12.3 dB.

**Which side is more likely wrong: the model.** 868 MHz runs in `RURAL_FORESTED` with
`vegWide = VEG_DENSE_FOREST` (`ca_tbl_causal_sim.cc:3257`), and the engine applies
`result.foliageDb` in its loss sum (`lora-physics-engine.cc:598-604`). **The model has
no foliage term at all.** 2.4 GHz sits in `RURAL_OPEN` with `VEG_SPARSE_SHRUBS`, and
its gap is near zero — consistent with foliage being the missing term rather than
something common to all tiers.

**This is model incompleteness, not an engine defect — and that is the opposite of
D1.** There, the model's refusal to reconcile indicted the engine and a six-decibel
indexing error was found. Here the disagreement indicts the model: the engine's loss
terms are named and sourced, and the model omits one of them. Saying so is the point of
the instruction not to tune.

**Consequence.** **G11 is wired into the suite and is RED**, for an accuracy reason
rather than a construction one — the precedent is G6, which sat red by design with its
remedy recorded beside it. **The propagation sweeps stay blocked.** The remedy is a
foliage term with `file:line` constants from the vegetation model, after which §7 is
re-run at the unchanged criterion.

**`Simulation.md` §11.2 is unchanged.** It states packet-level physics is unverified
for receiver sensitivity, and this reconciliation does not change that — a model that
cannot yet reproduce the engine cannot corroborate it.

---

## The PDR criterion was an artefact, and the foliage term closed the offset but not the slope

### The criterion was never well-defined, and its implementation made it worse

§7 registered *"within 3 dB on PDR"*. **PDR is a probability on [0,1]; "3 dB" on a
probability requires a stated transformation, and §7 gave none.**

The implementation used a log-ratio floored at 1e-6, which is unbounded as either side
approaches zero. It reported **52.07 dB for 2.4 GHz — entirely from one point**: at
6000 m the engine PDR is 0.0000 (clamped, −60 dB) against a model PDR of 0.1612
(−7.93 dB). The tier with the *smaller* margin error carried a PDR error twelve times
larger than the tier with the bigger one. No physical mechanism does that; a log-ratio
near zero does.

**Candidates, all computed before choosing:**

| candidate | 868 | 2.4 | UWB |
|---|---|---|---|
| (a) absolute difference in probability | 0.125 | 0.320 | 0.192 |
| (b) log-ratio inside 0.05 ≤ PDR ≤ 0.95 | — | **still 52.07** | — |
| (c) r80 and transition width | already registered separately | | |
| (d) \|computed margin − engine margin\|, dB | 4.10 | 19.91 | 2.05 |

**(b) did not fix it**, and the reason is its own lesson: the band test admitted a point
when only *one* side was in band, so the unbounded side still dominated. A population
filter has to be stated over both members of a comparison.

**Chosen: (d), and not because it is smallest.** (a) is numerically smaller and (d)
fails two tiers. The grounds:

- *"3 dB" is natively a dB quantity.* A margin is in dB; a probability is not.
- The margin **is** the link budget, which is what §7 exists to validate.
- It is bounded and defined at every distance, including where PDR saturates.
- It is only computable because the model now computes a margin. The previous
  implementation could not have used it, which is why it reached for a ratio.

**The 15% on r80 half is well-defined and stays exactly as registered.**

### The foliage term: sourced, predicted, and three of four predictions held

ITU-R P.833-10, `A_veg = A_max·(1 − exp(−depth·γ/A_max))`
(`lora-foliage-submodule.cc:36`), with `γ` and `A_max` from
`lora-physics-parameters.cc:304-309` and `depth ~ LogNormal(ln 100, 0.6)` from `lora-physics-parameters.cc:293`.
`VEG_DENSE_FOREST` on 868 and `VEG_SPARSE_SHRUBS` on 2.4/UWB
(`ca_tbl_causal_sim.cc:3174-3175`). **The values were read from source, never from the
size of the gap** — the pre-registration is committed at `c44a091`, before the term was
implemented.

| prediction | outcome |
|---|---|
| 868's constant offset closes to within ~2 dB | **MISS** — it closed from +12.32 dB to **−4.10 dB**, overshooting by 4.10 rather than landing within 2. The lognormal's *mean* exceeds its median 13.77 dB |
| 2.4 and UWB move by ~3 dB, **not** the <1 dB the governing document predicted | **HIT** — both moved ~3–4 dB. The document's "<1 dB" is confirmed missed |
| the distance-growing component does **not** close | **HIT** — 2.4 GHz's far gap is still **+19.91 dB** |
| the reconciliation still fails on at least 2.4 GHz | **HIT** — all three fail the 15% r80 |

**The improvement is large and is reported as an improvement in the model, not in the
agreement:** r80 error 164.9% → **30.9%** (868), 88.7% → **27.6%** (2.4), 140.8% →
**59.2%** (UWB). **UWB now passes the margin criterion at 2.05 dB.**

Fixture B's fifth constant confirms the term's signature rather than mere movement:
detuning `γ` for `VEG_DENSE_FOREST` from 0.20 to 0.60 moves 868's r80 by **33.7%** and
the two sparse-shrub tiers by **0.0%**. The asymmetry is the evidence.

### What remains, and which side is more likely wrong

**868's slope is now nearly right** — its gap runs −4.10 dB near-field to +1.50 dB at
6 km, so the residual is a constant over-correction, not a slope error.

**2.4 GHz's slope is not** — −1.27 dB at 10 m to **+19.91 dB at 6 km**. The modelled
NLOS probability is accurate there (model 0.105 against a measured `nlos_frac` of
0.1105 at 6 km), so the *probability* is right and the *magnitude* is short by ~20 dB.

**The model is again the more likely wrong side.** Named candidates, all present in the
engine's loss sum at `lora-physics-engine.cc:598-604` and all absent from the model:
`terrainDb` (clamped 0–30 dB, and Fresnel clearance degrades with distance),
`atmosphericDb`, and the Markov-state NLOS additions of +6 dB partial and +18 dB deep
(`lora-physics-engine.cc:352-355`) which are separate from the propagation NLOS
probability and which the model does not implement. **Which of those it is has not been
established and is not asserted.**

**G11 stays RED and the sweeps stay blocked.** `Simulation.md` §11.2 is unchanged: a
model that still misses a tier's slope by 20 dB cannot corroborate the engine's
receiver sensitivity.

### The asymmetry worth keeping

> **D1**: the model refused to reconcile and the **engine** was wrong — a six-decibel
> indexing error, found and fixed.
> **Here**: the model refuses to reconcile and the **model** is wrong — first an
> omitted foliage term, now a missing distance-dependent loss.

Same procedure, opposite verdict, and in both cases **the term was named before
anything was changed**. That is the argument for the instruction not to tune,
demonstrated twice from opposite directions, and it is worth more than either
individual outcome.

---

## Localising the residual: the Markov hypothesis is refuted, and the term is two-ray

### The test, and the registered prediction it missed

If the missing term were NLOS-conditioned with magnitude `M`, then
`residual = nlos_frac × M`, so `residual / nlos_frac` would be **constant and equal to
M**. Measured:

| tier | residual/nlos_frac, near → far |
|---|---|
| 868 MHz | **−315.5 → +2.8** |
| 2.4 GHz | **−2619.9 → +180.2** |

Not constant, and it **changes sign**. Decisively, the residual is **−1.27 to −4.88 dB
on 2.4 GHz at 10–100 m where `nlos_frac` is 0.000–0.002** — substantial residual on
effectively-LOS links, which an NLOS-conditioned term contributes nothing to.

**Registered prediction — "the residual concentrates on NLOS links, LOS residual within
a few dB" — is a MISS.** The Markov `+6/+18 dB` additions are out, exactly as the
falsifier specified.

### What the slope analysis says instead

Engine slope from its own margin column, against the model's exponent:

| tier | model | engine, far field | reaches its two-ray breakpoint? |
|---|---|---|---|
| 868 MHz | 23.0 dB/dec | 34.6 → 47.2 | d_b = **347 m**, sweep to 6000 m — yes |
| 2.4 GHz | 22.0 dB/dec | 42.2 → **61.0** | d_b = **977 m**, sweep to 6000 m — yes |
| UWB | 22.0 dB/dec | **20.7 → 23.7** | d_b = **3198 m**, sweep to 180 m — **never** |

**UWB's slope matches the model almost exactly, and UWB is the only tier that never
reaches its breakpoint.** `d_b = 4·h_t·h_r/λ` with h_t ≈ 30 m and h_r ≈ 1 m
(`lora-physics-parameters.cc:13-14`).

On 2.4 GHz the residual **crosses zero at ~1200 m** against a computed breakpoint of
**977 m** — a quantitative match, not a qualitative one.

### A correction to this file's own earlier record

An earlier entry states `two_ray_frac = 0.0000 at all nine distances` and treats the
two-ray mechanism as refuted. **That measurement was 868 MHz only** — nine distances is
the 868 sweep — and it was generalised to all three tiers. Measured in `probe_v7`:

    868MHz   two_ray_frac  0.0000 at every distance
    2.4GHz   two_ray_frac  0.8895 - 1.0000
    UWB      two_ray_frac  0.9960 - 1.0000

**The mechanism fires on two of three tiers, essentially always.** The original
observation was correct about 868 and wrong about the population it was applied to —
the same glob-shaped error this file has recorded three times, in a different guise.

### Consequence

The term to add is **two-ray ground reflection**, not a Markov addition and not terrain.
It is pre-registered in `experiments/TWORAY_PREREGISTRATION.md` and **has not been
implemented in this pass** — one term, one prediction, one verdict.

## The UWB margin/r80 inconsistency: partially consistent, and r80 is the harsher criterion

`dr80/dmargin`, computed by shifting sensitivity 1 dB and re-solving:

| tier | m per dB | as % of its own r80 |
|---|---|---|
| 868 MHz | 77.40 | **3.90%** |
| 2.4 GHz | 163.69 | 11.16% |
| UWB | 2.08 | **10.19%** |

UWB's 2.05 dB margin error maps to **4.3 m = 33%** of the engine's 12.8 m r80. The
observed r80 error is **59.2%** (20.4 m against 12.8 m, a 7.6 m gap). So the mean margin
error explains **about half** of it; roughly 26 percentage points come from something
else — most plausibly the transition *width*, since r80 depends on the shape of the PDR
curve and not only on its mean position. **That is a separate finding from the margin
residual and is not asserted as understood.**

**The 15% criterion is not relaxed.** It is registered and it stands. But it is
**2.6× harsher on UWB than on 868** in margin terms — 10.19% of r80 per dB against
3.90% — because r80 at 12.8 m is a shallow transition in absolute metres. That is
recorded as a limitation of the criterion, reported alongside the result it produces.
Any replacement is registered for future work, not applied here.

---

## Two-ray: traced, implemented, and VERIFIED — after two defects of mine

The engine's two-ray was traced before implementing
(`lora-two-ray-ground-submodule.cc:10-39`, `lora-physics-engine.cc:386-396`). The first
implementation made agreement much worse and was very nearly recorded here as a
refutation of the mechanism. It is not one. Full verdict in
`experiments/TWORAY_PREREGISTRATION.md`.

### W7 — WITHDRAWN: "the breakpoint is `4·h_t·h_r/λ`"

The engine uses **`4π·h_t·h_r/λ`** (`lora-two-ray-ground-submodule.cc:26`). Every breakpoint computed in this file was low
by a factor of π.

### W8 — WITHDRAWN: "the residual crosses zero at ~1200 m against a breakpoint of 977 m"

This was the argument that selected two-ray over the other two candidates, and it matched
a breakpoint that was wrong twice over — the π factor, and a 30 m anchor height the probe
never uses. The conclusion it reached happens to be right, which is precisely why it is
worth withdrawing explicitly: a void argument that lands on the right answer is the
hardest kind to catch, and this one survived because its conclusion looked good.

### The mechanism is CONFIRMED — against the engine's own column, not against `r80`

`r80` is a poor instrument for this question because every excess term sits between the
path loss and the PDR. Comparing directly to `mean_pl_db` removes all of them. Worst
absolute error across the full sweep:

| tier | log-distance `n` | two-ray | engine env |
|---|---|---|---|
| 868 MHz | **0.11 dB** | 9.06 dB | `RURAL_FORESTED` — gated off, correctly |
| 2.4 GHz | 7.77 dB | **0.96 dB** | `RURAL_OPEN` |
| UWB | 4.49 dB | **0.02 dB** | `RURAL_OPEN` |

Two-ray wins on exactly the two tiers whose `two_ray_frac` is 0.89–1.00 and loses on the
one whose `two_ray_frac` is 0.0000. Both directions, all three tiers.

### The two defects, both in my implementation

1. **Wrong heights.** I used the `lora-physics-parameters.cc:13` default of 30 m. The
   engine reads `rxPosition.z` (`lora-physics-engine.cc:385-386`) and only falls back to
   the distribution when `z <= 0`; the probe registers anchors at `{15.0, 6.0, 2.5}` with
   `nodeH = 1.5` (`ca_tbl_causal_sim.cc:1116-1117`), so the fallback never fires. The
   2.4 GHz breakpoint was 3068 m instead of **920 m** — putting the entire region where
   its `r80` lives in the wrong branch.
2. **Horizontal instead of slant distance.** The engine works in 3-D. At 10 m under the
   6 m anchor that is 0.80 dB, which is exactly the error seen there.

Averaging over the height *distribution* was tested first and recovered under 1 dB of the
~10 dB error. That is what ruled the distribution out and pointed at the positions.

### The lesson, which is not the one the last pass drew

A model change that worsens agreement is evidence about the **change**, not about the
**physics**. Last pass drew the opposite inference and was one commit away from recording
a mechanism as refuted while the engine's own diagnostic column showed it firing on
0.89–1.00 of links. The rule this project already had — *do not report a mechanism
without a column or code path showing it fires* — has a converse that was missing:
**do not report a mechanism as absent while a column shows it firing.**

Three refutations in three passes should have been the signal. Markov was refuted by its
registered falsifier on measured data. Terrain was argued out, not measured. Two-ray was
"refuted" by a regression in a number, which is the weakest of the three and was wrong.

### Where the residual stands

| tier | initial | + foliage | two-ray, bad heights | **two-ray, sourced heights + slant** |
|---|---|---|---|---|
| 868 MHz | 164.9% | 30.9% | 30.9% | **30.9%** |
| 2.4 GHz | 88.7% | 27.6% | 152.9% | **44.6%** |
| UWB | 140.8% | 59.2% | 113.5% | **113.3%** |

`r80` on 2.4 GHz and UWB is still worse than the post-foliage state, and the term was
kept anyway: the post-foliage path-loss law is now *measured* wrong by 4.49–7.77 dB
against `mean_pl_db`, and restoring a proven-wrong term because it flatters `r80` is
tuning to match. The base term is verified; the disagreement has moved somewhere else.

### Where it moved — measured, not implemented

Isolating the engine's non-path-loss loss as `X = tx − pl_eng − sens − margin_eng`:

| tier | X_engine near | X_engine far | model X | `nlos_frac` over that span |
|---|---|---|---|---|
| 2.4 GHz | 3.84 dB @ 10 m | 17.81 dB @ 6000 m | 3.9 → 5.7 dB | 0.000 → 0.111 |
| UWB | 4.27 dB @ 2 m | 9.77 dB @ 180 m | flat 3.92 dB | 0.000 → 0.004 |

A distance-growing excess on effectively-LOS links, 5.5 dB across two decades on UWB.
**No fourth term was implemented.** The base-term question that blocked it is now closed,
but one term per pass stands, and the next candidate must be pre-registered first.

## PDR transition width: measured, and it is not a single global knob

UWB's `r80` error (113.5%) is far larger than its worst margin deviation (5.79 dB) would
suggest. Transition width was the untested candidate for that gap. Measured, not adjusted.

**Definition, stated before the numbers.** Width = the distance span over which PDR falls
0.95 → 0.05 (linearly interpolated in PDR), converted to dB via the mean-margin slope
across that same span. Engine side: `results/probe_v7/probe_range_sweep.csv`. Model side:
`lb.pdr_at` with the 2000 corpus draws — *the function `validate.py` actually calls*.

| tier | engine width | model width | |
|---|---|---|---|
| 868 MHz | PDR only reaches 0.26 at the 6000 m sweep end — **never spans 0.95→0.05** | 45.60 dB | not measurable engine-side |
| 2.4 GHz | 18.80 dB | 21.71 dB | **model 1.15× wider** |
| UWB | 28.14 dB | 17.43 dB | **engine 1.61× wider** |

**The sign differs between tiers.** A single global width change cannot fix both; it would
improve UWB and worsen 2.4 GHz. Width is therefore not the missing term either, though on
UWB it is real and in the direction the hypothesis predicted.

### What it does explain: the r80-vs-margin inconsistency

The UWB curves nearly coincide at the bottom and diverge at the top:

| | engine | model | ratio |
|---|---|---|---|
| d @ PDR 0.95 | 6.2 m | 16.4 m | **2.65×** |
| d @ PDR 0.05 | 114.7 m | 121.9 m | 1.06× |

`r80` is sampled at PDR 0.80 — deep in the high-PDR region where the curves are 2.65×
apart — while the worst margin deviation is dominated by long range, where they nearly
agree. That is the whole of the "inconsistency": the two criteria sample opposite ends of
a curve that disagrees at one end and agrees at the other. `r80` is the harsher criterion
because it is sampled where the disagreement lives, which is what the earlier entry
concluded qualitatively and this measures.

### A measurement error caught before it was reported

The first attempt measured a model width of 3.98 dB for UWB — a 7× discrepancy that
looked like a decisive finding. It was an artefact of the probe: it called
`lb.rx_power_dbm` (`linkbudget.py:363`) directly, which applies no shadow fading, while the model's PDR comes
from `lb.pdr_at`, which adds `sd * u_shadow` (`linkbudget.py:440`; the shadow term is formed at `linkbudget.py:457` and consumed at `linkbudget.py:459-462`). The probe
measured a quantity the model does not use. Re-measured against `pdr_at` the width is
17.43 dB and the finding shrinks from 7× to 1.61×.

This is the same failure the file has now recorded five times: **a number computed over
the wrong population/estimator, checked only for plausibility.** It was caught only
because 3.98 dB against a 5.0 dB shadow sd is arithmetically impossible — the check that
worked was a physical bound, not a review of the code.

Engine `shadow_sd_eff_db` is 5.000–5.005 dB on the `RURAL_OPEN` tiers, matching the
model's `shadow_sd = 5.0` exactly, so shadowing is not the source of the width difference
on either side.

---

## G11 assertion 3 was passing on an empty population

The suite run reported:

    experiment sidecars claiming a corpus digest: 0, all matching: yes

Zero sidecars, and it printed **"all matching: yes"**. The assertion verified nothing and
said it was satisfied. The population is legitimately empty right now — the sweeps that
write these sidecars are blocked behind assertion 2 — but "nothing to check" was being
rendered as "checked, and consistent". That is the difference between a gate that is
waiting and a gate that is asleep, and from the output they were indistinguishable.

Two further population leaks in the same function, both silent:

- `except Exception: continue` — an unparseable sidecar was skipped without a word,
  shrinking the population invisibly. A corrupt sidecar is now a **FAIL**.
- A matched file carrying no digest key was also skipped silently. Those are now listed
  by name, so the gap between "files matched" and "files checked" is always visible.

The globs are now printed, because **a glob is a population claim** and an unprinted one
cannot be audited.

### Fixture, tested in four directions

Built as a throwaway git repo in `/tmp` with the module files actually tracked — the
first attempt used a plain directory, where the assertion's tracked-files half set
`ok = False` before the digest check ran, so its "positive" case failed for an unrelated
reason and proved nothing. That is the fixture being wrong, not the gate.

| direction | expected | observed |
|---|---|---|
| no sidecars | PASS, reported **VACUOUS** | PASS, "verified NOTHING" |
| correct digest | PASS, `n = 1`, no longer vacuous | PASS, "1, all matching: yes" |
| wrong digest | FAIL with the mismatch named | FAIL, mismatch named |
| unparseable sidecar | FAIL, population shrink named | FAIL, shrink named |

None of the four fail by crashing.

### A defect report withdrawn before it was made

The suite printed `=== SUITE FAILED ===` and the shell reported exit 0, which looks like a
gate suite that cannot fail a caller. It is not: `catbl_gates.sh:353` is `exit 1`, and the
0 came from `tail` at the end of my own pipeline. Worth stating because a CI harness that
pipes the suite through `tail` or `tee` without `PIPESTATUS` **would** read green on a red
suite — the script is correct and the invocation is the hazard.

---

## The model is a 4-term budget against a 14-term engine

Written as grounding for the next pass, not acted on in this one. `X` was treated
throughout as "the missing term", singular. `lora-physics-engine.cc:597-599` shows the
engine's actual loss sum, and the singular framing is wrong:

    additional_loss = positiveShadowing + result.fadingDb + result.nlosDb +
                      result.foliageDb + result.terrainDb + result.dopplerPenaltyDb +
                      result.interferencePenaltyDb + multipathPenaltyDb +
                      thermalEfficiencyLossDb + tier2LossDb + tier3LossDb +
                      result.atmosphericDb + state.propagationLossBiasDb
    totalLoss = result.pathLossDb + additional_loss + negativeShadowingGain
              + result.hwTxVariationDb - result.antennaGainDb

The model has four of these: path loss, NLOS, foliage, shadowing. Terms it has **no
counterpart for at all**: `fadingDb`, `terrainDb`, `dopplerPenaltyDb`,
`interferencePenaltyDb`, `thermalEfficiencyLossDb`, `atmosphericDb`,
`propagationLossBiasDb`, `hwTxVariationDb`. `multipathPenaltyDb` is set to 0 at `lora-physics-engine.cc:596`
and `soilAttenuationDb` at `lora-physics-engine.cc:593`, so those two are genuinely absent from the engine here.

Three passes have asked "which term is missing" as though the answer were one term. The
residual may be a sum, and a pass that adds one term and measures a partial close should
not read that as a partial refutation.

### One candidate eliminated for free

`terrainDb = std::clamp(terrainPenaltyDb, 0.0, 30.0)` (`lora-physics-engine.cc:400`) is **non-negative**, so
its mean is strictly positive; it is Fresnel/diffraction-derived (`lora-terrain-submodule.cc:141-191`)
so it grows with distance; and it saturates at 30 dB. That shape matches the measured `X`
on both `RURAL_OPEN` tiers, including 2.4 GHz flattening near 17.8 dB. It is the leading
candidate and it is **not yet pre-registered or implemented.**

The shadowing clamp is asymmetric — `[-2*sd, +3*sd]` (`lora-physics-engine.cc:417-418`) —
which rectifies the draw and gives it a positive mean. Quantified analytically before
being pursued: the shift is `0.0081*sd`, i.e. **+0.041 dB** at `sd = 5` and +0.057 dB at
`sd = 7`, against a residual of 5.5–14 dB. **Eliminated**, at the cost of one integral
rather than an implementation pass. Recording it so it is not re-proposed.

---

## G7's heading claims a population wider than the one it checks

Observed during the G1-G13 run, not acted on. The gate prints
`--- G7 suite left the tree clean ---`, but its population is

    git status --porcelain --untracked-files=no -- results/

— tracked files under `results/` only. The scoping is deliberate and the comment at
`catbl_gates.sh:216-219` explains why (`results/` always holds untracked campaign
output). The gate's *output line* is honest: it prints the path it checked. Only the
heading overstates it, by saying "the tree" where it means "tracked files in results/".

That mattered on this run: the tree carried modifications to `src/` and `scripts/`
throughout, and G7 passed. Correctly — they are outside its population and were not made
by the suite — but a reader taking the heading at face value would conclude the suite had
verified something it never looked at.

**Deliberately not fixed in this pass.** Editing a gate script after the run that
verified it, without re-running the suite, is the failure mode this project spent two
campaigns removing. It is a one-line heading change and belongs in a pass that can
re-run G1-G13 behind it.

---

## An instruction that would have preferred a wrong law with a better number

Recorded because the next such instruction will come from the same place, and because the
only reason it was caught is that declining it was stated rather than done quietly.

The instruction was: revert the two-ray change pending diagnosis, and land the corrected
version only "if the corrected implementation improves on the post-foliage numbers". It
was written before the outcome was known, and the outcome it did not anticipate arrived:
a **verified-correct path-loss law that scores worse on `r80`**.

| | post-foliage law | corrected two-ray law |
|---|---|---|
| worst error vs engine `mean_pl_db` | 4.49 dB (UWB), 7.77 dB (2.4 GHz) | **0.02 dB**, **0.96 dB** |
| `r80` relative error | 59.2%, 27.6% | 113.3%, 44.6% |

Applied literally, the instruction would have restored a term **measured wrong by
4.49–7.77 dB against the engine's own column** in order to improve a derived metric. That
is fitting the metric — the failure this project has spent every pass avoiding.

### The rule that resolves it

**`r80` is derived; the margin against the engine's own column is direct. When they
disagree about which version is better, the direct measurement wins.**

`r80` sits downstream of every excess term in the budget. A base term can be exactly right
and still produce a bad `r80` if terms *after* it are missing — which is precisely the
situation here, and precisely why `r80` could not be used to judge the base term. Comparing
against `mean_pl_db` removes everything downstream from between the claim and the check.

### Why it is recorded here rather than settled quietly

The decision was flagged with its reasoning and a one-command undo rather than buried in a
commit body. That is what made it reviewable, and the review reversed the instruction
rather than the work. An instruction declined silently — even correctly — leaves no such
opportunity. **Flag an instruction you decline, with the reasoning**, is now a standing
rule and this is its first recorded instance.

---

## THE TERMINUS: the registered §7 criterion cannot be met, and is reported failed

Not "has not been met yet". **Cannot be**, by an independently-implemented model.

The registered criterion is `r80` within 15% and margin within 3.0 dB across the swept
range, for full reconciliation. Meeting it requires every active loss term. One of those
terms — `terrainDb`, the largest remaining at ~13 dB on 2.4 GHz and ~5.5 dB on UWB — is
transcription-only: its active path is `proceduralLoss`, whose input is
`GetProceduralElevation`, three hardcoded fBm octaves over a bespoke `SmoothNoise`
(`lora-terrain-submodule.cc`). Reproducing it means copying it, and a transcription cannot
validate its source.

**The criterion is NOT rescoped.** It stands at 15% and 3.0 dB, it fails, and the cause is
named. A criterion relaxed after seeing why it fails is worthless, and this project has
refused that move four times. What is legitimate is registering a *new and separate*
criterion for a *different* question, prospectively — see
`experiments/G11A_REGISTRATION.md`.

### G11a as literally specified is not computable, and importing terrain would be circular

The proposal was to compare `model + engine-reported terrain` against the engine margin,
with terrain supplied from the engine's own column. **There is no terrain column.**
`results/probe_v7/probe_range_sweep.csv` has fifteen columns and none of them is terrain,
and no new ns-3 runs are authorised.

The only way to obtain terrain from existing output is as the residual
`X_engine − X_model`. Defining terrain as whatever closes the gap and then reporting that
the gap closes is **circular by construction**: G11a would pass at exactly 0.00 dB on
every tier and every distance, and would mean nothing. That is the same failure as the
original `validate.py`, which consumed the quantity it validated.

**Declined, with the reasoning, and replaced by something that measures.** See the
registration for the substitute: validate on the sub-population where terrain is provably
inactive **by geometry**, so no terrain value is imported at all.

---

## 868's negative near-field residual — one bounded look, partly identified, recorded OPEN

The model is **3.94 dB too lossy** at 100 m on 868 MHz, decaying with distance and
reversing sign to +0.55 dB by 6 km. Opposite in sign to every other discrepancy. Terrain is
provably inactive below 152 m there, so it is not terrain.

### Identified: a real defect worth 0.466 dB

**The model evaluates ITU-R P.833 at the mean of `gamma` and `A_max`; the engine samples
both.** `lora-physics-parameters.cc:318-319`:

    specificAttenuationDbPerM = Normal(gamma, gamma*0.25, gamma*0.2, gamma*1.8)
    maxAttenuationDb          = Normal(amax,  amax*0.2,   amax*0.5,  amax*1.8)

`A_veg = A_max·(1 − exp(−depth·gamma/A_max))` saturates, so by Jensen
`E[A_veg] < A_veg(E[gamma], E[A_max])` and the model over-attenuates:

| vegetation | model, at the mean | engine, sampled | model high by |
|---|---|---|---|
| `VEG_DENSE_FOREST` | 16.424 dB | 15.958 dB | **+0.466 dB** |
| `VEG_SPARSE_SHRUBS` | 3.917 dB | 3.827 dB | +0.090 dB |

Same class as the foliage-depth error this file already records — a saturating function
evaluated at a point estimate instead of integrated over its distribution. **Not fixed in
this pass**: it is a change to a term already scored, and it needs its own registration.
Recorded with its magnitude so it is not rediscovered.

### Ruled out, each with its number

| candidate | ruled out because |
|---|---|
| the fading NLOS branch | `nlos_frac` is 0.0130 at 100 m, so the registered omission is worth **0.011 dB** — three orders short |
| foliage `gamma`, `A_max` | identical to the engine: 0.20 and 25.0 (`lora-physics-parameters.cc:309`) |
| foliage depth distribution | identical: `LogNormal(log(100), 0.6, 0, 500)` (`lora-physics-parameters.cc:293`) |
| terrain | provably inactive below 152 m by the geometric bound |
| link hysteresis | already corrected for in the re-census |
| two-ray | gated off in `RURAL_FORESTED`; `two_ray_frac` is 0.0000 at every 868 distance |

**≈3.5 dB remains OPEN.** One observation for whoever picks it up: the discrepancy shrinks
monotonically as `nlos_frac` rises (−3.94 at 0.013, −3.01 at 0.111, −0.47 at 0.315, +0.43
at 0.418), which is the signature of **two errors of opposite sign partially cancelling** —
foliage too large and NLOS excess too small — rather than one term being wrong. No fifth
term hunt was started.

---

## The fault injector's comment describes a model it does not implement

`ca_tbl_causal_sim.cc:2544-2549` states:

> `injectFaultAnchors = 0` means COMMON-MODE (every tier-2 link, h = m). **`>0` means a
> SINGLE-ANCHOR fault (h = 1), which is the canonical RAIM model** and the physically
> realistic one.

The selector two lines below is `(anchorId % 97) % 3 == seed % 3`. Over the deployment's
**12 tier-2 anchors** that faults **exactly 4**, deterministically, at every seed tested.

| label | comment claims | actually injects |
|---|---|---|
| F1 (`injectFaultAnchors > 0`) | `h = 1`, canonical RAIM | **`h = 4` of 12** |
| F3 (`injectFaultAnchors = 0`) | `h = m`, common-mode | `h = 12` of 12 — correct |

**The canonical single-fault condition is bracketed and never met.** Fault Detection and
Exclusion is derived for exactly one faulty measurement; the project's two fault models sit
at a third of the anchors and at all of them.

### What this does and does not change

**It does not change any result.** The numbers were produced under the model the code
implements, not the model the comment describes, and they stand exactly as recorded. Claim
9's verdict is unaffected: a fault model that turns out to be *harder* than documented does
not make a negative result softer.

**It changes the description.** Every campaign using this injector — `campaign_20260810`,
`campaign_recovery`, `c9_clean`, `abl_clean`, `excl_test` — tested `h = 4` and `h = 12`, not
`h = 1` and `h = m`. `Simulation.md`'s ledger says "2 fault models" without characterising
them, so no ledger wording is wrong; what was missing is the characterisation itself, now
added.

**The injector is NOT changed.** It is the model the corpus was measured under, and editing
it in place would invalidate every result in the tree. `--faultMultiplicity` is added
beside it, default `-1`, preserving the existing selector bit-exactly — verified by a
byte-identity null control against the committed `c9_clean/F1_seed1`.

Found by the exclusion campaign: `exclude` helped at `h = 4` and harmed at `h = 12`, which
made the multiplicity the governing variable and sent me to read the selector.

---

## A checker that reads part of a citation is measuring its own parser

`doc_audit.py` extracted 137 `file:line` citations from the tier-1 documents and read
**only the first line of each**. Measured against its own regex: **62 are single-line and
75 are ranges**, spanning 2 to 51 lines, mean 6.2. **388 cited lines were never read.**

The consequence was a reported statistic about the documents that was a statistic about the
tool. The token-overlap split was published as `no-overlap 71 / within-3 34 / in-line 32`;
after the range fix it is `65 / 21 / 51`. Those first numbers described the extractor.

**A parser's coverage of the citation grammar is a population claim.** Enumerated here
before any count derived from it was trusted:

| form | count | handled |
|---|---|---|
| `file.cc:N` single | 62 | yes |
| `file.cc:N-M` range | 75 | yes, after the fix |
| `` `:N` `` bare continuation | 137 occurrences | **no — excluded, needs file context from a preceding citation** |
| `file.md` bare | 67 | excluded — G10's population |
| `file.md §N` section | 13 | excluded — no line to verify |
| `Identifier()` symbol | 4 | excluded — no line cited |
| `` `results/x` `` | 73 | excluded — G10's population |

Every exclusion is stated with a reason rather than left as a silent gap.

## Withdrawing a hand-check withdraws everything built on it — third instance

The token heuristic was scoped out of gating on the strength of one hand-check: that
`Architecture.md` cites `lora-terrain-submodule.cc:69` for "terrain hardcodes 868 MHz" and
line 69 is `double frequencyHz = 868e6;`. **Line 69 is blank. The constant is at line 70.**
The hand-check mis-indexed `sed` output, and the heuristic had been right that line 69 alone
does not support the claim.

The scoping decision was then carried forward as "still stands, for a better reason". That
is the move this file has already recorded twice — **an argument whose reasoning is void
does not become sound because its conclusion looks right** (W8, W10). Third instance.

**Re-derived properly**: the FP threshold was fixed *before* measuring (< 20% gate,
20–60% triage, > 60% scoped out), 20 of the 65 flags were drawn at random with a stated
seed, and each was hand-adjudicated. **13 of 20 supported their claim: FP = 65%.** The
scoping decision stands, now on evidence. The 95% CI is [0.44, 0.86] and straddles the
triage band, which is reported rather than rounded away.

**Method fix, made mechanical:** every line quoted in an audit carries a tool-emitted line
number — `awk 'NR>=a && NR<=b {printf "%d: %s\n", NR, $0}'`, never bare `sed -n 'a,bp'`.
A transcribed line number is an unverified claim in a document whose purpose is verifying
claims.

### Citation drift found by the sample, and corrected

7 of the 20 sampled citations did not support their claim — extrapolating to roughly 23
across the 65 flagged. Corrected here against tool-emitted line numbers:

| document claim | was | is |
|---|---|---|
| the injector's `h = 1` comment | `ca_tbl_causal_sim.cc:2496-2501` | **`ca_tbl_causal_sim.cc:2544-2549`** |
| `vegWide = VEG_DENSE_FOREST` | `ca_tbl_causal_sim.cc:3174` | **`ca_tbl_causal_sim.cc:3257`** |
| the D1 fix | `lora-2p4/uwb-physics-engine.cc:154` | **`:155`** (154 is its comment) |

The first is the clearest argument for pinning adjudications to content hashes: **it was
written in commit `a3b468e`, in the same commit that inserted ~40 lines above it**, and was
invalidated by my own edit before it was ever committed.

Four further drifted citations were recorded as found and are now **corrected**. Each was
located by grepping for the claim's own content rather than by searching near the stale line
number, because a drifted anchor carries no information about where its target went:

| claim | stale anchor (what is actually there) | correct anchor |
|---|---|---|
| tier→environment mapping | `ca_tbl_causal_sim.cc:3143-3144` — a comment about beacon teardown | `ca_tbl_causal_sim.cc:1040`, `ca_tbl_causal_sim.cc:1048`, `ca_tbl_causal_sim.cc:1056` bind the mapping (`c868`←`envWide`, `c2p4`/`cUwb`←`envFacility`); `ca_tbl_causal_sim.cc:3226-3227` set the values |
| the `0.0` sentinel | `ca_tbl_causal_sim.cc:1413` — the `if (received)` guard | `ca_tbl_causal_sim.cc:1437-1438`, the `meanSnr` ternary whose false branch is now `quiet_NaN()` |
| floor discard — selection, not fusion | `ca_tbl_causal_sim.cc:2494-2496` — the `tier == CA_TBL_TIER_868` dispatch | `ca_tbl_causal_sim.cc:2713-2714` decides `usedFloor`; `ca_tbl_causal_sim.cc:2723` performs the selection |
| the trigger the arms differ at | `ca-tbl-policy.cc:877` — inside a comment about the LOS-fraction cut-off | `ca-tbl-policy.cc:930-931`, `mahal > m_config.mahalanobisThreshold && allLos` |

Distance from the stale anchor to the correct one: **83** lines (3143→3226, and 2100 lines to
the macro that actually binds the mapping), **24** (1413→1437), **219** (2494→2713), **53**
(877→930). Three of the four are close enough that a reader checking the neighbourhood would
have found the right code — and none was caught that way, because **the stale line itself read
plausibly**: a `tier ==` dispatch, an `if (received)` guard, a comment about the very parameter
under discussion. A citation is not verified by the cited line being sensible. It is verified by
the cited line containing the claim.

---

## The continuation form is not mechanically resolvable in this corpus — with a counterexample

The bare `` `:N` `` continuation was excluded from the citation audit as an unhandled form.
That exclusion was challenged on the grounds that it *is* mechanically resolvable, unlike a
`§N` section reference. The challenge was right to demand a test and the test settles it the
other way.

**Population, now measured rather than excluded:** 145 bare-`:N` citations across the
tier-1 documents — comparable to the 137 `file:N` citations the audit already covers. So
the audit's population was roughly **half** the citations, and the omitted half carries the
"four terms zeroed on the line after the line computing them" finding.

**Rule implemented and fixture-tested in four directions:** bind to the nearest preceding
file reference in the same table row or line; fall back to the enclosing paragraph; never
cross a blank line or a list-item boundary; report an orphan as a document defect. The
fixture passes all four, including the cross-boundary negative.

**Applied to the corpus it resolves 41 of 145 and declines 104.** Spot-checking the
declines shows they are **not document defects** — the documents use a *section-scoped*
convention: a file is named once and then referenced by bare `:N` throughout the section.

**Broadening the rule to "nearest preceding file anywhere above" binds wrong.** The
counterexample, with tool-emitted line numbers:

    REPORT_1_Propagation_Module.md
      273: | 868 MHz | `-123.0` dBm | `lora-physics-parameters.cc:391` | ...
      274: | 2.4 GHz | `-106.0` dBm | `lora-2p4-physics-parameters.cc:14` | ...
      275: | UWB     | `-104.0` dBm | `lora-uwb-physics-parameters.cc:14` | ...
      ...
      283: | Path-loss exponent `n` | 2.2 | 2.3 ... | `:24`, `:27` |

`:24` and `:27` mean `lora-physics-parameters.cc`, and that file's lines 24 and 27 are
`Fixed(2.2)` for `ENV_RURAL_OPEN` and `Normal(2.3, 0.1, 2.1, 2.6)` for
`ENV_RURAL_FORESTED` — **the citations are correct.** But the nearest preceding file
reference is `lora-uwb-physics-parameters.cc` at line 275. Nearest-preceding resolves them
to the wrong file, confidently, and would then compare the claim against lines 24 and 27 of
a file the document never meant.

**Verdict: the exclusion stands, and now has evidence.** A resolution rule that guesses
wrong is worse than one that declines, and this corpus contains a case where the obvious
rule guesses wrong. Resolving these requires section-scope tracking that models "which file
is under discussion", which is prose comprehension, not parsing.

**What is legitimate instead:** the 41 that resolve within line or paragraph scope are
audited. The 104 are recorded as **outside the audit's mechanical reach**, with their count
and the reason, rather than as a silent gap or as 104 false defects.

### Third instance this pass of a tool limitation nearly reported as a document finding

1. The extractor read one line of every range; the 71/34/32 split described the extractor.
2. The token heuristic flagged 65 citations; 65% of a sample were the tool's fault.
3. This rule declined 104 continuations; spot-checking shows they are correct citations.

Each time the number looked like a document defect rate and was a tool property. The
standing rule this earns: **before reporting a count as a property of the subject, check
whether it is a property of the instrument** — and the check is to hand-adjudicate a random
sample, which caught all three.

---

## A status check must not run after the operation that changes the status

`git ls-files --error-unmatch <path>` was used to establish whether the two reports were
tracked. It reported **"tracked"** for `REPORT_1_Propagation_Module.md`, which is not in
`HEAD`. The check had been run **after** `git add`, and `git ls-files` reads the **index** —
which the `git add` had just populated. **The check consumed its own side effect.**

The authoritative test reads the commit graph and cannot be contaminated by staging:

    git cat-file -e HEAD:<path>

Against that, both reports came back `NOT IN HEAD`, which is how the finding was actually
established.

Same family as the instrument-versus-subject rule above: an instrument that is downstream of
the thing it measures reports the measurement back to you.

### Other index-reading status checks, grepped rather than recalled

`git ls-files` appears at `catbl_gates.sh:237` (G8), `catbl_gates.sh:297` (G9), `publication_ready.sh:50`,
and in `citation_audit.py`'s TRACKED category. **All four are correct as written**, because
each runs in a gate suite that stages nothing — the suite asserts a clean tree at G7 and
does its regeneration in `mktemp -d`. The defect was the *manual* check run mid-session
after a `git add`, not the gates.

Recorded anyway, because "correct today because nothing stages during the run" is a property
of the caller, not of the check, and the next caller may not have it.

## G9 asserted against a hardcoded list, so it could not see what was missing

G9 asserts *named deliverables are tracked and committed*. It passed while
`REPORT_1_Propagation_Module.md` (1354 lines) and `REPORT_2_Project_Retrospective.md`
(957 lines) — tier-1 material, and the citation audit's own subject — were absent from the
repository. They were not on its list.

**Third instance in this audit of a check succeeding because its population excludes what it
should have caught**, after `TRACKED_ELSEWHERE` passing a citation that does not resolve
from the repository root, and an empty sidecar population reporting "all matching: yes".

**Inverted as G9b:** every `.md` in the working tree must be tracked, except an enumerated
allow-list parsed from `EXCLUDED.md`'s `reviewed-untracked-md` block. A must-be-tracked list
goes stale in silence; a must-not-be-tracked list fails loudly the moment something new
appears.

The allow-list was built from the **measured** population — 148 untracked `.md` files,
grouped — not from recall: four pattern classes with reasons (work orders and replies,
G8-enforced handoffs, vendored and archived trees, campaign-local `results/**/*.md`), plus
**39 legacy files enumerated individually**. Individually, deliberately: a `CA_TBL_*`
pattern would let a *new* planning document inherit an exemption, and the fixture tests
exactly that case.

Fixture, four directions, all passing: the real tree passes; a new unlisted `.md` trips it;
a new `CA_TBL_*` file does **not** inherit an exemption; and an allow-listed file that
becomes tracked is caught by G8, which is this rule's mirror.

---

## A citation that lands on a `case` label or a blank line is a candidate for off-by-one — 17 of 200 were

**Origin.** Expanding the environment table in `REPORT_1_Propagation_Module.md` found **twelve
off-by-one citations in fifteen**, every one the same shape: the citation pointed at the
`case ENV_RURAL_OPEN:` label and the value it claimed to cite was on the *next* line. That is a
mechanically detectable shape, so it should be checked in one pass rather than found one at a
time. `scripts/case_label_check.py` does that.

**Population, stated before the result.** 311 citations over the 14 tier-1 documents, in two
disjoint sets that are not equally knowable:

- **200 qualified** — `path.cc:N` or `path.cc:N-M`. Fully mechanical.
- **111 continuation** — a bare `` `:N` `` whose file is carried over from earlier prose. This
  form is not mechanically resolvable in this corpus (see the section above). The tool resolves
  each to the nearest preceding qualified citation, labels the result HEURISTIC, and reports
  every unresolvable one by name rather than dropping it.

**Yield: 17 qualified hits, in three classes.**

| class | count | verdict |
|---|---|---|
| leading blank line — `lora-terrain-submodule.cc:69-70`, where 69 is blank and 70 is `double frequencyHz = 868e6;` | 7 | defect. Repointed to `lora-terrain-submodule.cc:70-71`: 70 sets the frequency, 71 derives the λ every claim goes on to use |
| leading blank line — `lora-physics-engine.cc:597-601` and `lora-physics-engine.cc:597-599`, where 597 is blank | 5 | defect. The thirteen-term sum is **`lora-physics-engine.cc:598-600`**. `lora-physics-engine.cc:597-599` was worse than off-by-one: it ends before the sum does, omitting five of the thirteen terms it is cited to enumerate |
| opens on a `case` label — `lora-physics-parameters.cc:304-309` | 5 | **not** off-by-one; the range legitimately opens on a label. But it covers three of the **seven** vegetation cases in a switch spanning `lora-physics-parameters.cc:300-316`, while the claim is "γ, A_max *per vegetation type*". Truncated range |

**A hit is not automatically a defect, and the class that proves it is the third one.** A range
citation may correctly open on a `case` label. Had the tool gated on this shape it would have
been wrong five times in seventeen. It flags candidates; a person returns the verdict. That
division is the same one `doc_audit.py` draws and for the same reason.

**Two kinds of citation that are correct while flagged, and are deliberately left alone.**

1. **Citation-as-quotation.** `DEFECTS.md:1580` cites `lora-terrain-submodule.cc:69` — a blank
   line — because it is *quoting the historical defective citation* as the evidence for a
   finding. Repointing it would destroy the record it exists to keep. Verification must not
   mutate its subject.
2. **Pre-registration documents.** A document that fixes predictions, intervals and
   falsifiers *before* a run is worth what it is worth because it was written in advance.
   Editing a citation in one afterwards is a retrospective change to the thing whose value is
   that it was not changed. `FOLIAGE_PREREGISTRATION.md`, `TWORAY_PREREGISTRATION.md`,
   `G11A_REGISTRATION.md`, `TERRAIN_SENSITIVITY_REGISTRATION.md`,
   `JENSEN_FOLIAGE_REGISTRATION.md` and `OFAT_CAMPAIGN_REGISTRATION.md` are that. Their stale
   citations are recorded here instead.

   **`LOSS_TERM_CENSUS.md` is not one of them, and calling it one was a classification error
   in the first draft of this section.** It lives in `experiments/` and opens "written before
   implementing anything further", but it registers no prediction and nominates no falsifier —
   it is a census of what the code contains, which is a claim about source that can be right or
   wrong today. It was therefore expanded and corrected with the living deliverables. The rule
   is what the document *does*, not which directory it sits in.

So corrections were applied to the **living deliverables** — `Architecture.md`,
`Simulation.md`, `REPORT_1_Propagation_Module.md`, `REPORT_2_Project_Retrospective.md`,
`LOSS_TERM_CENSUS.md` — and to site pointers in this log that are claims about current code
rather than quotations. Every hit that survives the check is in one of the two protected
classes; the count rises as this section is written, because the section quotes the stale
ranges as its own subject matter.

**A defect introduced while fixing defects.** The correction table above was first written with
its "correct anchor" column in the bare continuation form — thirteen new `` `:N` `` citations
added to a corpus whose open task is eliminating one hundred and eleven of them. Caught by
re-running the census after the edit rather than before it, and by the census reporting a
population, not a pass. **A count taken only before the change cannot see what the change added.**

---

## The 111 continuations, expanded: 8 drifts, and a carry-over heuristic that is wrong 42% of the time

**What was done.** Every bare `` `:N` `` continuation in the tier-1 corpus was expanded into a
fully-qualified `path:line` citation, each one **verified against the source line before it was
written**, not after. 101 of 111 were expanded. The 10 that remain are in the two protected
classes above: five quote a historical defective citation as their own evidence, five are in
pre-registration documents.

**The registered prediction, scored.** Registered before the work: *30–40 drifts across the
145*. Measured:

| pass | expanded | drifted | rate |
|---|---|---|---|
| environment + census tables (earlier) | 39 | **12** | 31% |
| everything else (this pass) | 101 | **8** | 8% |
| **total** | **140** | **20** | **14%** |

**The prediction is not met — 20 against a registered floor of 30 — and the reason is the
concentration.** Twelve of the twenty drifts are in a single fifteen-row table, where one
systematic cause was operating: the citation pointed at a `case` label and the value was on the
next line. Extrapolating that table's 31% rate across the corpus was the error. Outside it the
rate is 8 in 125, or 6.4%. **A rate measured where a systematic cause is active does not
generalise to a population where it is not** — and the registered interval was set by doing
exactly that.

The interval is left as registered. It was wrong in the direction that costs nothing: it
predicted more defects than existed.

**The eight drifts of this pass**, every one found because the claim's own content was grepped
rather than the neighbourhood of the stale line read:

| document | stale | correct | what was actually at the stale line |
|---|---|---|---|
| `REPORT_1` | `:154` | `lora-2p4-physics-engine.cc:155`, `lora-uwb-physics-engine.cc:155` | the assignment's comment |
| `REPORT_1`, `DEFECTS.md` | `:1376-1380` | `ca_tbl_causal_sim.cc:1391-1395` | a UWB `ProcessTransmission` call |
| `REPORT_1` | `:1450-1452` | `ca_tbl_causal_sim.cc:1476-1480` | the shadow-sd computation, not the CRLB gate |
| `DEFECTS.md` | `:2024-2037`, `:2064` | `ca_tbl_causal_sim.cc:2137-2144`, `ca_tbl_causal_sim.cc:2175-2179` | a comment block; and off by one |
| `DEFECTS.md` | `:259-261` | `linkbudget.py:457`, `linkbudget.py:459-462` | the k-quadrature loop |
| `LOSS_TERM_CENSUS.md` | `:18-19` | `lora-atmospheric-submodule.cc:60` | a closing brace and a blank line |

**The measurement that settles the form.** The open question was whether the continuation form
could be resolved mechanically instead of by hand. It can be tested directly: for each expanded
citation whose *line number* was correct, compare the file the nearest-preceding-qualified-
citation heuristic would have chosen against the file verification actually established.

| form | n | right file | **wrong file** |
|---|---|---|---|
| truly bare — `` `:N` `` with nothing but prose before it | 69 | 40 | **29 (42%)** |
| alias-bearing — `engine `:585-586``, `params `:471`` | 24 | 2 | **22 (92%)** |

**A tool built on the carry-over heuristic would have misattributed two in five.** That is not
a tool with a bug; that is the form carrying less information than resolution requires. The
alias-bearing row is the same point from the other side: those citations *are* resolvable, but
by the English word next to them — `engine`, `params`, `fading-submodule` — which the heuristic
does not read and which no two documents in this corpus spell the same way.

The 42% is why the expansion was done by hand against tool-emitted source lines, one citation
at a time, and why the ten that remain are documented rather than guessed.

**This section is itself the exception G15 will need.** Nine bare `` `:N` `` citations remain in
it and cannot be expanded: seven quote a stale citation as the evidence for a correction, and
two are specimens of the alias form being discussed. A rule reading "no bare `:N` in a tier-1
document" would fail on the document that explains why bare `:N` is a defect. The exemption
must be **enumerated at the file and the reason**, never a pattern over `DEFECTS.md`, or the
next bare citation written here inherits it silently — the same failure mode that made G9's
hardcoded allow-list useless.

---

## The vendor-name rule was not violated in the repository — and nothing was checking

**The premise, tested.** 23 `.md` files in this tree contain a vendor name, several with
project-sounding names — advisor handoffs, publish runbooks, next-steps plans. The working
assumption was that some are tracked, which would be a live violation of a hard rule.

**Tracked status established with `git cat-file -e HEAD:<path>`**, not `git ls-files`, because
`ls-files` reports the index and would say "tracked" for a file that was merely staged:

| population | n | tracked |
|---|---|---|
| `.md` files containing a vendor name | 23 | **0** |
| all tracked files, any type, scanned for the same | 7129 | **1** |

The single tracked hit is `src/visualizer/visualizer/core.py`, and it is `Gdk.Cursor` in
vendored upstream ns-3.47 source — a false positive, and a useful one, because it is exactly
the shape a gate matching bare tokens would have failed on.

**So the rule holds, and the reason it holds is that nobody tracked those files, not that
anything prevented it.** That distinction is the whole finding. G12 gates commit identity
fields and message bodies; it has never been able to see a vendor name sitting inside a tracked
document. And **G9b now requires every `.md` to be tracked or allow-listed**, which makes a file
crossing from allow-listed to tracked an ordinary event. The protection was an accident of
housekeeping, and the housekeeping rule just changed.

**G12b** closes it: no tracked file's *content* may carry a vendor identity.

**Form, not token — for the reason G12 already records.** G12's first version failed on the
commit that documented the policy, because it matched a topic word. G12b matches an identity:
the token must stand alone, not preceded or followed by a dot or a word character. `Gdk.Cursor`,
a variable named `vendorThreshold`, a field `x.openai_compat` — none asserts authorship, and a
gate that fails on them is a gate someone turns off.

**The gate cannot scan itself**, because it must contain the pattern in order to be the pattern.
Excluding it by path would leave exactly one tracked file where prose could sit unexamined, so
the exemption is paid for by a self-check — and the self-check is a **form rule, not a pinned
count**: every matching line in the gate file must *be* a pattern definition. A count would have
to be edited whenever a matcher is legitimately added, and an edit-to-pass is not a check.

**The fixture found a defect in the gate on its first run, in the direction that mattered.**
The content scan was `grep -IqE` — case-sensitive — while the pattern is lower case and a
document naming a vendor capitalises it. **G12b would have passed the exact file it exists to
catch.** It passed the real tree, it passed the token-inside-an-identifier case, and it reported
"tracked contents clean across 7129 files" — a clean number from a matcher that could not have
matched. Two further fixture defects were mine and not the gate's: the probe self-checked its own
extracted copy rather than the repository's gate file, and direction 1 piped the suite into
`grep -q`, whose early exit sends SIGPIPE and turns a passing gate into a failing pipeline under
`pipefail`.

Four directions, all passing: the real tree passes; a tracked file asserting vendor authorship
fails; prose smuggled into the gate file the scan exempts fails via the self-check; a vendor
token inside a larger identifier passes.

---

## Deferred, on purpose, with the reason recorded

The citation audit's value has been realised: the population is known (323 citations over 14
tier-1 documents), the drift is found and corrected, and the expansion is done. What remains is
the **durability layer** — machinery that keeps the corpus correct rather than machinery that
made it correct. It does not block the novelty work, and the novelty work is the project's
remaining risk.

| deferred | why it is safe to defer |
|---|---|
| the adjudication ledger over 323 citations, with content hashes | the adjudication has happened; the ledger records it durably. Nothing downstream reads it yet |
| G14 (ledger gate) | gates the ledger; there is no ledger |
| G15 (no bare continuations) | 19 bare continuations remain, all documented and all in the two protected classes. G15 needs an enumerated exception list that does not yet exist |
| the 13 remaining `TRACKED_ELSEWHERE` adjudications, and tightening G10 | G10's two known-bad entries are closed; the remainder are known and enumerated |
| tier-2 verification | tier 1 is verified and tier 3 carries supersession headers |

This is a decision, not an omission. If it is never picked up, the cost is stated above: the
corpus is correct today and nothing keeps it correct automatically.

---

## A constant's comment is not a measurement: the fifth instance, and the one that reversed a conclusion twice

**The class.** A comment asserts a property the code does not have, or no longer has, and a later
reader takes it as evidence. This tree now has five recorded instances:

| # | site | the comment said | the code did |
|---|---|---|---|
| 1 | `ca_tbl_causal_sim.cc:2545-2549` | the injector faults a single anchor, `h = 1` | faulted **4 of 12** tier-2 anchors |
| 2 | environment table, `REPORT_1` §5.5 | citations point at the value | pointed at the `case` label, value on the next line — **12 of 15** |
| 3 | `two_ray_frac` scope | read as a distance diagnostic | a **gating** diagnostic: `ENV_RURAL_OPEN && !isNLOS` |
| 4 | `lora-terrain-submodule.cc:70` | terrain scales with frequency | hardcodes 868 MHz for every tier |
| 5 | `ca_tbl_causal_sim.cc:3251` | `// UWB r80 = 10 m` | **see below** |

**The fifth is different in kind, and worse, because the comment was right.**

Sizing the tier-3 calibration geometry needed UWB's r80. Taken from the constant's own comment —
`uwbTriadRadiusM = 10.0; // UWB r80 = 10 m` — none of the ten intra-site anchor pairs is in range,
so tier-3 self-calibration looked infeasible. Computing r80 from the analytic model instead gave
**25.2 m**, under which all ten are in range. That was written up as a stale comment corrected.

**Grepping every r80 in the tree — rather than trusting the correction — found four figures, and
the 10 m was not stale:**

    ca_tbl_causal_sim.cc:220    12.9 m   64-site decorrelated probe, 2000 trials/point
    ca_tbl_causal_sim.cc:3278   ~13.5 m  looser restatement of the same
    ca_tbl_causal_sim.cc:3251   10 m     the siting constant, rounded down from it
    ca_tbl_causal_sim.cc:1187   10 -> 5  the UNSTABLE 400-trial estimate, superseded at :1191
    linkbudget.py (computed)    25.2 m   the analytic model

The 10 m comment is a conservative restatement of a real engine measurement of **12.9 m**. **The
model and the engine disagree by roughly 2× on UWB r80** — and the disagreement is not a detail,
it decides the answer:

| criterion | tier-3 links retained | solvable at 2 surveyed |
|---|---|---|
| model, 25.2 m | 10 of 10 | redundancy **+3** |
| **engine, 12.9 m** | **5 of 10**, adjacent only | **SINGULAR, rank 4 of 6** |

Under the engine's own number the surviving graph is a 5-cycle, and a cycle is not rigid in 2-D.

**Three things this records.**

1. **The conclusion reversed twice** — infeasible, then feasible, then unresolved. Each reversal
   came from widening the population of the check: one comment, then the model, then every r80 in
   the tree. **The first two answers were confidently wrong and the third is honestly open.**
2. **"Bounded exposure" was the wrong phrase.** `B2_GAUGE.md` first said the ≈3.7 dB model-engine
   margin residual left the conclusion safe. A 3.7 dB margin residual *is* small as a margin.
   Expressed as a **range**, it is a factor of two, and it flips a rank result. **A residual quoted
   in the units it was measured in can look harmless in the units it is used in.**
3. **The right instrument was in the tree the whole time.** `:220` records a 2000-trial engine
   probe, and `:1187-1191` explains why 400 trials was not enough — the tree already knew both the
   number and why the number needed care. Nothing had to be re-measured; it had to be **read**.

**Not generalised to tier 2.** Its intra-site chord is 259.8 m against a censored tier-2 r80 of
≥ 900 m (`:217-218`) — an order of magnitude of headroom, so no plausible model-engine gap reaches
it. Tier 2's zero-redundancy finding is geometric and stands on its own.
