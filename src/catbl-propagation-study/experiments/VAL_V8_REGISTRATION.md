# VAL-V8 — G11's reconciliation against the engine as it is, registered before the comparison

Registered before the model is evaluated against `results/probe_v8`. That directory does not exist at registration.
The verifiable timestamp is this file's commit. The script, `val_v8.py`, is committed with it and not edited
afterwards.

## Why

TECH_DEBT R.1. G11 and `validate.py` compare `linkbudget.py` with `results/probe_v7`, built Aug 27. `50fc8620`
(2026-09-08) then made the 2.4 GHz and UWB tiers follow the wide-area environment and vegetation (sentinel 255):
- before, they were `ENV_RURAL_OPEN` / `VEG_SPARSE_SHRUBS`;
- after, they are `ENV_RURAL_FORESTED` / `VEG_DENSE_FOREST`.

That commit measured tier 2's r80 moving 1149.5 → 156.0 m and UWB's 12.8 → 0.0 m (r50 5.3 m). Every G11 figure since
then describes a configuration no run uses.

## What is run and computed

1. **`results/probe_v8`:** G11's exact probe invocation, `--probe=true`, at today's code defaults, from the binary
   built at `635c0ab9` (the null control shows default output identical to its parent).
2. **The model:** `linkbudget.py` with a new override key `env`, used wherever `TIER_ENV` is read. It is evaluated
   with tiers 2/3 at:
   - `env = RURAL_FORESTED`;
   - `veg = VEGETATION["VEG_DENSE_FOREST"]`;
   - `veg_scale` per tier as before, because the engine's f^0.75 re-derivation is unchanged by `50fc8620`.

   **Every value is the engine's declared default. No value is chosen here.** Adding the key does not change
   `validate.py`'s output: its stdout is byte-identical before and after, which is checked.
3. **The criterion:** G11's, unchanged — r80 within 15 %, margin within 3.0 dB over the swept range. The G11a
   sub-population is also reported. Its terrain bound depends on heights only, so it is unchanged.

**Not done:**
- the IQ term is not added to the model (R.2; whether it is a defect is FIDELITY-1's question);
- no term is fitted;
- G11 is not re-pointed by this commit.

## Disclosure, which limits what this can show

**The engine side has been seen.** IQ-TERM's default run is today's default probe with two extra output-only
columns. Its PDR and margin columns are, by the null control's logic, those `probe_v8` will hold.

So nothing below is a blind prediction about the engine. The model side has not been evaluated in this
configuration. The comparison is deterministic, with no free parameter, so it cannot be fitted after the fact.

## Expectation, stated before the model is evaluated

**The convention is `validate.py`'s: gap = model − engine margin.**

**The IQ term is not new.** `probe_v7`'s engine subtracted it too (it was enabled before 2026-09-08). The only change
between `probe_v7` and today's defaults on tiers 2/3 is the environment and vegetation. So, if the model's forest
configuration mirrors the engine's, every gap should return to roughly its `probe_v7` value. At the G11a points those
were:
- 2.4 GHz: −2.21, −2.45, −2.70 dB at 10, 25, 50 m;
- UWB: −1.80, −0.02, +1.52, +2.49 dB at 2, 5, 10, 20 m.

**The registered expectation:** each G11a point's gap on `probe_v8` lies within ±1.5 dB of its `probe_v7` gap.
868 MHz is unchanged by construction and is reported as a control: its gaps should match `validate.py`'s today
exactly.

**G11's criterion on today's engine is expected to FAIL,** as it did on `probe_v7`. It fails on 868 MHz at short
range regardless (−4.95 dB), and the r80 comparisons for tiers 2/3 are new and unpredicted.

**What would contradict this:**
- any G11a point on 2.4 GHz or UWB more than 1.5 dB from its `probe_v7` gap. That would mean the forest move
  changed something the model does not represent. A first candidate is the foliage clamp at 35 dB or the
  `A_max` saturation under the f^0.75-scaled γ.
- an 868 MHz gap that differs from today's `validate.py`. That would mean the probe itself changed.
