# REALISTIC-9 — pre-registration (committed before the first cell)

**Why.** REALISTIC-8's L1 held as registered but its peak missed carried range bytes; the audit found 218 of 1200 devices above 2.5 % in some hour. DD 56 now counts
those bytes in the ledger (`--dutyCountCarry`, default true) and reports a peak sampled after every exchange, carried byte and MAC frame.

**Binary and design.** As REALISTIC-8 (frozen binary, SHA-256 before every cell, `dirty 0`), same cells and flags, with the corrected ledger. Seeds 1201-1220 (emergency)
and 1221-1230 (quiet shift), never used. 360 runs. `scripts/realistic9_run.sh`, `scripts/realistic9_analyse.py`, committed with this file.

**Seen before registering.** REALISTIC-8 in full (other seeds): H1 and H2 failed, so they may fail again. Development seeds 721-722: largest audited peak 2.488 %.

## Predictions (fixed now)

- **H1-H3.** REALISTIC-6's rules, unchanged.
- **L1.** Every worker device of ours in every cell, the quiet shift included: mean and audited trailing-hour peak at most 2.5 %.
- **C1.** Every main emergency cell: ours, mean crew coverage at least 0.85.
- **T1, Q1 (reported, no prediction).**

**Not covered.** Searchers' devices, gateways, 2.4 GHz and UWB airtime.

**Abort / completeness.** As REALISTIC-8, plus: a missing device line. No cell dropped or chosen. Nothing is adopted as a default by this campaign.
A Result section is appended here after the run, whatever it says.

## Result (run 2026-11-24 21:07-23:31 IST; source cd4b69b48; 360/360 cells rc 0, dirty 0, problems 0)

Full tables: `results/realistic9_2026-11-24/REALISTIC9.md`. "Ours" = `ca_tbl_full`. Intervals are paired 95 % bootstrap over seeds.

- **H1: FAILS.** Ours minus 2.4 GHz only, searcher within 10 m: ROAD1 -524 s [-915, -190], ROAD3 +224 [-25, 594], CLUS3 +259 [65, 522], RAND3 -770 [-1199, -386],
  ROAD3F4 +832 [403, 1321]. RAND3 passes the clearly-better rule (upper bound below -300 s). Non-inferiority (upper bound below +120 s) fails at ROAD3, CLUS3 and
  ROAD3F4; at CLUS3 and ROAD3F4 ours is later and the interval excludes zero. At ROAD3F4 ours never arrives in 50 % of runs, 2.4 GHz only in 15 %.
- **Not repeated from REALISTIC-8:** ROAD3 (-393 [-780, -79] there) and ROAD3F4 (+22 [-254, 245] there). CLUS3 later, ROAD1 and RAND3 earlier in both. The seeds
  differ and so does the ledger; the runs do not separate the two causes.
- **H2: FAILS** at ROAD3 (t25 +210 s [0, 630]) and ROAD3F4 (+393 [-54, 1050]); holds elsewhere.
- **H3: HOLDS.** A250 +0.178 to +0.382, every lower bound at least 0.118.
- **L1: HOLDS.** 0 of 1400 devices (1200 emergency, 200 quiet shift) above 2.5 %, mean or audited peak; largest 2.489 %. The peak covers ranging, MAC frames and carried bytes.
  Searchers' devices are not covered.
- **C1: HOLDS.** Coverage 0.886 (RAND3) to 0.947 (ROAD1), nominal 0.95.
- **T1 (reported):** arrival and t25 differ from the flat threshold in no main cell (every interval includes zero); the trigger has no demonstrated benefit over it.
- **Q1 (reported):** quiet-shift coverage ROAD1 0.999, ROAD3 0.831, **RAND3 0.745, CLUS3 0.677**: over-confidence remains. A250 over the better single radio +0.174 to +0.400
  with three gateways, -0.000 with one.
- **H4 (reported):** A250 gain +0.281 (crew of 10), +0.285 (15), +0.231 (ROAD3MUL), +0.050 (CLUS3MUL). Arrival: CLUS3MUL -361 s [-719, -76]; CREW10, CREW15 and ROAD3MUL
  include zero.
