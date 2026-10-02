# PREREG GEO-01 stage 1 — which gateway shapes are worth simulating, at the spec's scale?

**Registered before the screen runs** (rule 13). Written 2026-10-02. Engine source at
registration: `759fe48`, the commit that adds `--gwLayout`. This commit adds this file and
the two analysis scripts, `scripts/geo_screen.py` and `scripts/geo_optimise.py`, and
nothing else. Stage 1 runs no arm: it asks the build where each shape puts the gateways (a
2.5 s run per candidate), takes tier-1 PDR and range error against distance from the
build's own probe, and computes coverage over a lattice. Its outputs go to
results/geo01_stage1_2026-10-02/, which does not exist at registration.

Owner decision 5 (2026-09-30): the preset's gateway geometry is **found by a registered
study**, the least infrastructure that meets a SAR criterion, with gateways only where a
vehicle can reach (`docs/SPEC_CONFORMANCE_PLAN.md` §3.4). Stage 1 is the screen that
decides what stage 2 simulates. Stage 2 has its own registration (report/PREREG_GEO01.md,
P5), on the P4 build.

---

## 1. Scale and inputs

- Search area: `--boxHalf=5000`, the spec's "5 km radius" taken as the half-width of the
  square. Default environment (`--env=1`, rural forested, vegetation 3).
- Gateways at 15 m (every shape), the subject at 1.5 m.
- Link model: the probe's `868MHz` rows of `probe_range_sweep.csv`
  (`--probe=true --seed=7`, 2 000 trials per distance), interpolated in slant distance.
  Beyond the probe's last point (6 000 m) PDR and error are extrapolated log-linearly
  from its last two points, and PDR is floored at 0. A 10 km square puts many lattice
  points beyond 6 km from most gateways, so this extrapolation carries weight. That is
  stated, not hidden.
- Range error: σ = √(π/2) × the probe's mean absolute range error (normal errors).
- Links are treated as independent. Shadowing is in fact correlated between nearby
  links, so P(≥k) is optimistic where gateways cluster. This bites the cluster shape most.

## 2. What is computed, per candidate

Over a 50 m lattice of the area (40 000 points):

- **P(≥1), P(≥2), P(≥3)** gateways decode, by a Poisson-binomial over the gateways' PDRs.
  These are the Case D / B / A coverage.
- **Predicted error** where P(≥3) ≥ 0.5: the Fisher information Σᵢ pᵢ uᵢuᵢᵀ / σᵢ² over
  every gateway, inverted, and then CEP50 ≈ 0.5887 (σ_major + σ_minor).
- Reported per candidate:
  - the mean P(≥3) over the area, the availability proxy the prune ranks by (§4);
  - `area_p3_ge_095`, the fraction of the area where P(≥3) ≥ 0.95;
  - the mean P(≥2) and P(≥1);
  - the median and the 95th percentile of the predicted CEP50 over the fixable points.

## 3. Candidates

N ∈ {3, 4, 5, 6, 8}, each built by the binary:

| shape | flags | realistic |
|---|---|---|
| ring | `--nGateways=N` (`--gwRingFrac` 0.6) | yes |
| arc 90/140/180/240° | `--gwArcDeg=… --gwArcCentreDeg=180` | yes |
| perimeter | `--gwLayout=perimeter` | yes |
| cluster | `--gwLayout=cluster` | yes |
| star | `--gwLayout=star` | yes |
| L-shape | `--gwLayout=lshape` | yes |
| ridge | `--gwRidge=true` | yes |
| optimiser, perimeter band | `--gwLayout=file` from `geo_optimise.py` | yes |
| optimiser, unrestricted | `--gwLayout=file` from `geo_optimise.py` | **no**: an upper bound |
| grid 2×2, 3×3 | `--gwGrid` | **no**: reference only, rejected on realism (HANDOFF_2026_09_09 §4) |

The optimiser finds a local optimum (§4). It is a lower bound on the best placement, not
the optimum.

## 4. The pruning rule (registered)

For each N, among the **realistic** candidates only:
1. rank by the **mean P(≥3) over the area**, highest first. This is the expected fraction
   of time a three-gateway fix is available to a subject at a uniformly random location;
2. two candidates within 0.005 of each other on that are ordered by the lower median
   predicted CEP50;
3. keep the best two.

The pruned set (2 × 5 = 10 candidates) is what stage 2 simulates. Nothing else about the
stage-1 numbers is a result: they are a model's prediction, not a measurement. The
selection itself (the smallest N meeting the operational criterion) is stage 2's, on
simulated availability.

**Why the mean and not the area with P(≥3) ≥ 0.95, which the plan named.** The scripts
were smoke-tested before this registration at a different scale, `--boxHalf=2000`, N = 3,
a 200 m lattice. They were never run at the registered 5 km scale. The smoke test showed
that at small N almost no point reaches P(≥3) ≥ 0.95, so that area rewards bunching the
gateways. At N = 3 a 500 m cluster scored 0.010 against 0.000 for every spread shape, and
would have ranked first. The mean P(≥3) is smooth at every N and is the availability
proxy the operational criterion is about. The area with P(≥3) ≥ 0.95 is still reported.

The optimiser (`geo_optimise.py`) maximises the same quantity, then mean P(≥2), then mean
P(≥1), by swap local search from ten starts: greedy, evenly spaced, and eight seeded random
subsets. It is a local optimum. In the smoke test the realistic optimum at N = 3 was a
tight cluster on the box edge. That is not a search failure. With independent links,
P(≥3) needs all three gateways in range, and on the box edge three gateways together
cover more than three spread apart (0.364 against 0.249 for the perimeter shape). The
independence assumption flatters clusters (§1), so this is the screen's most fragile call.

## 5. Predictions, made before the screen runs

- **P1.** At N = 3, no realistic shape covers more than half the area with P(≥3) ≥ 0.95,
  and none has a mean P(≥3) above 0.5. Tier-1 r80 is 1 515 m, against a 10 km square.
- **P2.** At every N the best ring, arc or perimeter shape ranks above the best of
  cluster and star on mean P(≥3). Spreading the gateways along an access route beats
  bunching them, once the area is large against the tier-1 range.
- **P3.** The unrestricted optimiser scores at least the best realistic shape at every N.
  If it does not, the greedy search is broken, and that is reported as a defect of the
  script, not as a finding.
- **P4.** The mean P(≥3) does not fall as N rises within a shape family. Any exception
  is reported and explained.

Each is checked against the table and written up as held or failed in
report/GEO01_STAGE1_RESULTS.md, whichever way it goes.

## 6. Abort conditions

- A candidate's run exits non-zero, or puts a gateway outside the box: stop and fix the
  shape code. Never drop the candidate.
- The probe's 868 MHz PDR curve is not monotone within 0.10 (the probe's own check):
  stop. Do not screen on a curve the build flags as broken.
