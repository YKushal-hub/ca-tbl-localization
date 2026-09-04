# Pricing the PCS repair — pre-registration

Written and committed **before the energy columns of `siting_repair/colocated_min` are read**. The
cells already exist, so this registers **before looking**, not before running — strictly weaker, and
labelled as such rather than presented as a pre-registration.

Availability and UWB-cycle counts from that directory have been read (they are the previous
result). `totalJ`, `j_868`, `j_2p4` and `j_uwb` for `colocated_min` have **not**.

## The per-cycle cost is not a constant — established before predicting

The work order's sketch used ~0.53 J per UWB cycle, from W11's 623.8 J over 1170 cycles. **Derived
from directories rather than accepted, that figure does not generalise:**

| directory | `j_uwb` | `cycles_uwb` | **J/cycle** | evidence gate |
|---|---|---|---|---|
| `claim2_rebase/legacy_evfalse` | 623.828 | 1170.1 | **0.5331** | **off** |
| `claim2_rebase/propagation_evfalse` | 128.234 | 212.2 | **0.6042** | **off** |
| `claim2_rebase/legacy_evtrue` | 0.188 | 73.0 | **0.0026** | **on** |
| `siting_repair/complementary_min` | 0.042 | 13.4 | **0.0031** | **on** |

**A 200× split, and it separates cleanly on the evidence gate**, not on the NLOS source, the
predicate form or the siting. With the gate off the node escalates blindly and pays full polling
cost on cycles where no anchor answers; with it on, the cycles it spends on UWB are ones where
anchors are present and ranging succeeds.

**So the work order's 0.53 J/cycle is the cost of *blind* escalation and does not apply to the
repaired arm, which runs with the gate on.** Using it would have over-predicted the repair's cost by
two orders of magnitude, and the falsifier the work order proposed — *"per-UWB-cycle cost is not
constant across configurations"* — is confirmed **before** any prediction was built on it.

**Derivation used.** The two evidence-gate-on directories: **0.0026 and 0.0031 J/cycle**.
`complementary_min` is the closer match — same predicate (`min`), same source, same gate, differing
only in siting — and it is out-of-sample for `colocated_min`, which is the cell being priced.

## Registered prediction

Repaired arm, `siting_repair/colocated_min`, 83.0 UWB cycles/seed [76.9, 89.2]:

- **additional UWB energy** = 83.0 × [0.0026, 0.0031] = **0.216 – 0.257 J**, widening to
  **0.200 – 0.277 J** across the cycle-count CI.
- **`j_uwb` for the repaired arm: 0.20 – 0.28 J.**
- **`totalJ`: 22.755 + ~0.24 ≈ 23.0 J**, and in any case **below 23.5 J** — i.e. the repair's UWB
  cost is **negligible**, under 1.5% of total energy.

**Falsifier.** Measured `j_uwb` outside 0.20 – 0.28 J. Then per-cycle cost is not stable even within
the evidence-gate-on family, and no energy claim may rest on the derivation.

**Mechanism, and it constrains both terms** — which is why a prediction is licensed here where it
was not for precision. Energy is `cycles × per-cycle cost`: the cycle count is measured with a CI,
and the per-cycle cost is derived from two matching directories that agree within 20%. Precision was
`tp/(tp+fp)` with the mechanism moving both terms and constraining neither.

**Unit check, per the standing clause.** `j_uwb` is per-seed joules and `cycles_uwb` is per-seed
cycles, so their ratio is J/cycle and multiplying by a per-seed cycle count returns per-seed joules.
The endpoint and the mediator share the seed as their unit.

## Not registered: the verdict

**M1's PASS/FAIL is the ledger's question and no prediction is registered on it.** The criterion is
`b49df46` §4, applied verbatim by `scripts/claim3_m1.py`, which reproduces the published Claim 3
cell exactly. It is scored, not forecast.

Both axes are reported together — availability and energy — because a single-axis result is what
this project's own future-work section identifies as the original mis-specification.

**Claim 9 is not touched.**
