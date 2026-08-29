# `scripts/p10_plane.sh` — preserved as-run

**This file is byte-identical to the original.** The provenance lives here, in a
sidecar, rather than in a header, because a header would modify the file and the
requirement was that its `sha256` match before and after the copy. Those two cannot
both hold; byte-identity is the one that is verifiable, so it wins.

    sha256  8b3969d15cea1d3d081bf4ea4824636002b72a7161a636661f1ec9d7c07691e1

## What it is

The campaign runner that produced `results/p10_plane_fix`, copied verbatim from
`~/p10_plane.sh` on 2026-08-29.

## Why it is here

It is cited by `report/REPORT_DATA_PACK.md` and lived **outside the repository**, so
a fresh clone could not reproduce a campaign its own documents cite. That is the
exact failure the completeness requirement exists to prevent, and it is the same
class as the four analysis scripts G4 caught — a script that produced published
results existing on one machine only.

## What was NOT done to it

It is preserved exactly as it ran, **not** rewritten to the repository's conventions.
Its value is that it is the script that produced the results; adapting it would
destroy the thing being preserved.

**LIMITATION:** it was invoked from `~/` and its relative paths assume that working
directory. Run it from where it was run, or adapt a *copy*.
