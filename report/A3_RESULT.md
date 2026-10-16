# A3 — the intra-cycle handshake: measured, and equivalent to what it replaces

> **Local line, legacy topology.** Ported to `main` on 2026-10-16 as a record (`docs/LOCAL_LINE_INTEGRATION.md`). Every number below was produced on the local line's binary and the pre-correction topology `main` treats as a different deployment (`CLAUDE.md`), on commits that resolve through `SHA_MAP.md` to `archive/local-line-2026-10-16`. Flags it names (`--imuPrior`, `--intraCycleHandshake`, `--imuCoastMomentMatch`) exist on that branch, not on `main`.

Registered in `report/PREREG_A3_INTRA_CYCLE_HANDSHAKE.md` (`a28b591`) against code `e5474d9`;
run at `e94d04d`, 40 invocations, every sidecar `dirty 0`, digests verified; scored once by the
registered scorer (`report/a3/`).

**Deviation, recorded.** The machine rebooted while the last seven `on` cells (seeds 614–620) were
running; none had written its sidecar digest, so none had produced a result. They were moved aside
unmodified (outside the repository) and re-run at the same commit, binary and arguments. A cell
that never completed was re-run; no completed cell was replaced. Without the re-run the exclusion
would have exceeded the registered 10 % abort.

## Registered outcomes

| | prediction | outcome |
|---|---|---|
| P1 | static arms byte-identical, every column | **HOLDS** — 0 of 60 row pairs differ |
| P2 | zero pre-emptions under the flag, intra-cycle handshakes > 0 | **HOLDS** — 0 pre-emptions; 3 729 intra-cycle handshakes |
| P3 | tier-1 exchanges blocked by the handshake off-time, reported | `fixed_threshold` **184.4** per run, `ca_tbl_*` 1.0 — **exactly one per intra-cycle handshake** |
| P4 | `ca_tbl_full` cep50 difference CI includes 0 | **HOLDS** — and the difference is exactly zero on every dynamic arm |

Also: escalations unchanged; `duty_868` ≤ 0.0227 everywhere (the regulation now enforced on the
handshake too); radio energy ×0.9997 on `fixed_threshold` (one CAD probe fewer per handshake),
×1.0000 on the tiered arms.

## The exact zero, explained rather than reported (working rule 4)

A difference of exactly zero is a control-flow claim, so it was traced. On `fixed_threshold`,
seeds 601 and 605: **0 of 1 440 cycles** have a different position estimate; the active tier, and
the cycles that used the tier-1 floor, are identical. The mechanism:

- **Flag off:** the escalation commits, and the *next* cycle pre-empts its tier-1 exchange to send
  the handshake. That cycle has no tier-1 measurement.
- **Flag on:** the handshake is sent at the end of the current cycle's off-time — and arms its own
  off-time (~1.65 s), which blocks the *next* cycle's tier-1 exchange. That cycle has no tier-1
  measurement.

**The same slot is lost either way.** The pre-emption disappears from the counter, and the
regulation's cost lands on exactly the exchange it used to. Under a correctly enforced off-time,
the work package's derivation — "eliminate pre-emption at any `dt ≥ 2.32 s` at no accuracy cost" —
is true and empty: no accuracy cost, and no accuracy gain, because nothing changed but the label.
The existing arbitrated path was already the equivalent.

A reporting note: under the flag the trace's `floorRanged` column counts the blocked attempts
(955 against 765 on seed 601), because it records that the floor was asked for, not that it
transmitted. The blocked count is `tier1RangingBlocked` in `A3_CENSUS`.

## What this settles

**A3 is closed as a no-op on outcomes.** Its one lasting product is the defect it exposed and fixed
under its flag: the escalation handshake did not arm the duty-cycle off-time it obliges, and tier-1
ranging did not consult the governor. Every committed result was produced without that enforcement;
this campaign shows that on the arms and configuration tested, enforcing it changes no estimate.
