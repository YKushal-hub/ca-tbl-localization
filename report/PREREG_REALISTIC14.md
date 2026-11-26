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
