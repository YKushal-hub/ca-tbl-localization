# Pre-registration — A3: the escalation handshake inside the cycle, with its off-time enforced

Written 2026-10-14. Code under test: commit `e5474d9` (`--intraCycleHandshake`).
Committed before the first cell; runner `scripts/sweeps/run_a3_handshake.sh`, scorer
`scripts/a3_handshake_analysis.py`, both committed with this file and not edited after it.

## 0. What A3 is, and what it had to fix first

`docs/WORK_PACKAGES.md` §A3: when the tier-1 duty budget is spent by a cycle's ranging, the
arbitrated path commits the escalation and **pre-empts the next cycle's tier-1 exchange** to
send the handshake. A3 sends it instead at the instant the off-time expires, inside the same
cycle — 2.263 s after a 56.58 ms exchange, before the next cycle at 2.5 s.

Building it found a modelling gap, recorded here because the experiment depends on it:
**the handshake never armed the duty-cycle off-time it obliges** — `ArmDutyCycle()` is called
only from `AccountRangingExchange()` (`ca-tbl-energy-model.cc`) — **and tier-1 ranging never
consulted the governor.** A deferral built on that model would reach zero pre-emptions "at no
cost" only because the regulation was not enforced on the handshake. So under the flag, and
only under it, the handshake arms its off-time (41 ms → ~1.65 s), tier-1 ranging respects the
governor, and every exchange that off-time blocks is counted (`A3_CENSUS` on stderr). With the
flag off nothing changes; the null control is in the A3 commit.

## 1. Design

| | |
|---|---|
| flags | `--duration=3600 --boxHalf=2000 --gwGrid=5 --nlosSource=propagation --escalationRequiresEvidence=true --pcsForm=min --trace=true` |
| treatment | `--intraCycleHandshake=true` against default, paired by seed |
| arms | all six |
| seeds | **601–620**, N = 20, disjoint from every registered campaign |

## 2. Predictions

**P1 (control, exact).** On `static_868`, `static_2p4`, `static_uwb`, every `arm_summary.csv`
column is byte-identical between conditions, every seed: 0 of 60 row pairs differ. A static
arm never escalates, so it never sends a handshake; the only off-time it meets is its own
ranging's 2.263 s, inside the 2.5 s cycle, so the governor never blocks it. *Falsifier:* any
differing row — the change reached a path it should not have; interpretation stops.

**P2 (exact, control-flow).** With the flag on, `handshakePreemptions = 0` on every arm and
seed: the deferral fits whenever 2.263 s + the frame (~41 ms) ≤ 2.5 s, which is always at
`dt = 2.5`. An exact zero is a control-flow claim (working rule 4), so its reachability is
registered with it: **`intraCycleHandshakes > 0`** summed over the dynamic arms' seeds.
*Falsifier:* any pre-emption, or zero intra-cycle handshakes (the path never ran).

**P3 (the cost, measured not assumed).** `tier1RangingBlocked` is reported per arm. The work
package claimed "no accuracy cost"; that claim is only as good as how often the cycle after
an escalation needs tier-1 ranging. **Not predicted in sign or size.**

**P4 (accuracy, two-sided).** Per dynamic arm, the paired per-seed difference in `cep50M` and
in availability `A_fix` (as A0 v2 defines it), treatment − control, with a seed bootstrap CI.
The work package predicted no change; registered here as: the CI of `cep50M` difference on
`ca_tbl_full` **includes 0**. *Falsifier:* it excludes 0, in either direction.

**Also reported:** escalations, `duty_868` (must stay ≤ 0.025 — a compliance check, now with
the handshake's off-time enforced), radio energy ratio.

## 3. Abort conditions

Completeness per cell (digest, `dirty 0`, six rows, six traces, and on treatment cells an
`A3_CENSUS` line per arm on stderr); > 10 % excluded aborts. P1 failing stops interpretation.
No seed extension, no re-run to replace a cell, no scorer edit after the first cell.

## 4. Disclosure

The A3 code was compiled and its flag-off null control run before this commit. One flag-on
smoke run was made to confirm the mechanism executes: seed 99 (outside every registered range),
600 s, `ca_tbl_full` only, these flags. Read: one escalation, one intra-cycle handshake, zero
pre-emptions, **one tier-1 exchange blocked** in the following cycle — the cost P3 measures —
and the refusal of `--sendHandshakeFrames` in combination. No error, availability or energy
figure from it was compared with anything. No flag-on run at seeds 601–620 exists before this
commit.

## 5. What this cannot establish

Anything at another `dt`; the work package's `dt ≥ 2.32 s` derivation is not swept here. The
C1 frames (`--sendHandshakeFrames`) are refused in combination with this flag, so contention
of a deferred frame is not measured.
