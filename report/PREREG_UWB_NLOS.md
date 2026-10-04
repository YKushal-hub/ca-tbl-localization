# PREREG UWB-NLOS — can a cited IEEE 802.15.4a NLOS model fill the UWB CIR confusion matrix?

**Registered before any registered cell runs** (rule 13). Written 2026-09-30. Engine source
at registration: `53e5c38`. This commit adds only this file. **The model it registers does
not exist yet.** The next commit adds it as `--uwbNlosModel=legacy|ieee802154a`, default
`legacy`. §2 specifies the model exactly, and that commit must implement §2 as written. Any
departure is an amendment and is recorded as one in the results file, report/UWB_NLOS_RESULTS.md.

The runs will be made from a clean tree at that commit. Their `run_id.txt` will cite its
short SHA and read `dirty 0`. The directory `~/catbl_uwbnlos/registered/` does not exist
at registration, and every file in it must postdate this commit.

Owner decision 2026-09-30 ("fix them all"): resolve `docs/TECH_DEBT.md` M.2. That item
says the CIR classifier (`86e43f2`) has no UWB positives: no decoded UWB frame is ever
NLOS in the engine.

---

## 0. The question, and why it has to be asked this way

M.2 gives two reasons the matrix is empty.

1. The engine's NLOS probability, 1 − exp(−d / scale), uses kilometre scales outdoors
   (8 000 m in `RURAL_FORESTED`, 54 000 m in `RURAL_OPEN`). At UWB distances it is about
   0.1 %.
2. Where NLOS is common, the UWB penalty loses the frame: 6 of 6 NLOS UWB links were lost.
   Both NLOS terms carry +9.64 dB at 7.99 GHz.

The owner's decision asks for both to be replaced with **cited** IEEE 802.15.4a parameters,
and for a check of whether the matrix then fills. Two facts in the sources decide what can be cited. They were read from the
report itself (IEEE P802.15-04-0662-02-004a, Molisch et al.):

- **802.15.4a has no LOS/NLOS probability.** Its LOS and NLOS parameter sets are separate
  channel models (§III, one table per environment). The state draw at UWB distances
  therefore has to come from somewhere else, and whatever it comes from, **mapping it onto
  this scenario is ours.**
- **802.15.4a has no data for this scenario, and says so.** Remark 1 (§I-B) names disaster
  areas, avalanches and rubble as important environments, and adds that no measurement data
  are available for them. Its outdoor NLOS path-loss parameters (n, σS, κ) are marked
  "educated guesses" in the outdoor table's own comment column.

So the default cannot become the new model. Whatever the matrix does, the default stays
`legacy`, and that decision is not an outcome of these runs.

## 1. What is being tested

Does the UWB confusion matrix of `--nlosClassifier` get **non-zero TP and non-zero FN** in
some regime, when the UWB engine draws its LOS/NLOS state and its NLOS excess loss from §2
instead of the legacy model?

**THE CIRCULARITY.** The classifier can only rediscover the NLOS the model creates. The
CIR is synthesised from the engine's own state: the S-V set chosen by `isNLOS`, the
direct ray K·10^(−nlosDb/10), and the SNR. A filled matrix therefore shows that the
modelled NLOS reaches the observables a receiver may read. It shows nothing about a real
channel.

## 2. The model, exactly

The model is used in the UWB engine only (`lora-uwb-physics-engine.cc`), on every UWB link
evaluation: ranging polls and MAC frames alike. The 868 MHz and 2.4 GHz engines are not
touched.

**State.** Draw one u ~ U(0, 1) from the engine's own sampler, at the point where the
legacy model draws its u. The link is NLOS iff u < P_NLOS(d_2D), with

    P_NLOS(d) = 1 − exp(−d / k),   k = −d_clutter / ln(1 − r) = −10 / ln(0.8) = 44.81 m

- *Cited:* the formula is 3GPP TR 38.901 V19.2.0 Table 7.4.2-1, InF-SL row (Pr_LOS =
  exp(−d_2D / k_subsce)). d_clutter = 10 m is Table 7.2-4's InF-SL clutter size. r = 20 % is
  Table 7.8-7's low clutter density; Table 7.2-4 itself says only "< 40 %".
- *OURS:* using a factory's sparse-clutter law for a ground-level outdoor SAR link, with
  trunks, undergrowth, terrain and bodies as the clutter. InF-SL is chosen because its base
  station is clutter-embedded, below the clutter height, as a UWB node at 1.5–2.5 m is in
  scrub or forest. That is an analogy and not a citation.
- d_2D is the horizontal distance, as in the source. Values: P_NLOS = 0.022 at 1 m, 0.044
  at 2 m, 0.106 at 5 m, 0.200 at 10 m (exactly 1 − (1 − r)) and 0.360 at 20 m.

**NLOS excess loss.** On an NLOS draw the link budget is charged, where the legacy penalty
was:

    ΔPL(d) = (PL0_N − PL0_L) + 10 (n_N − n_L) log10(max(d_3D, 1 m) / 1 m) + 20 (κ_N − κ_L) log10(f / 5 GHz)

This is the 802.15.4a NLOS path loss minus its LOS path loss for one environment. The
terms are eq. (5) at d0 = 1 m, and the frequency term of eq. (12), with PL0 referenced at
fc = 5 GHz (eq. 10). The antenna attenuation A_ant = 3 dB is common to both sets and
cancels. f is the engine's UWB carrier, 7.99 GHz.

The pair is chosen by environment class **exactly as the CIR synthesiser chooses its S-V set**
(`CaTblCirModelFor`), so that the link budget and the CIR describe one channel:

| environment class | pair | PL0 L / N (dB) | n L / N | κ L / N | ΔPL at 1 / 2 / 5 / 10 / 20 m (dB) |
|---|---|---|---|---|---|
| `ENV_INDOOR_OFFICE` | CM3 / CM4 office | 35.4 / 57.9 | 1.63 / 3.07 | 0.03 / 0.71 | 25.27 / 29.60 / 35.33 / 39.67 / 44.00 |
| every other class | CM5 / CM6 outdoor | 45.6 / 73.0 | 1.76 / 2.5 | 0.12 / 0.13 | 27.44 / 29.67 / 32.61 / 34.84 / 37.07 |

These values were checked against the report's tables (§III-B, §III-C). The report's
measurement ranges are 3–28 m for office and 5–17 m for outdoor, and the outdoor table says
"valid up to 20 m". Beyond those ranges the fit is an extrapolation, and that is stated.

The excess is **deterministic** given d. No NLOS shadowing is drawn: the report's
simulation procedure excludes shadowing (§II-C), and the engine already applies its own
shadowing to every link, widened under NLOS by its own `nlosToLosSigmaRatio`. Adding
802.15.4a's σS on top would count that variation twice. The composition itself is
**OURS**: a difference of two 802.15.4a fits is added on top of the engine's own absolute
LOS path loss, which is not 802.15.4a's.

**What the model does not do.** The legacy SDR-grade Markov penalties (+6 / +18 dB) are not
added. They are inactive at the scenario's statistical fidelity in any case.

**What follows the new state unchanged.** Everything the engine and the synthesiser already
condition on `isNLOS` follows the new state, and none of it is edited:

- the NLOS Rician K draw, the NLOS shadowing σ ratio and the NLOS delay spread;
- two-ray suppression, in `RURAL_OPEN`;
- the UWB ranging NLOS bias and floor;
- the CIR's S-V set (CM6 or CM4) and its direct ray K·10^(−nlosDb/10). At ΔPL ≥ 25 dB that
  ray is at least 25 dB below K, so it is effectively absent. That matches 802.15.4a's
  NLOS sets, which carry no separate LOS component.

`nlosProbability` reports P_NLOS(d_2D).

**Randomness.** The model makes exactly one sampler draw per evaluation, the state. The
legacy model makes the same draw, and on an NLOS draw two more. No new random variable is
created, so no stream index moves, and only the UWB engine's own draws are displaced. The
868 MHz and 2.4 GHz engines, the MAC and every static non-UWB arm are expected to be
byte-identical between the two models. The code commit checks this and does not assume it.

## 3. Population, denominator, window, truncation (rule 1)

- **Unit:** one UWB link evaluation of the measured tag inside `rangeTier()`, i.e. one poll.
- **Denominators:** the matrix is over *decoded* UWB links, the rows of
  `nlos_links_static_uwb_seed<N>.csv` with tier UWB. Scored rows fill tp/fp/fn/tn, and
  unscored rows (a CIR peak under the noise threshold) are counted apart. *Polled* = decoded
  + lost, and the lost count by truth is the summary's `lostLos` / `lostNlos`.
- **Window:** 0–3600 s, 1 440 cycles per seed, counted from the trace-free outputs by row.
- **Truncation:** none can occur, because each evaluation is instantaneous.
- **Pooling:** sums over the 10 seeds of a cell. No per-seed claim is made.

## 4. Cells

Common to every run:

```
--topology=1 --arms=static_uwb --duration=3600 --trace=false --nlosClassifier=true
--seed=<N> --outdir=$HOME/catbl_uwbnlos/registered/<cell>/seed_<N>
```

The legacy topology is used because it is the one topology where UWB links are polled at
all. The default topology polls none (M.2; the NC2 searcher regime gives 0 UWB rows). It is
a scenario for exercising the channel model, not a claim about the deployment.

| cell | extra flags | seeds |
|---|---|---|
| `A_802` | `--envFacility=0 --vegFacility=1 --uwbTriad=2 --uwbNlosModel=ieee802154a` | 7–16 |
| `A_leg` | `--envFacility=0 --vegFacility=1 --uwbTriad=2 --uwbNlosModel=legacy` | 7–16 |
| `B_802` | `--uwbNlosModel=ieee802154a` | 7–16 |
| `B_leg` | `--uwbNlosModel=legacy` | 7–16 |

- **A** is the searcher's end-game: UWB anchors 2 m from the site in open ground with
  sparse shrubs, so polls fall at 1–3 m. The configuration was checked before registration
  on a seed outside this set (seed 101, 900 s, pre-change binary, legacy model): 207 decoded
  UWB rows, 148 of them at 1–4 m, with accumulated CIR SNR 0–45 dB.
- **B** is the default environment (`RURAL_FORESTED` / dense-forest facility, the 10 m
  triad).
- Runs are launched from the repository tree that built the binary (M.3), and the binary's
  sha256 is recorded before every launch.

## 5. Predictions

The margins quoted below are the pre-change probe's (`--probe=true`, seed 7, 2 000 trials
per distance), kept at `~/catbl_uwbnlos/probe_open` (`--envFacility=0 --vegFacility=1`) and
`~/catbl_uwbnlos/probe_def` (defaults).

- **P1 (the fill), `A_802`:** pooled UWB tp ≥ 1 **and** fn ≥ 1. The margin arithmetic
  behind it: at 2 m in open ground the LOS mean margin is 25.8 dB (pre-change probe) against
  ΔPL = 29.7 dB. At 1 m it is about 32 dB, extrapolated from the probe's 2 m and 5 m points,
  against 27.4 dB. So some NLOS frames decode,
  near sensitivity, where the accumulated SNR is ~7–20 dB and many CIRs will be unscored.
- **P2 (legacy controls reproduce M.2), `A_leg` and `B_leg`:** pooled UWB tp = 0 and fn = 0.
  Few NLOS polls: at most 3 in `A_leg` and at most 15 in `B_leg`. Legacy P_NLOS is 3.7e-5 at
  2 m in `RURAL_OPEN` and 1.25e-3 at 10 m in `RURAL_FORESTED`.
- **P3 (the default environment), `B_802`:** NLOS polls are common (lostNlos ≥ 50 pooled),
  and **at most 2** NLOS frames decode pooled, each at d_3D ≤ 3 m. So tp + fn ≤ 2. At 10 m
  the forest's LOS mean margin is −6.2 dB against ΔPL = 34.8 dB.
- **P4 (control flow), both `_802` cells:** every UWB row with truthNlos = 1 has nlosDb =
  ΔPL(distM) within 1e-6 dB, and every row with truthNlos = 0 has nlosDb = 0.
- **P5 (descriptive; what the circular detector sees), `A_802`:** mean score over scored
  NLOS rows exceeds mean score over scored LOS rows.

## 6. Falsifiers, and what follows

- **P1 false** (tp = 0 or fn = 0 in `A_802`): the cited model does not fill the UWB matrix
  even at 1–3 m in open ground. M.2 then stands for every cited NLOS excess, and filling
  the matrix needs an obstruction model 802.15.4a does not supply: a direct path blocked
  with the scattered energy intact, as a body or a trunk does. That model would be ours, and
  it would be a separate decision. **No parameter is changed and no cell is re-run to
  rescue P1.**
- **P2 false:** the legacy population claim in M.2 is wrong. Stop and investigate before
  reporting anything else.
- **P3 false** (more than 2 decoded NLOS in `B_802`, or any beyond 3 m): the margin
  arithmetic is wrong. Report the distances and SNRs, and do not re-tune.
- **P4 false:** the code does not implement §2. That is a defect, and it withdraws the cell.

## 7. Abort conditions

Any run exiting non-zero; any sidecar not reading `dirty 0`; the binary's sha256 differing
between cells. Any of these stops the campaign, and the partial results are not reported
as results.

## 8. What is not concluded, whatever happens

- **Nothing about a real channel.** This is the circularity in §1.
- **No cross-cell magnitude.** The two models consume the UWB engine's draws at different
  rates, so `A_802` and `A_leg` do not share a stream past the first NLOS draw. Only
  population statements compare across them (rule 8, and its cross-binary form).
- **Nothing about the deployment.** The legacy topology is not the system (`CLAUDE.md`,
  topology block).
- **Nothing about the default.** It stays `legacy` for the reasons in §0.
