# UWB-NLOS — the cited 802.15.4a model does not fill the UWB CIR matrix

**Written** 2026-09-30 from `results/uwb_nlos_20260930`, a verbatim copy of
`~/catbl_uwbnlos/registered/`.
- **Registered** in `report/PREREG_UWB_NLOS.md` at `166d431` (12:52 +0530), before the model
  existed.
- **The model** was built at `57225b8`, exactly as the registration's §2 specifies. There is
  **no amendment** to §2.
- **The 40 cells** ran from a clean tree at `57225b8` between 20:07 and 20:13 +0530. Every
  file postdates both commits.
- **Provenance.** All 40 sidecars read `source 57225b8 dirty 0 profile optimized`, and their
  digests verify. The binary's sha256 was `ce0a43e9…` for every cell (`RUNS.md`).
- **Completeness.** Every seed has 1440 steps. No §7 abort condition fired.
- **The analysis** is `scripts/uwb_nlos_analyse.py`, committed with the model and applied
  unchanged. `ANALYSIS.md` is its verbatim output.

**Verdict: the fill fails.**
- P1 is **FALSIFIED**: tp = 0 and fn = 2 in the open-ground cell.
- P2 and P3 are also falsified, in the clauses shown in §2. Their investigation (§3) finds
  the registration's own arithmetic wrong, not the model or M.2.
- P4, the implementation check, **HOLDS**.
- The default stays `legacy`, as registered, whatever the result.

---

## 1. Pooled UWB matrix per cell (10 seeds each, decoded links; §3 denominators)

| cell | tp | fp | fn | tn | unscored LOS | unscored NLOS | lost LOS | lost NLOS | decoded | decoded NLOS |
|---|---|---|---|---|---|---|---|---|---|---|
| `A_802` open ground, 2 m triad, 802.15.4a | 0 | 7 | 2 | 6 936 | 310 | 28 | 512 | 1 445 | 7 283 | 30 |
| `A_leg` same, legacy | 0 | 6 | 0 | 7 912 | 1 044 | 0 | 2 988 | 10 | 8 962 | 0 |
| `B_802` forest default, 802.15.4a | 0 | 0 | 0 | 696 | 334 | 2 | 981 | 459 | 1 032 | 2 |
| `B_leg` same, legacy | 0 | 0 | 0 | 936 | 492 | 0 | 1 635 | 4 | 1 428 | 0 |

These are populations. The two models consume the UWB engine's draws at different rates and
share no stream past the first NLOS draw, so no magnitude compares across a pair (§8, rule 8).

## 2. The registered predictions

| | prediction (§5) | observed | verdict |
|---|---|---|---|
| **P1** | `A_802`: tp ≥ 1 **and** fn ≥ 1 | tp 0, fn 2 | **FALSIFIED** |
| **P2** | `A_leg`, `B_leg`: tp = fn = 0; NLOS polls ≤ 3 and ≤ 15 | tp = fn = 0 in both (**holds**); NLOS polls **10** in `A_leg` (> 3), 4 in `B_leg` | **FALSIFIED** (the `A_leg` count clause) |
| **P3** | `B_802`: lostNlos ≥ 50; ≤ 2 decoded NLOS, each ≤ 3 m | lostNlos 459 (holds); 2 decoded NLOS (holds), at **4.38 m** and 1.56 m | **FALSIFIED** (one beyond 3 m) |
| **P4** | both `_802`: NLOS rows carry ΔPL(distM) to 1e-6 dB; LOS rows 0 | 0 violations over 30 + 2 NLOS rows, and over every LOS row | **HOLDS** |
| **P5** | descriptive: `A_802` mean score NLOS > LOS | NLOS 0.0000 over 2 scored rows; LOS 0.0008 over 6 943 | **FALSIFIED** |

## 3. P2 — investigated first, as §6 requires

§6: *"P2 false: the legacy population claim in M.2 is wrong. Stop and investigate before
reporting anything else."*

**M.2's claim is not what failed.** M.2 says the engine's NLOS probability at UWB range is
"about 0.1 %". The legacy cells' NLOS rate per polled UWB link is:
- `A_leg`: 10 / 11 960 = **0.084 %**;
- `B_leg`: 4 / 3 067 = **0.13 %**.

**What failed is the registration's arithmetic for the count.** The "≤ 3" in `A_leg`
assumed polls at about 2 m, where legacy RURAL_OPEN gives P = 3.7e-5. But the legacy
topology's poll set is the one-hop neighbour table, and entries persist within their TTL as
the node walks between sites. So polls reach far beyond the 2 m triad:

| cell | decoded distM p10 / p50 / p90 / max | legacy NLOS expected among decoded, Σ P(d) | distance at which legacy P equals the observed rate |
|---|---|---|---|
| `A_leg` | 1.44 / 3.16 / 33.5 / 299.9 m | 1.77 | 45 m |
| `B_leg` | 1.69 / 4.62 / 13.8 / 89.4 m | 1.13 | 10 m |

- Lost polls carry no distance in the outputs. *Inferred, not counted:* they are the far
  polls, where the link fails and legacy NLOS is likelier.
- The P2 clause that tests M.2's claim about **decoded** links, tp = fn = 0 (no decoded
  legacy UWB frame is NLOS), **holds** in both cells.

**Conclusion of the investigation.** The legacy population claim stands. The prediction's
count threshold was mis-derived when it was registered, and that is recorded here rather
than re-stated. Nothing is re-run.

## 4. P1 — why the cited model does not fill the matrix

**The model does produce NLOS.**
- `A_802` drew NLOS on 1 475 of 9 240 polled UWB links (16.0 %; polled = decoded + lost, §3), against 10 of 11 960 under
  legacy. Almost all were lost: 1 445 of 1 475.
- The excess at 1–4 m is 28.3–31.9 dB, and that exceeds UWB's margin nearly everywhere.
  The registration's margin arithmetic expected some frames to survive near sensitivity,
  and 30 did.

**The 30 frames that survived cannot be scored.** Their in-band SNRs are −18.7 to −36.2 dB,
so the accumulated SNR after the 30.1 dB preamble gain is roughly +11 to −6 dB.
- **28 of the 30 are unscored**: the CIR's peak is under the 3σ noise threshold, so the
  leading-edge test has nothing to read.
- **The 2 that are scored both score 0**: the leading edge *is* the peak, so they read as
  LOS. Those are the 2 fn.
- *Inferred from the detector's arithmetic (`CaTblLeadingEdge`), not separately measured:*
  at these SNRs only the strongest tap clears the noise threshold. A suppressed direct path
  ahead of a stronger diffuse peak is exactly what the test cannot see at low SNR.
- The model's direct ray, K·10^(−ΔPL/10) with ΔPL ≥ 28 dB, is effectively absent, as §2
  intended. What is missing is SNR headroom, not an obstruction signature.

**What the registration says follows (§6), applied as written.**
- *"The cited model does not fill the UWB matrix even at 1–3 m in open ground. M.2 then
  stands for every cited NLOS excess."*
- *"Filling the matrix needs an obstruction model 802.15.4a does not supply: a direct path
  blocked with the scattered energy intact, as a body or a trunk does. That model would be
  ours, and it would be a separate decision."*
- *"No parameter is changed and no cell is re-run to rescue P1."*

This read-out adds one observation the registration did not anticipate. A body/trunk
obstruction model would have to leave enough accumulated SNR for the CIR's pre-peak taps to
clear the noise threshold. An excess of ~28 dB does not; roughly 10–15 dB would. That is a
number to reason from, not a result.

## 5. P3 — distances and SNRs, reported and not re-tuned

`B_802`'s two decoded NLOS frames:
- seed 8, at 4.375 m, in-band SNR −46.08 dB, excess 32.18 dB, unscored;
- seed 13, at 1.559 m, −41.96 dB, 28.87 dB, unscored.

The 4.38 m frame falsifies "each within 3 m", so the margin arithmetic behind that clause was
wrong in the forest environment by at least that frame. Both are unscored, like `A_802`'s.

## 6. A finding recorded at the code commit

The registration's §2 "Randomness" expected the 868 MHz and 2.4 GHz engines, the MAC and
every non-UWB static arm to be byte-identical between the two models. **They are not.**
- At NC1's flags under `ieee802154a`, `static_868` differs in 28 `arm_summary` columns, and
  its trace from t = 10 s.
- The network-wide MAC counters move: `mac_tx` 11 353 → 12 143.
- Mechanism (`57225b8`'s message; TECH_DEBT M.2): the UWB model changes which UWB-channel
  frames the MAC delivers, the shared MAC random stream is then consumed differently, and
  every tier shifts.
- It does not touch this campaign. Every prediction is a population inside one cell, and
  §8 already forbade cross-model magnitudes.

## 7. What is not concluded (§8), unchanged

- Nothing about a real channel: the classifier can only rediscover the NLOS the model creates.
- No cross-cell magnitude.
- Nothing about the deployment: the legacy topology is not the system.
- Nothing about the default. It stays `legacy`, as registered, and this result gives no
  reason to revisit that.

## 8. What it means for the spec-conformance plan (`docs/SPEC_CONFORMANCE_PLAN.md`)

- **PH-2 / register row 6 stays PARTIAL.** The spec's CIR leading-edge detector exists and
  is exercised. Its UWB matrix cannot be filled by a *cited* channel model. Filling it needs
  either an *ours* obstruction model (a separate owner decision, §4) or a regime where the
  decoded NLOS frames keep SNR headroom.
- **The preset's UWB channel model (plan §5 item 1).** The recommendation is `legacy`. The
  802.15.4a model does not deliver what it was built to test, and it draws NLOS on 16.0 % of UWB
  polls in open ground and loses 15.6 % of them, which would starve every tier-3 mechanism the preset needs
  populated. This is the owner's decision; it is recommended, not taken.
