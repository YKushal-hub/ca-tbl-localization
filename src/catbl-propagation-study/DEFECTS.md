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

**Fix.** `lora-2p4-physics-engine.cc:154`, `lora-uwb-physics-engine.cc:154`:

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
arm (`:2024-2037`, `:2064`), and they share an RNG stream. Changing how many
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
(`:1376-1380`): *"Channel diagnostics are accumulated over ALL trials, not only
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
post-EVM (`lora-physics-engine.cc:631-632`, `:598-605`).

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

**Site.** `scratch/ca_tbl_causal_sim.cc:1413`. `meanSnr` fell back to `0.0` when
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
the arms differ only at `ca-tbl-policy.cc:877`, whose trigger is
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
`vegWide = VEG_DENSE_FOREST` (`ca_tbl_causal_sim.cc:3174`), and the engine applies
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
`lora-physics-parameters.cc:304-309` and `depth ~ LogNormal(ln 100, 0.6)` from `:293`.
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
