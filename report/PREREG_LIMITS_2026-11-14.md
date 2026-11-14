# PREREG LIMITS — tier-1 reach sensitivity (L1) and the two-tier arm (L2)

**Registered before any measurement cell runs** (rule 13). Written 2026-11-14. This commit adds this file,
`scripts/limits_run.sh`, `scripts/limits_analyse.py` and nothing else. The engine changes it measures with are already
committed (`--tier1ExtraLossDb`, `--arms=ca_tbl_two_tier`, the policy's `skipTier2`); the gate that covers them
(`scripts/two_tier_gate.sh`) is a later commit and is not a measurement. Outputs go to
`~/ns/catbl-work/limits_2026-11-14/`, which holds only the null controls, the probes and one pilot at registration (none
is a measurement cell).

## The two limitations

- **L1.** "868 MHz reach unexplained: 2390 m simulated vs ~250 m in forest [17] (about 9.6x) and 15 km open [18]. Tier 1's
  role depends on it; no reach-sensitivity run exists for tier 1."
- **L2.** "No two-tier 868 MHz -> UWB arm, so tier 2's necessity is argued from reach, not tested."

## What was done before registration, and is not a measurement

1. Probes with the frozen PRESET-5 binary (`--specProfile=true --probe=true --probeTerms=true --probeMonotoneTime=true`,
   seed 361, `--env` 0-3), and with the new binary at `--tier1ExtraLossDb` 10, 15, 20, 30, 40, 46, 52 (env 1, seed 361):
   868 MHz r80 = 2408 m (E1, 0 dB), 1285 (+10), 888 (+15), 604 (+20), **256 (+30)**, 111 (+40), 0 (+46). The offsets
   below were chosen from these, so the probe at +30 is a calibration, not a test; P1 therefore reads fresh seeds.
2. A 900 s pilot, seed 361, `--env=1`, configs A and B, `--arms=ca_tbl_full,ca_tbl_two_tier`. Its first version kept the
   NW-7 grant gate for the two-tier arm and was **degenerate**: 0 confirmations against 7-11 timeouts, because the
   arbiters are 2.4 GHz radios (`arbMacById`) that grant only on UWB-reach evidence recorded from UWB exchanges or 2.4 GHz
   ranges under 29.5 m, so a node without a 2.4 GHz radio can never be granted. The arm was changed to enter UWB
   directly (no grant) and the pilot rerun: two-tier then spent 76 (A) and 66 (B) of 360 tag-0 cycles on UWB, 70 of
   the 76 (A) and 35 of the 66 (B) with no UWB anchor in nominal reach. **Precondition for L2, stated here: the
   two-tier arm must reach UWB (tag-0 UWB cycles > 0 pooled in every config x env cell group) or L2 is reported as
   degenerate, not as a result.** The pilot's numbers are not used in any prediction's threshold.
3. Null controls (below), which must pass before offset 0 or the sweep's per-arm results are reused.

## L1 — design

- **Flag.** `--tier1ExtraLossDb=X` adds X dB to tier 1's receiver sensitivity table (the 868 MHz link needs X dB more
  received power). It is a sensitivity shift, not a path-loss term: an RSSI range is still read from the unshifted
  received power, so tier-1 *ranging accuracy* is not degraded by it, only tier-1 reach and delivery. The L1 results
  are therefore **optimistic about tier 1's accuracy at short reach**, and the report says so. Tier 1 is also the
  control plane (identity queries, finder sessions, relay chain, beacons, the floor fallback), so `static_2p4` is
  expected to move too; this is registered below (P7), not a surprise.
- **Offsets:** 0 dB (the ENV-SWEEP's E1 cells, reused), +15 dB (probe r80 ~890 m), +30 dB (probe r80 ~256 m, the forest
  field value). Not tuned to any outcome.
- **Cells:** configs A (5 km) and B (endgame, `--boxHalf=750 --finderMode=1`), `--env=1` only, seeds 361-380,
  `--duration=1800`, preset flags exactly as in the sweep. **Arms: `ca_tbl_full`, `fixed_threshold`, `static_868`,
  `static_2p4`.** `poucet_fusion` is dropped: its energy is already measured at the preset, and tier-1 reach does not
  change its three-radio cost structure; its availability under reduced tier-1 reach is therefore NOT measured here
  and no claim about it is made. New cells: 2 offsets x 2 configs x 20 seeds = **80** (plus one verification cell).
- **Reuse of offset 0.** The sweep's E1 cells were produced by the frozen binary. They are reused only if the new binary
  with the flag off reproduces them byte-for-byte (null control N1/N2 and the VERIFY cell below).

## L2 — design

- **Arm.** `ca_tbl_two_tier` (by name only, not in 'all'): the adopted decision rule (it runs as `ca_tbl_no_integrity`,
  which the null control shows identical to `ca_tbl_full` under the preset) on a node with only the 868 MHz and UWB
  radios. Choices, all documented in the code (`RunArm`, `CaTblPolicyConfig::skipTier2`), each favouring the two-tier
  arm or neutral:
  - escalates 868 -> UWB directly, demotes UWB -> 868; the same PCS thresholds apply at tier 1 (up) and UWB (down);
  - **no grant gate** (the grant path needs a 2.4 GHz radio, see above); the arm skips the grant latency, denials and
    slot arbitration the adopted arm pays for;
  - the evidence gate keeps its neighbour-presence half (an 868-ranged neighbour within 20 m) and drops its
    estimate-quality veto (sigma <= 5 m), which a tier-1-only estimate would almost never pass;
  - the dual base's **868 MHz** half stays (868 + UWB fusion); its 2.4 GHz legs are not ranged, woken or billed awake;
  - the infrastructure (gateway/finder radios, the 2.4 GHz request-over-mesh path) is untouched, so the arm's anchors
    keep what the adopted arm's anchors have. A real two-radio fielding would lose that too; the arm is therefore a
    **lower bound on tier 2's necessity**, not an estimate of it.
- **Cells:** configs A and B x `--env` 0, 1, 2, 3 x seeds 361-380 = **160** cells, `--arms=ca_tbl_two_tier`, 1800 s.
  Comparators are the ENV-SWEEP's per-arm results for the same cells, reused after the per-arm independence check
  (VERIFY cell: the pair run's `ca_tbl_full` row equals the sweep's, and the filtered two-tier row equals the pair's).
- **Denominators.** Every A(25 m) is over all 720 cycles per node in the 1800 s window (a no-fix cycle counts by the
  estimate held); five-node pools tag 0 and four subjects. Mechanism shares: tag-0 only, denominators stated in each
  table (all cycles; or the pooled tag-0 UWB cycles). A cycle on UWB whose run ended inside the window is counted as
  it stands (censoring rule: a truncated observation still settles what it already crossed).

## Null controls (must pass before any cell is reused or any measurement is read)

- **N1.** New binary, nothing selected: for 6 cells (seeds 361-365; configs A and B; env 0-3; 600 s; the default 'all'
  arms in two of them and the five sweep arms in four), `arm_summary.csv` and every other output file except
  `run_id.txt` and `run.log` are byte-identical to the frozen PRESET-5 binary's.
- **N2.** The new arm run alone twice is identical (determinism).
- **N3.** A filtered run's row for an arm equals the same arm's row in a larger run (per-arm streams; VERIFY cells).
- **N4.** `ca_tbl_two_tier`'s mapping: `ca_tbl_full` and `ca_tbl_no_integrity` rows and all traces identical with names
  normalised (done on the frozen binary, 600 s, 117 files: identical).
- A failure of N1 withdraws the reuse of the sweep and stops the campaign.

## Predictions (L1) — thresholds fixed here; each is reported measured, hold or fail

| # | prediction | basis |
|---|---|---|
| P1 | probe r80 at +30 dB, env 1, seeds 362-364 (mean), lies in [200, 320] m | seed-361 pilot probe 256 m |
| P2 | `static_868` five-node A(25 m) falls from 0 to +30 dB in both A and B (paired interval of the difference above 0) | tier 1 is its only radio |
| P3 | CA-TBL five-node A(25 m) in B at +30 dB is at least 0.95 | the endgame box is 1.5 km: inside 256 m r80 more often than not, UWB does the rest |
| P4 | CA-TBL five-node A(25 m) in A at +30 dB is below 0.95 | 12 gateways over 10 km at ~256 m r80 leave gaps |
| P5 | CA-TBL beats `static_868` on five-node A(25 m) at every offset and config (paired interval above 0) | PRESET-5 +0.617 at 0 dB |
| P6 | CA-TBL five-node A(25 m) in A is non-increasing with offset (0 >= 15 >= 30) | less reach, less availability |
| P7 | `static_2p4` five-node A(25 m) in A also falls from 0 to +30 dB (interval above 0) | tier 1 is the control plane for tier 2 too |
| P8 | CA-TBL subject energy in A moves by less than 25 % of its 0 dB value at +30 dB | one tier transmits per cycle; the floor is cheap |

## Predictions (L2)

| # | prediction | basis |
|---|---|---|
| Q1 | `ca_tbl_full` minus `ca_tbl_two_tier` five-node A(25 m) in A, E1: paired interval above 0 | UWB reach is ~6 m (E1): a node escalated from 868 finds no anchor |
| Q2 | Q1 holds (interval above 0) in at least 3 of 4 environments in A | the mechanism is environment-independent |
| Q3 | the E1 difference is smaller in B than in A | the endgame places finders near the subjects |
| Q4 | CEP95 tag 0, A, E1: `ca_tbl_full` better than two-tier (interval of full - two-tier below 0) | |
| Q5 | subject J, A, E1: two-tier cheaper than `ca_tbl_full` (interval of full - two-tier above 0) | no 2.4 GHz radio awake or ranging |
| Q6 | at least half of two-tier's tag-0 UWB cycles in A, E1 (pooled, 20 seeds) have no UWB fix; NOT TESTABLE if fewer than 100 UWB cycles pooled | the pilot's 70/76 |

## Falsifiers and reporting

A prediction that fails is reported as failed. No arm, offset, environment, seed or configuration is added or dropped after
the first cell. Single-seed differences are not reported (rule 8). An exact zero (e.g. two-tier tier-2 energy) is reported
only with the path shown reachable (the gate's REACH check). The two-tier result is a lower bound on tier 2's necessity and
the L1 result is optimistic about tier-1 ranging accuracy; both statements go in the report with the numbers.

## Abort conditions

- N1 fails; or a sidecar is not `dirty 0`; or more than one source; or a manifest mismatch before any cell;
- not 80 L1 cells / not 160 L2 cells, or a cell's `rc` not 0; a seed outside 361-380;
- a cell whose log lacks the preset lines (UWB mode 850k, ekf5 window smoother); a window solver failure;
- an arm with fewer than 20 seeds in any group;
- L2 degenerate: the two-tier arm never on UWB in an entire config x env group (reported, not hidden);
- memory pressure: drop to 10 jobs rather than let a cell be OOM-killed (an OOM-killed cell has no `rc` 0 and is rerun).
