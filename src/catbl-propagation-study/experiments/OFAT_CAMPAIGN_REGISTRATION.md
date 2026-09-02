# The OFAT campaign — registration, written before the first sweep

Verifiable timestamp: this file's commit, which precedes every sweep result.

## The problem this campaign has to solve openly

The model **does not reconcile** with the engine. G11b is permanently red, terrain is
unvalidatable, and ~3.7 dB (UWB) and ~3.5 dB (868 MHz) of residual remain unattributed with
candidates ruled out but no cause found. **Curves computed on this model have untrustworthy
absolute values.**

The response is not to fix the model. It is to require that **every conclusion carries the
test showing it does not depend on the parts that are wrong.**

## THE ROBUSTNESS VERDICT RULE — registered before any sweep

Every conclusion is recomputed on two axes:

1. **Terrain**, at **0 dB** and **30 dB** (its saturation, `lora-physics-engine.cc:400`).
2. **Unattributed residual**, as a margin offset of **−4, 0, +4 dB** per tier — chosen to
   bracket the measured 3.5–3.7 dB rather than to be comfortable.

| result | classification | reporting |
|---|---|---|
| holds at both terrain limits **and** all three offsets | **ROBUST** | report it as a finding |
| holds under terrain, moves under offset | **SENSITIVE** | report the conclusion *and* its sensitivity; **not** stated as a finding |
| moves under terrain | **UNDETERMINABLE** | report as undeterminable by this module |

**A conclusion that fails is reported as failing, never dropped.** A sixteen-row table with
undeterminable rows is a better document than a twelve-row table with an unexplained gap.

**§5.1 and §5.2's exact terrain invariance is evidence about those two conclusions only.**
It is not carried across. Each experiment earns its own verdict.

## Protocol, unchanged from §8 of the governing document

- one parameter swept, all others at baseline, **both limits reached**, with the limiting
  mechanism named at each end: **saturation, inversion, or model breakdown**
- same corpus for every experiment; its digest recorded in each sidecar; comparison across
  digests refused
- every quantity with its uncertainty over the corpus, and the corpus size stated
- **negatives are results**: a parameter that does not matter across its whole physical
  range is reported as such

## Registered predictions

Baseline tier for single-tier sweeps is 868 MHz unless stated. Intervals are the span
across the corpus.

| # | parameter | sweep | prediction | falsifier |
|---|---|---|---|---|
| P1 | carrier frequency | 150 MHz → 24 GHz | `r80` falls monotonically; slope ≈ −20 dB/decade of `f` in margin, so `r80 ∝ f^(-2/n)` with n=2.3 → **r80 ratio ≈ 10^(-2/2.3) per decade ≈ 0.14×** | non-monotonic `r80` in `f` |
| P2 | bandwidth | 10 kHz → 2 GHz | CRLB sigma falls as 1/BW **until noise growth reverses it**; a minimum in sigma must exist inside the range. `r80` falls monotonically (noise ∝ BW) | no minimum in sigma anywhere in 10 kHz–2 GHz |
| P3 | TX power | −30 → +30 dBm | `r80` rises as `10^(P/(10n))`, i.e. **×10 per 23 dB** at n=2.3; no saturation, since nothing in the model caps range | any saturation or inversion |
| P4 | RX sensitivity | −70 → −140 dBm | mirror image of P3 exactly: `r80` rises `10^(-S/(10n))`. **Must break down** where sensitivity passes the thermal floor `−174+10log10(BW)+NF` = **−117.0 dBm** at 125 kHz, NF 6 | no breakdown at the thermal floor |
| P5 | noise figure | 0 → 15 dB | `r80` falls as `10^(-NF/(10n))`, **÷4.4 over the range**. 0 dB is the unphysical sanity anchor | non-monotonic |
| P6 | spreading factor | 7 → 12 | sensitivity −2.5 dB/step → `r80` **×1.28 per step**, ×3.4 over SF7→SF12; airtime ×2/step forces the duty-cycle limit | `r80` not monotonically increasing |
| P7 | path-loss exponent | 1.6 → 6.0 | `r80` collapses; at fixed margin M, `r80 = 10^(M/(10n))` so the **span is enormous** — orders of magnitude | monotonicity fails |
| P8 | shadowing sigma | 0 → 16 dB | `r80` **rises** with sigma, because the PDR curve's upper tail extends faster than its lower tail truncates at the 0.80 crossing; spread widens throughout | `r80` falls with sigma |
| P9 | NLOS probability | 0.0 → 1.0, 11 levels | `r80` falls monotonically and **saturates** as p→1, both ends physical and both run | non-monotonic |
| P10 | NLOS excess delay | 0 → 200 m | ranging **bias** grows linearly as `p·delay`; position error floor `= p·delay` independent of anchor count — see §5.2 | the floor depends on anchor count |
| P11 | Rician K | −20 → +30 dB | mean fade loss falls monotonically from Rayleigh (**+2.5 dB**) toward 0 dB; saturates above K≈20 dB | non-monotonic, or a limit outside 0–2.5 dB |
| P12 | delay spread | 10 ns → 3 µs | **model breakdown** where delay spread exceeds `1/BW`; first-path detection fails. At 125 kHz that is **8 µs**, so the sweep does NOT reach breakdown at 868 — predicted and stated in advance | breakdown occurs below 3 µs at 125 kHz |
| P13 | anchor count | 3 → 30 | position sigma falls as `1/sqrt(n)`; **×0.32 from 3 to 30**; no floor under zero-mean noise | a floor appears without a bias term |
| P14 | anchor spacing | 50 m → 10 km | coverage fraction falls; the useful axis is `spacing/r80`, and coverage collapses **above spacing/r80 ≈ 1.5** | collapse point moves with tier when expressed in `spacing/r80` |
| P15 | anchor geometry | ring/grid/collinear/clustered | GDOP: ring best, collinear **→ ∞** (degenerate, rank-deficient) | collinear GDOP finite |
| P16 | node speed | 0 → 50 m/s | coherence time `T_c ≈ 0.423/f_D` falls below frame airtime at high speed. **References `SESSION_CHANGELOG.md` F4**: `RunProbe` produces structurally zero velocity, so every `r80` in this project was measured at v = 0 | — |

## P16's standing issue

`SESSION_CHANGELOG.md` F4 records that `RunProbe` produces structurally zero node velocity.
**Every `r80` in this project was measured at v = 0.** This experiment is what says whether
that matters, and its result is the scope statement for every other `r80` in the project.

## Abort conditions

- any sweep whose corpus digest differs from `ea40076996e71a84b4bcce81` — comparison refused
- any experiment where a swept parameter is a **distribution** and the model applies a
  point estimate to a nonlinear function: **stop and integrate**, per the standing rule
  recorded after its third instance. Watch specifically at P8, P11 and P12.

---

# RESULTS

Corpus `n = 2000`, digest `ea40076996e71a84b4bcce81` — the same corpus every earlier
experiment used; comparison across digests is refused. Raw output:
`results/ofat_campaign.txt` (`.log` is gitignored).

## The sixteen rows

| # | parameter | conclusion | class | limit behaviour and magnitude |
|---|---|---|---|---|
| P1 | carrier frequency | decreasing | ROBUST | 150 MHz→24 GHz: `r80` 3617 → 191.8 m, **18.9×** over 2.2 decades (0.264× per decade). No saturation or inversion; **model breakdown** at the top end under 30 dB terrain, where the link is dead at every range |
| P2 | bandwidth | decreasing | ROBUST | sigma 27.48 → 0.0615 m. **No interior minimum** — registered prediction MISSED, see below |
| P3 | TX power | increasing | ROBUST | −30→+30 dBm: `r80` 41.8 → 3760 m, **2.12× per 10 dB**. No saturation, as registered |
| P4 | RX sensitivity | increasing | ROBUST | −70→−140 dBm: `r80` 12.0 → 3955 m. Thermal floor is **−117.0 dBm**; the model computes past it because **it has no noise-floor check** — that absence is the model breakdown this sweep exposed |
| P5 | noise figure | decreasing | ROBUST | 0→15 dB: `r80` 2440 → 1042 m, **2.34×** (registered 4.4× on the nominal exponent — MISSED, see below) |
| P6 | spreading factor | increasing | ROBUST | SF7→SF12: `r80` 1811 → 3174 m, 1.12×/step, **1.75× total**. At 2.5% duty the SF12 minimum inter-transmission interval is **16.1 s** |
| P7 | path-loss exponent | decreasing | ROBUST | n 1.6→6.0: `r80` 6638 → 18.8 m, span **353×** — the largest effect of any parameter |
| P8 | shadowing sigma | non-monotonic | **UNDETERMINABLE** | interior maximum at sd ≈ 4 dB under 0 dB terrain; monotonic decrease under 30 dB. **Registered prediction MISSED** |
| P9 | NLOS probability | decreasing | ROBUST | p(1 km) 0→1: `r80` 3644 → 267.5 m. Both ends run and both physical, as required |
| P10 | NLOS excess delay | increasing | ROBUST | floor `= p·delay = 60.0 m` at 200 m, **identical at 3, 6, 12, 30, 100 and 1000 anchors** |
| P11 | Rician K | decreasing | ROBUST | −20→+30 dB: mean fade loss **+2.509 → +0.005 dB**. Rayleigh limit `10γ/ln10 = 2.507 dB`; both limits asserted at import |
| P12 | delay spread | increasing | ROBUST | at 125 kHz, `1/BW = 8.0 µs`, so a 3 µs sweep **never reaches breakdown — registered in advance.** On UWB `1/BW = 2.0 ns` and breakdown begins there |
| P13 | anchor count | decreasing | ROBUST | 3→30: position sigma **0.316× (1/√n)**, no floor under zero-mean noise |
| P14 | anchor spacing | decreasing | ROBUST | coverage collapses above **spacing/r80 = 1.41** |
| P15 | anchor geometry | ring best, collinear degenerate | ROBUST | GDOP ring 0.8165, grid 0.7071, clustered 4.808, **collinear ∞** — rank-deficient, as registered |
| P16 | node speed | increasing | ROBUST | coherence time 146.1 ms at 1 m/s → 2.9 ms at 50 m/s against a **12.5 ms** frame airtime; decorrelation mid-packet **above 20 m/s** |

**15 ROBUST, 0 SENSITIVE, 1 UNDETERMINABLE.**

## Missed predictions, reported as missed

**P2 — no interior minimum exists, and this was derivable rather than empirical.**
`sigma = c/(2π·BW·√(2·SNR))` with noise ∝ BW gives `SNR ∝ 1/BW`, so
`sigma ∝ 1/(BW·√(1/BW)) = 1/√BW` — monotonically decreasing forever. The registration
predicted noise growth would *reverse* the improvement; it only slows it from `1/BW` to
`1/√BW`. The registered falsifier ("no minimum anywhere") fired, and the prediction was
wrong on algebra I could have done before running.

**P3, P4, P5 — registered on the nominal path-loss exponent where an effective one governs.**
The registration used `n = 2.3` to predict `r80` ratios. Measured effective exponents near
`r80` are **3.03–3.54 (868 MHz)** and 3.66–4.07 (2.4 GHz), because the NLOS excess and
foliage terms are themselves distance-dependent and steepen the margin slope. P5's 2.34×
against a registered 4.4× is entirely this. **UWB's `n_eff` is 1.99–2.00 — exactly
free-space**, which independently confirms the two-ray branch is what governs there.

**P8 — `r80` does not rise with shadowing throughout.** It rises to sd ≈ 4 dB and then
falls. Shadowing helps while its upper tail extends reach faster than its lower tail
truncates it, and stops helping once the link is loss-dominated. Under 30 dB terrain the
rise disappears entirely, which is why P8 is the one UNDETERMINABLE row.

## The methodological result: 0 SENSITIVE is a finding about the test, not the physics

Zero of sixteen conclusions moved under the ±4 dB offset. That is **not** evidence that
everything is robust to the unattributed residual — it is evidence that the test as
registered is too coarse to fail, because monotonic direction almost never changes under
4 dB.

**The registration never operationalised what "the conclusion holds" means.** That is the
same silence that made §5.1's metrics A and B disagree — repeated in a document written
*after* diagnosing it there. It is recorded as a gap, not resolved retroactively: the
registered rule stands as monotonic shape, and effect-size stability is reported as a
**separate** dimension rather than folded in. Retrofitting a criterion after seeing results
is the move this project has refused four times, and it does not become acceptable because
the registration is mine.

## Two artefacts caught before they were reported as findings

**Four experiments first classified UNDETERMINABLE on a NaN.** `lb.r80` scans a distance
grid and returns NaN when it finds no 0.80 crossing — including when the link is dead at
every range, which is what 24 GHz under 30 dB terrain is. That NaN read as an undefined
shape, so P1, P3, P4 and P7 looked terrain-dependent when the physics is "the link is
dead". A dead link is a statement, not the absence of one. The campaign runner returns the
floor; `lb.r80` is untouched because `validate.py` and G11 depend on it.

**`nlos_scale_m` had no override at all.** All three read sites took `ENV[env]` directly,
so P9 would have swept NLOS probability and moved **no number whatsoever** — a sweep that
runs clean and measures nothing, which is the exact failure this project has hunted since
the overnight diagnostic. `linkbudget.py` now **raises** on any unrecognised override key.

**A third, in the model rather than the campaign:** the Rician quadrature's Bessel series
overflowed above K ≈ 100, returning **+7.512 dB** of mean fade loss where the K→∞ limit
must be 0 dB. Rewritten in log space with the large-argument asymptotic, and **both
analytic limits are now asserted at import** so a numerical failure cannot pass silently.
