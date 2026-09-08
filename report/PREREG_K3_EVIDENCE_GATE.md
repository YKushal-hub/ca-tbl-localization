# PREREG K3 — is the evidence gate the 300 J?

**Registered before the cell runs.** Rule 13.

A note on provenance, because it is visible in the sidecars. This is committed while the
K2 cells are still executing, so K2's `run_id.txt` files will cite this commit rather than
`5a73344`. The **binary is unchanged** — no rebuild happened between them, and the `build`
timestamp in every K2 sidecar shows the same build. Committing a registration before its
cell runs is a load-bearing rule; keeping every sidecar in one campaign on a single SHA is
a tidiness preference. The rule wins, and the discrepancy is written down here rather than
left for a reader to trip over.

---

## 1. The claim this tests, which is currently an inference

`report/K1_DOMINATION_RESULTS.md` §4 reports that the repaired bundle cuts
`ca_tbl_full`'s energy by 83–96% — 20+/0− at every scale, p < 0.0001, and roughly 1500×
the RNG-displacement floor. It then says the mechanism **"is almost certainly the evidence
gate"**, reasoning that `k1b` escalates 196–204 times and spends 300+ J, that `static_uwb`
costs ~890 J for full-time use, so the excess must be UWB, and that the evidence gate is
what suppresses escalation to a tier with no coverage.

**That is an inference from five flags moving together, not a measurement.** The bundle
also contains gated PCS, floor-on-demand, the corroboration witness and the bidirectional
digest. Any of them could carry the energy.

The recommendation that follows from it — that `--escalationRequiresEvidence` should
default to **true** — is a change to a shipped default. It should not rest on an
inference.

## 2. The cell

`k3_noevid_box1500` — the `k1a` configuration at `boxHalf=1500`, with
`--escalationRequiresEvidence=false` and **nothing else changed**. `k1a_box1500` is the
paired control at the same 20 seeds.

## 3. Predictions

| # | prediction | how it fails |
|---|---|---|
| R1 | Turning the evidence gate off raises `ca_tbl_full` `totalJ` by **more than 100 J** at 20 seeds | the rise is below 100 J, in which case the gate is not the mechanism and §4's inference is withdrawn |
| R2 | The rise is accompanied by a rise in `escalations` and a rise in `j_uwb` specifically | energy rises without UWB rising, which would mean a different mechanism |
| R3 | `k3_noevid_box1500` does **not** reach `k1b_box1500`'s 320.99 J — the other four flags carry some of it | it reaches or exceeds it, meaning the gate carries essentially all of the bundle |

R1 is the one that matters. It is deliberately set well below the ~305 J the bundle moves,
so that "the gate is a large part of it" and "the gate is all of it" are distinguishable
by R3 rather than conflated.

## 4. Falsifier

**S1.** If R1 fails, §4's sentence "the mechanism is almost certainly the evidence gate"
is **withdrawn** from `K1_DOMINATION_RESULTS.md` and replaced with the measurement, and no
recommendation about the default is made on the strength of it.

## 5. Abort conditions and analysis

As K1 and K2: 120 rows exactly, counted; `dirty 0`; a new path. Paired per-seed
differences against `k1a_box1500` on `totalJ`, `j_uwb`, `escalations`, `rmseM` and
`fixCycles`, two-sided sign test, with the static arms as the displacement floor.
