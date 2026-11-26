# REALISTIC-14 — pre-registration (committed before the first cell)

**Why.** REALISTIC-11's H1 failed at CLUS3 and ROAD3F4. The cause found (DD 78): under every design with both LoRa radios the searchers polled for and steered on
868 MHz entries only, while 2.4 GHz-only searchers used their own radio (DD 30), so ours steered on entries tens of minutes old. DD 78 (`--finderDigestBothLora`)
lets a searcher with both radios ask on both. This repeats REALISTIC-11's emergency cells with DD 78 on for every design.

**Binary and design.** Frozen binary from this commit (SHA-256 before every cell, `dirty 0`), the DD 67 preset (so, unlike REALISTIC-11, `--tdoaHeightCorrect` is
on through the preset), REALISTIC-11's emergency cells and flags plus `--finderDigestBothLora=true`, every design. Seeds 2301-2320, never used. 180 runs:
ROAD1, ROAD3, CLUS3, RAND3, ROAD3F4, CREW10, CREW15, ROAD3MUL, CLUS3MUL, five designs each. Not run: the guarded-skip (TII2) cells (no hypothesis uses
them; C: is nearly full) and the quiet shift (no searchers, so DD 78 cannot act). `scripts/realistic14_run.sh`, `scripts/realistic14_analyse.py`, committed with
this file. Each run keeps the files the analysis reads (traces, node positions, summaries, logs).

**Seen before registering.** REALISTIC-11 in full. Development seeds 794-799 (DD 78): ours - 2.4 GHz only arrival CLUS3 +150 -> -4 s, ROAD3F4 +449 -> +375 s
(one scenario carries it), ROAD1 and RAND3 not clearly changed. ROAD3F4 may still fail H1.

## Predictions (fixed now)

- **H1-H3.** REALISTIC-6's rules, unchanged.
- **L1.** Every worker device of ours, every cell: mean and audited trailing-hour 868 MHz peak at most 2.5 %.
- **L2.** Every searcher's device of ours, every emergency cell: the same.
- **C1.** Every main emergency cell: ours, mean crew coverage at least 0.85.
- **T1, H4 (reported, no prediction).** Ours against the flat threshold; crew size and the field 2.4 GHz error law.

**Not covered.** Quiet shift, gateways' and 2.4 GHz/UWB airtime, collapse, any paired comparison with REALISTIC-11 (other seeds, other preset).

**Abort / completeness.** As REALISTIC-11, plus the DD78 line for every design in every log. No cell dropped or chosen. DD 78 enters the preset only if the owner
decides after this result. A Result section is appended here after the run, whatever it says.

## Result (run 2026-11-26 20:46-23:33 UTC; source 7967f93ca; 180/180 cells rc 0, dirty 0, problems 0)

Full tables: `results/realistic14_2026-11-27/REALISTIC14.md` (raw traces outside the repository, hashed in `RAW_MANIFEST.sha256`). "Ours" = `ca_tbl_full`.
Intervals are paired 95 % bootstrap over seeds.

- **H1: FAILS.** Ours minus 2.4 GHz only, searcher within 10 m: ROAD1 -1083 s [-1597, -595], ROAD3 +220 [-300, 790], CLUS3 -1 [-5, 3], RAND3 -1008 [-1514, -549],
  ROAD3F4 -110 [-505, 252]. No cell is clearly later (REALISTIC-11: CLUS3 +121 [56, 201], ROAD3F4 +542 [189, 973]); non-inferiority by 120 s is not shown at
  ROAD3 and ROAD3F4, whose intervals are wide. Clearly better at ROAD1 and RAND3.
- **H2: FAILS** (ROAD3 t25 +236 s [-464, 963], ROAD3F4 0 [-630, 630]: intervals too wide). Holds elsewhere.
- **H3: HOLDS.** A250 +0.211 to +0.406 over the better single radio, every lower bound at least 0.132.
- **L1, L2: HOLD.** No worker's or searcher's device above 2.5 % (mean or audited trailing-hour peak).
- **C1: HOLDS.** Coverage 0.898 (RAND3) to 0.941 (ROAD1).
- **T1 (reported):** against the flat threshold, ours is clearly later at ROAD3, +145 s [2, 416], and within 25 m 0.026 less often [0.001, 0.052]; elsewhere no
  clear difference. No benefit over the flat threshold.
- **H4 (reported):** crew of 10 and 15 and the field error law: arrival against 2.4 GHz only not clear in any (CREW10 +87, CREW15 +189, ROAD3MUL -62,
  CLUS3MUL +162 [-1, 486]); A250 +0.044 to +0.277.
- Not a paired comparison with REALISTIC-11 (other seeds, and the DD 67 preset adds the mast-height correction). DD 78 enters the preset only on the owner's decision.
