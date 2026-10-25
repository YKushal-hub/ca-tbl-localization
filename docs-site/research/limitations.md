# Limitations

These are measured or recorded limits, in the repository's own words where it has them. Read them before treating a
result as a statement about the real world.

## Simulation and model

- **No hardware.** Nothing here has been checked against a field node. A same-site hardware test (TECH_DEBT T45) is
  open.
- **Receiver sensitivity is unverified end to end.** A six-decibel indexing error (`sf - 7` against tables whose index
  0 is SF5) sat on the tier carrying about 95 % of active cycles and **passed every gate, verifier check and validation
  in the project**. An independent closed-form model refusing to reconcile found it. No check in the project would
  catch a new error of that kind. Sensitivities are now read from data-sheet rows, but that is a reading, not a check.
- **Unattributed residuals.** About 3.7 dB on UWB and 3.5 dB for 868 MHz near field remain unexplained; conclusions are
  drawn by recomputing under ±4 dB of margin offset chosen to bracket them.
- **Loss terms computed and discarded.** Four loss terms (antenna gain, Doppler penalty, multipath penalty, soil
  attenuation) are assigned 0.0 on the line after they are computed, in the caller; in one configuration eight of
  sixteen terms in the loss sum were structurally zero.
- **Effective sensitivity is not a constant.** A 1.5 dB link-hysteresis margin applies whenever the link was previously
  receiving, outside the loss sum, so the reported margin's denominator moves with the outcome and can mimic a
  distance-dependent effect.
- **Terrain is procedural** and was not rescoped (gate G11 stays red by decision). Foliage depth is clamped to the link
  length.
- **Propagation assumptions.** The path-loss exponent and shadowing values are engineering assumptions with no traced
  field source.
- **The NLOS detector is circular by construction.** On a simulated channel it can only rediscover the obstructions the
  channel model created; its measured precision on one audit was 0.02–0.08.
- **The tier-1 ranging error is large**: 1543 m mean at the distance where delivery falls to 0.8. Tier 1 is a coverage
  floor, and applying a time-of-arrival Cramér–Rao bound to it would be a category error (an open defect: the CRLB gate
  should not run on tier 1).
- **One random sampler ignores its seed** (`lora-distribution-sampler.cc`); determinism holds and no result is known to
  be affected.
- **The evidence gate's 20 m radius** lies beyond UWB's reach (delivery 0.14 at 20 m).

## The decision layer

- The escalation rule is **not shown to beat** a flat variance threshold; the score carries no information beyond the
  covariance trace.
- The predicted-benefit rule $\Delta J/C>\lambda$ is specified and **not evaluated**.
- The integrity check does not detect most injected faults and was removed from the preset.
- The thresholds and the dwell are fitted or the project's own choice; none was swept across environments.

## The evaluation

- **The 5 km margin is thin**: 0.952 (CI [0.911, 0.975]; 18 of 20 runs).
- Results hold for **one environment** (forested, dense vegetation) and one geometry family. Different terrain, density
  or subject behaviour has not been evaluated.
- The requirement is on position error within 25 m; time to first fix and convergence are not measured.
- The deployment that meets the requirement (12 gateways, 12 finders) is **not the brief's**; it was found by measuring
  gateways × finders.
- Energy is radio energy only; processing energy is not modelled.
- A self-deploying gateway mesh is **not implemented** and is outside the brief's fixed deployment.

## Regulatory

The simulator models the Indian 865–867 MHz duty-cycle rule. Other jurisdictions, and the 2.4 GHz and UWB bands, are
not modelled ([regulatory considerations](../energy-and-compliance/regulatory.md)).

Open work with entry points is in [`docs/WORK_PACKAGES.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/docs/WORK_PACKAGES.md) and
[`docs/TECH_DEBT_STATUS.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/docs/TECH_DEBT_STATUS.md).
