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
