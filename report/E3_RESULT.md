# E3 — CAL-05 with the inertial prior: recall rises, and so do false alarms

> **Local line, legacy topology.** Ported to `main` on 2026-10-16 as a record (`docs/LOCAL_LINE_INTEGRATION.md`). Every number below was produced on the local line's binary and the pre-correction topology `main` treats as a different deployment (`RULES.md`), on commits that resolve through `SHA_MAP.md` to `archive/local-line-2026-10-16`. Flags it names (`--imuPrior`) exist on that branch, not on `main`. CAL-05's `cv` cells, which R1 reproduced, are `main`'s own evidence too.

Registered in `report/PREREG_E3_IMU_INTEGRITY.md` before any `imu` cell; run in the registered
queue at `c620352`, 400/400 invocations, every sidecar `dirty 0`, digests verified, one binary
(build stamp Oct 15 2026 09:28:44); scored once by the registered scorer, unedited (`report/e3/`).

**Provenance notes, recorded.** (1) The machine stopped at 2026-10-16 12:09:36 with 179 cells
complete; the 11 then running had no digest, were moved aside unmodified and re-run at the same
commit, binary and arguments. No completed cell was replaced. (2) 267 sidecars record `source
c620352`, 133 record `source c620352c`: the same commit, abbreviated one character longer once a
fetch added objects to the repository mid-campaign. Both resolve to `c620352c98ce…`.

This is the integrity check (Mahalanobis against χ²₀.₉₉ = 9.21), not the evidence gate.

## Registered outcomes

| | prediction | outcome |
|---|---|---|
| R1 | every `cv` cell reproduces its CAL-05 sidecar byte-for-byte | **HOLDS** — 200 of 200 identical |
| E1 | recall rises at 20/40/80 m × F1/F3, CI excluding 0 in ≥ 4 of 6 | **HOLDS** — 6 of 6 point estimates > 0, 6 of 6 CIs above 0 |
| false alarms | not predicted, reported two-sided | **rise in all ten cells**, every CI above 0 |

### Paired per-seed differences, `imu − cv` (n = 20)

| bias | fault | Δ recall | Δ false-alarm rate |
|---|---|---|---|
| 10 m | F1 | +0.136 [+0.114, +0.161] | +0.013 [+0.011, +0.016] |
| 20 m | F1 | +0.396 [+0.365, +0.425] | +0.020 [+0.015, +0.027] |
| 40 m | F1 | +0.414 [+0.374, +0.457] | +0.035 [+0.025, +0.045] |
| 80 m | F1 | +0.298 [+0.257, +0.345] | +0.063 [+0.052, +0.076] |
| 160 m | F1 | +0.141 [+0.112, +0.174] | +0.146 [+0.110, +0.186] |
| 10 m | F3 | +0.305 [+0.253, +0.357] | +0.019 [+0.015, +0.023] |
| 20 m | F3 | +0.448 [+0.404, +0.497] | +0.044 [+0.036, +0.053] |
| 40 m | F3 | +0.303 [+0.265, +0.347] | +0.088 [+0.073, +0.104] |
| 80 m | F3 | +0.118 [+0.089, +0.145] | +0.137 [+0.113, +0.163] |
| 160 m | F3 | +0.064 [+0.039, +0.091] | +0.263 [+0.205, +0.324] |

50 % recall crossing (descriptive): F1 57.7 → **19.1 m**, F3 32.7 → **12.9 m**.
Precision falls in every cell, to 0.61 (160 m F1) and 0.47 (160 m F3), from 0.90–0.98 under constant velocity.

## Reading, as registered

**The inertial prior moved the operating point along the curve; it did not show a better
detector.** The registration fixed this reading in advance for the case where both rates rise,
and both rise everywhere. A tighter `S = P_prior + R_fix` puts a bias further into the χ² tail —
which is E1 — and puts unmodelled error there too. At small biases the trade is favourable on its
face (at 20 m F1, recall +0.40 for false alarms +0.02); at 160 m F3 it is not (recall +0.06 for
+0.26). Whether the detector is better at a *matched* false-alarm rate is not answered by these
cells: that needs the threshold swept, and it was not registered.

The false-alarm rise grows with the injected bias, which a property of the prior alone would not
do. A candidate, **not measured**: an accepted biased fix contaminates the prior's origin
(registration §7), and an overconfident inertial prior then flags the honest fixes that follow the
fault window. That is consistent with the primary campaign's finding that this filter is
overconfident where fixes are sparse (NEES 5.0 here), and moment-matched coasting — which was
off in E3 — is the mechanism built for that. Neither is established by this campaign.

## What this cannot establish

That the check is independent of the radio (its origin is still the last accepted fix); anything
at another threshold, window fraction or suspect policy; anything on `main`'s corrected topology.
