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

Regenerated at `7c0c5e5 dirty 0` (`results/p10_plane_v7`, 20 seeds, digests
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

### W2 — `trig_precision` 0.9343 cannot be reproduced from the directory it cites

The ledger publishes `trig_precision = 0.9343`. Recomputed over all 20 seeds of
`results/p10_plane_fix` — the directory the ledger cites — the mean is **0.9279**.
No population over that directory was found that yields 0.9343. This is a
provenance gap, not rounding: the published number and its cited source do not
correspond.

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
