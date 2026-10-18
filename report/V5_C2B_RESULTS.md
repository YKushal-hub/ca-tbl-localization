# V5-C2B results — escalating to UWB whenever reachable costs 59 % more energy for no accuracy (registered, exploratory)

Registration: `report/PREREG_V5_C2B.md` at `5d22e838`, committed before any cell. 80 runs, all `rc 0`,
`source 5d22e838 dirty 0`, one compile stamp, frozen binary with a SHA-256 manifest. Tracked per cell:
`arm_summary.csv`, `run_id.txt` (`results/v5_2026-10-17/`).

## 1. Verdicts (all as registered)
| # | prediction | outcome |
|---|---|---|
| V5-1 | B: `ca_tbl_full` CEP95 C2B − OFF > 0 | **held**: +0.5 m [−3.7, +5.2] (14.9 → 15.4 m) |
| V5-2 | B: energy >= 1.3 × | **held**: × 1.593 (24.0 → 37.9 J; UWB cycles 35 → 68) |
| V5-3 | B: CEP50 within ±20 % | **held**: −0.09 m [−0.46, +0.29] |
| V5-4 | A: CEP50 C2B − OFF < 0 | **held**: −5.0 m [−20.1, +7.5] (no UWB reachable at 5 km; 2.4 GHz escalations only) |
| V5-5 | `static_2p4` identical | **held** |

**Claim C2b in CA-TBL: NOT SUPPORTED** (the registered expectation).

## 2. Reading — what the C2 question finally comes to
- **868 → 2.4:** not gating on confidence is right. Stage A showed it (always_attempt best; the confidence-gated
  rules 10–100× worse), and the preset's dual base already does it: both LoRa tiers are ranged every cycle.
- **2.4 → UWB:** gating on confidence is the energy-efficient choice in this engine. In the endgame 2.4 GHz alone
  already gives CEP50 about 3.5 m. UWB adds nothing measurable to accuracy at this scale, and every extra UWB
  cycle costs energy (UWB is the largest share of tag-0 energy there).
- So the correct rule is tier-dependent:
  - attempt the next LoRa tier opportunistically;
  - escalate to UWB only on need, the existing PCS / trace trigger with its evidence gate.

  This is not the uniform rule C2 (or C2b) proposed.
- **Decision:** `--c2bEscalation` stays in the engine, default off, a recorded negative.

## 3. Analysis output (`scripts/v5_analyse.py`)
# V5-C2B results (exploratory)

| config | arm | variant | CEP95 (m) | CEP50 (m) | tag-0 J | UWB cycles | escalations |
|---|---|---|---|---|---|---|---|
| A | static_2p4 | OFF | 634.5 | 249.6 | 14.25 | 0.0 | 0.0 |
| A | static_2p4 | C2B | 634.5 | 249.6 | 14.25 | 0.0 | 0.0 |
| A | ca_tbl_full | OFF | 841.7 | 288.9 | 16.01 | 0.0 | 22.3 |
| A | ca_tbl_full | C2B | 842.4 | 284.0 | 16.04 | 0.0 | 23.9 |
| A | fixed_threshold | OFF | 847.9 | 279.6 | 16.34 | 0.5 | 18.5 |
| A | fixed_threshold | C2B | 846.9 | 286.3 | 16.38 | 0.8 | 20.6 |
| B | static_2p4 | OFF | 34.0 | 3.9 | 9.60 | 0.0 | 0.0 |
| B | static_2p4 | C2B | 34.0 | 3.9 | 9.60 | 0.0 | 0.0 |
| B | ca_tbl_full | OFF | 14.9 | 3.5 | 24.00 | 35.4 | 55.6 |
| B | ca_tbl_full | C2B | 15.4 | 3.4 | 37.91 | 68.2 | 95.3 |
| B | fixed_threshold | OFF | 12.7 | 3.5 | 26.47 | 39.6 | 47.5 |
| B | fixed_threshold | C2B | 15.0 | 3.2 | 36.53 | 65.6 | 83.7 |

B ca_tbl_full CEP95 C2B-OFF +0.5 [-3.7, +5.2] m; CEP50 -0.09 [-0.46, +0.29] m; energy ratio 1.593
A ca_tbl_full CEP50 C2B-OFF -5.0 [-20.1, +7.5] m

## Predictions

- **V5-1**: held
- **V5-2**: held
- **V5-3**: held
- **V5-4**: held
- **V5-5**: held

**C2b in CA-TBL: NOT SUPPORTED** (supported only if B CEP95 is not worse and energy rises < 30 %)

