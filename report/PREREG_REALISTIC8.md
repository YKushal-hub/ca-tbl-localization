# REALISTIC-8 — pre-registration (committed before the first cell)

**Why.** REALISTIC-7 fixed two-range fixes. Two problems remained. (1) A device's true 868 MHz airtime, ranging plus every sent MAC frame, was
3.6-4.8 % on development runs against the legal 2.5 %, because two governors each allowed 2.5 %. (2) The crew's 95 % region still covered too
little at the office (0.77), because peer ranges were fused as independent. DD 56 (`--sharedDuty868`) and DD 57 (`--peerSplitCi`) address them.

**Binary.** Frozen at launch by `scripts/realistic8_run.sh` (SHA-256 checked before every cell); source = the commit adding this file;
`dirty 0` on every cell.

**Design.** Seeds 651-670 (emergency) and 671-680 (8 h quiet shift), never used. REALISTIC-6's cells, every design with DD 54, DD 56 (ranging share 0.5)
and DD 57 on. 400 runs. Measures and statistics: `scripts/realistic8_analyse.py`, committed with this file.

**Seen before registering (development seeds 721-724, four seeds, not evidence).** Ours with all three on: no device above 2.5 %; coverage 0.88-0.92; crew A(25 m)
0.34 on the three-gateway road (0.39 without the cap), 0.40 at random gateways (0.59), 0.70 at the office (0.77); searcher arrival 2845 s on the three-gateway
road against 2216 s for 2.4 GHz only.

## Predictions (fixed now)

- **H1-H3.** REALISTIC-6's rules, every design with the three mechanisms on. On the evidence above H1 and H2 may fail.
- **L1.** Every worker device of ours, in every main, crew and field-error cell: mean and trailing-hour peak 868 MHz duty cycle at most 2.5 %.
- **C1.** Every main cell: ours, mean crew coverage at least 0.85.
- **T1, Q1 (reported, no prediction).**

**Not covered.** Searchers' devices, gateways and 2.4 GHz and UWB airtime.

**Abort / completeness.** Any rc not 0, not `dirty 0`, a manifest mismatch, a missing DD54/56/57 line, an alarm not at 3000 s, or fewer than the registered
seeds in a cell: incomplete, nothing is evaluated. No cell dropped or chosen. Nothing is adopted as a default by this campaign.

**Reporting.** `REALISTIC8.md` by the analyser; a Result section is appended here after the run, whatever it says.

## Result (run 2026-11-24 15:54-18:14 IST; source 75dbde3eb; 360/360 cells rc 0, dirty 0, problems 0)

Full tables: `results/realistic8_2026-11-24/REALISTIC8.md`. The text above says 400 runs and 3.6-4.8 %: the committed runner has 14 x 20 + 8 x 10 = 360
cells (a counting slip), and the development range is 2.59-4.77 % over 80 device-runs (the first figure came from one run). No prediction depends on either.

- **H1: FAILS.** Ours minus 2.4 GHz only, searcher within 10 m: ROAD1 -633 s [-1134, -156], ROAD3 -393 [-780, -79], RAND3 -1062 [-1589, -524], ROAD3F4
  +22 [-254, 245], CLUS3 +170 [18, 383]. Only RAND3 passes the clearly-better rule (upper bound below
  -300 s); the office is later than 2.4 GHz only.
- **H2: FAILS** at ROAD3F4 (+92 s [-89, 365]); holds elsewhere.
- **H3: HOLDS.** A250 +0.165 to +0.470, every lower bound above 0.12.
- **L1: HOLDS as registered (0 of 1200 devices), but not in substance.** The registered peak missed carried range bytes and was sampled after ranging only.
  The audit replay (DD 56, correction) puts 218 of 1200 devices above 2.5 % in some hour, largest 2.64 %; by cell: CREW15 134 of 300, CREW10 67 of 200,
  ROAD3MUL 6, ROAD3 5, ROAD1 4, ROAD3F4 2, and 0 at CLUS3, CLUS3MUL and RAND3; quiet shift (outside L1) 69 of 200. **Do not quote REALISTIC-8 as legal.** REALISTIC-9 repeats it
  with the bytes in the ledger.
- **C1: HOLDS.** Coverage 0.887 (ROAD3F4) to 0.924 (CLUS3), nominal 0.95, so still slightly over-confident.
- **T1 (reported):** ours equals the flat threshold everywhere except CLUS3 arrival (+122 s [1, 320], ours later).
- **Q1 (reported):** quiet-shift coverage ROAD1 0.998, ROAD3 0.880, **RAND3 0.765, CLUS3 0.737**: over-confidence remains there. A250 over the better single radio: +0.189 to
  +0.473 with three gateways, -0.001 with one.
