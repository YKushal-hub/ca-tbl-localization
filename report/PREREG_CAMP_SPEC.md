# PREREG CAMP-SPEC — the spec configuration measured

**Registered before any cell runs** (rule 13). Written 2026-10-03. Engine source at
registration: `cef0fc6`. The preset (`--specProfile`) then carries:
- GEO-01's geometry (report/GEO01_RESULTS.md);
- CAL-02's outcome for PCS (report/CAL02_RESULTS.md).

This commit adds this file, `scripts/camp_spec_run.sh`, `scripts/camp_spec_analyse.py` and
`report/geo01_runner_up_flags.txt` (the runner-up's geometry flags for cell C), and nothing
else. The outputs go to `results/spec_2026-10-03/`, which does not exist at registration.

`docs/SPEC_CONFORMANCE_PLAN.md` §3.5. The docx's own metrics, on the spec configuration:
- CEP50/95;
- PCS-error correlation against the trace control;
- integrity-check precision and recall under injected faults;
- time to first fix and time to precision;
- precision spread across batch targets.

Plus the plan's sweeps, the comparator arms, GEO-01 stage 3, and the owner's 2026-10-03
endgame arm.

---

## 1. Cells (`scripts/camp_spec_run.sh`; 1 800 s, traces on, `--specProfile=true` throughout)

| cell | what | arms | seeds |
|---|---|---|---|
| A_main | the preset | the six: `ca_tbl_full`, `ca_tbl_no_integrity`, `fixed_threshold`, `static_868`, `static_2p4`, `static_uwb` | 1-20 |
| B_end | the endgame: `--boxHalf=750 --finderMode=1` (finders converging; the owner's decision after MS-3) | the six | 1-20 |
| C_runnerup | GEO-01 stage 3: the runner-up's geometry | `ca_tbl_full`, `static_868` | 1-20 |
| D_route_* | NW-2's routing-weight sweep, in the endgame (where subjects route through each other): (α, β, γ, slack) ∈ {(1,1,1,1), (1,0,0,1), (0,1,0,1), (0,0,1,1), (1,0.01,1,1), (1,1,1,2), (1,0,0,2), (1,0.01,1,2)} | `ca_tbl_full` | 1-10 |
| E_<fault>_<recovery> | the SY-5 fault matrix (see §2), `--injectBiasFraction=0.25` | `ca_tbl_full`, `ca_tbl_no_integrity` | 1-10 |
| G_single, G_single_end | the single-hop confidence baseline ("LoRaWAPS-style"), at 5 km and in the endgame (see §3) | `ca_tbl_full` | 1-20 |

## 2. The fault matrix (E)

Faults, magnitudes OURS, each over the seed-derived window [1 800 × (0.25 + 0.5 · (seed
mod 7)/7), + 450) s:
- `rb_common`: 30 m common-mode bias on every tier-2 range;
- `rb_single`: the same on one seed-chosen anchor;
- `txdrop`: a 6 dB transmit-power drop (cold-sagged battery);
- `antenna`: 10 dB antenna loss (ice detuning).

Each is run with the preset's `--suspectRecovery=downweight` and with `none` (reject and
coast). The label is "inside the window". The detector is tag 0's SUSPECT flag on a fix
cycle. Precision is TP/(TP+FP) and recall TP/(TP+FN), pooled over seeds. CEP95 is
reported inside and outside the window.

**DC-7's consequence, pre-registered:** under downweight a suspect fix is still fused (at
R × 4). So the in-window CEP95 is no better under downweight than under none for the
bias faults, and may be worse; recall is unaffected, because the flag is the same.

## 3. The single-hop baseline (G)

The preset with no relayed measurement in any fix set: `--relayedRanging=false`. That
also takes off what is FATAL without relays: `--relayChain`, `--hopVarianceTerms`,
`--hopProvenance`, `--fixRouting`, `--fixRoutingConfidence` and `--fixRoutingSubjects`.
So the baseline loses routing as well as relayed ranges; that is stated, not hidden.

It is evaluated against the preset (A, B) on tag 0's fix cycles:
- median error by the hop depth of the fused set (`hops_*`; 0 for the baseline);
- ρ(PCS, error) within each depth, the confidence calibration across hop counts.

## 4. Metrics and tests (`scripts/camp_spec_analyse.py`)

- **A, B, C, per arm:**
  - availability (the five subjects, `fixCycles/steps`), with a 95 % seed bootstrap;
  - tag 0's CEP50/95, and the subjects' mean CEP95;
  - TTFF (never = 1 800 s);
  - tier occupancy;
  - subject energy.
- **PCS against the trace control:** A = ρ(−traceSigma, error) − ρ(PCS, error) per seed,
  on tag 0's geometry-bearing fix cycles. The verdict comes from a 95 % seed bootstrap of
  the mean (BETTER / WORSE / INDISTINGUISHABLE), for `ca_tbl_full` and
  `ca_tbl_no_integrity`.
- **Sessions:** from the base station's batch session, the time to first coarse and to
  precise per target (never = 1 800 s), and the precision spread (UNDEFINED counted).
- **D:** the subjects' delivered / originated ratio, and diverged hops. The registered
  criterion picks the highest mean delivery ratio, with the default (1,1,1,1) winning any
  difference within 0.01. The weights stay OURS, chosen by that criterion.
- **C against A:** GEO-01 stage 3. A flip in the order on availability or CEP95 is
  reported, not re-tuned.

## 5. Predictions, made before any cell

- **P1.** At 5 km (A) no arm has a UWB-delivered fix, and the precision spread is
  UNDEFINED on every seed (report/MS3_UWB_CHECKPOINT.md, TECH_DEBT N.15).
- **P2.** PCS against the trace control is INDISTINGUISHABLE or WORSE for `ca_tbl_full`
  in A and B (CAL-01, CAL-02).
- **P3.** Integrity precision under every fault is below 0.5: SUSPECT fires on most
  preset fix cycles even without a fault (305 of 318 in G24's run).
- **P4.** In the endgame (B) `static_uwb` has the lowest tag 0 CEP50 and the highest
  subject energy of the six arms.
- **P5.** The single-hop baseline's median error at depth 0 is within 10 % of the preset's
  depth-0 median in A (relays rarely carry the 5 km fix: N.11, N.15).

Each is written up as held or failed in `report/CAMP_SPEC_RESULTS.md`.

## 6. Abort conditions

- A cell exits non-zero: stop, fix the cause, never drop the cell.
- `results/spec_2026-10-03/` exists: the runner refuses.
