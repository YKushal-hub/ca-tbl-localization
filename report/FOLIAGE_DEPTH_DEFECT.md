# Foliage depth is drawn independently of link length, and it is tier 3 that pays

**2026-09-08.** Found while asking why the probe reports **UWB r80 = 0.0 m** — no
distance at which UWB reaches 80% packet delivery — under the corrected environment.

**Status: DEFECT.** No flag reaches it; it is a one-line physical constraint that is
missing.

---

## 1. What the code does

`src/lora-physics-engine/model/submodules/lora-foliage-submodule.cc:20`:

```cpp
double depth = sampler.Sample(params.vegetationDepthM);
```

`txPos` and `rxPos` are parameters of this function. They are used only as a cache key
for the wind-variation process. **The vegetation depth is never compared against the
distance between them.**

The distribution is `LogNormal(ln 100, 0.6, 0, 500)` — median 100 m, 10th percentile
46 m, 90th percentile 216 m — and the ITU-R P.833-10 term it feeds is

```
A_veg = A_max · (1 − e^(−depth·γ/A_max))
```

where `depth` is, in the recommendation, **the length of vegetation the path traverses**.
A path cannot traverse more vegetation than its own length. Here it routinely does.

## 2. The consequence is scale-dependent, which is why it hid

For `VEG_DENSE_FOREST` (γ = 0.20, A_max = 25 dB), at the median draw of 100 m:

| link | length | depth charged | A_veg charged | A_veg if depth ≤ length | over-charge |
|---|---|---|---|---|---|
| tier 1, 868 MHz | ~1515 m | 100 m | 13.77 dB | 13.77 dB | **0 dB** |
| tier 2, 2.4 GHz | ~156 m | 100 m | 13.77 dB | 13.77 dB | **0 dB** |
| tier 3, UWB | ~10 m | 100 m | 13.77 dB | **0.98 dB** | **12.8 dB** |
| tier 3, UWB | ~5 m | 100 m | 13.77 dB | **0.50 dB** | **13.3 dB** |

A link at or above the drawn depth is unaffected. A link below it is charged for
vegetation it cannot have crossed, and **the shorter the link the worse it gets** — which
is exactly backwards, and it means the error is invisible on the tier carrying most of
the traffic and maximal on the tier that looks broken.

At UWB's path-loss exponent of 2.3, 12.8 dB is a range factor of
10^(12.8/23) = **3.6×**. The measured r80 of 0.0 m and r50 of 5.3 m are therefore not a
statement about ultra-wideband in forest; they are a statement about a 10 m link being
charged 100 m of canopy.

## 3. Why this matters beyond one tier's range number

`docs/TECH_DEBT` H.1 records tier 3 as "reached on roughly 2.5% of cycles", and
`ENV_SPLIT_DEFECT` §K.5 records that a three-tier claim resting on a tier that fires that
rarely is not yet a three-tier claim. **Part of that rarity is this defect.** How much is
not yet known — the honest statement is that tier 3's reachability has never been
measured with a physically admissible foliage depth.

It also touches the tier-2 numbers, though less: 2.4 GHz's r80 of 156 m is above the
median draw, but a large fraction of *individual* 2.4 GHz links in a 300–400 m
deployment are shorter than the depth drawn for them.

## 4. The fix, and why it is not made here

```cpp
const double linkM = CalculateDistance(txPos, rxPos);
double depth = std::min(sampler.Sample(params.vegetationDepthM), linkM);
```

One line, and the two positions it needs are already parameters.

It is **not applied in this commit** for two reasons, both of which are about not
producing another number that has to be thrown away:

1. It changes the physics for every tier and every past result, exactly as the
   environment correction does. Landing two link-budget changes at once makes neither
   attributable, and the K1 campaign currently running is the first measurement on the
   corrected environment. Sequencing them is the difference between two clean
   before/after comparisons and one confounded one.
2. `ComputeFoliageLossDb` is called by all three engines and by two test suites, and
   the change must go in behind a default-off flag with a null control showing the
   flag's off state is byte-identical, per the required shape in `CLAUDE.md` §4.

*Cost:* the line is trivial; the null control, the flag, the gate and the re-measurement
are not. *Registered as the next physics item after the environment correction.*

## 5. What is NOT claimed

- Not that UWB is usable at long range once fixed. Its link budget is 87.5 dB against
  868 MHz's 137 dB; it is a short-range tier by design and it stays one. The claim is
  that its *modelled* range is understated by roughly a factor of three.
- Not that the `LogNormal(ln 100, 0.6)` depth distribution is wrong for a wide-area
  868 MHz link. For that tier it is reasonable and unaffected by the clamp.
- Not that this explains the whole of tier 3's scarcity. Finder geometry — three
  searchers within metres of the casualty simultaneously — is a separate and probably
  larger constraint.
