#!/usr/bin/env python3
"""G3 null ablation: does ca_tbl_full differ from ca_tbl_no_integrity, and WHY.

SCOPE -- READ THIS BEFORE TRUSTING A GREEN G3
=============================================
This gate tests the CAUSAL ORDERING of the ablation. It does NOT test RNG
determinism. Determinism is gated by det_ byte-identity (G2 in catbl_gates.sh,
which runs the same seed twice and diffs), and by the stored det_v6a/det_v6b
pair.

The trade is explicit and this gate is WEAKER than the one it replaces in one
direction: ordering accepts any divergence that begins at or after the first
rejection, so RNG contamination starting at or after that row is invisible here.
The old any-difference diff would have caught it.

That is acceptable only because the coverage MOVED rather than vanished. G2
compares two runs of one binary; today two independently rebuilt binaries also
produced byte-identical traces across all six arms. Contamination is what those
tests are for. G3 never should have been carrying that job -- it was doing two
jobs and doing both badly:

  as an ordering test   it failed on legitimate rejection cascades, and its
                        comment asserted "RNG contamination has returned", a
                        cause it never tested for
  as a determinism test it compared 37 of 42 columns (`cut -f1,3-15,17-37`), so
                        estXhp, estYhp, meanSigmaM, meanSigmaRawM and sigmaGeoM
                        were never compared at all -- and three of those were
                        added during the project, each one silently widening the
                        blind spot

If you want "did these two arms differ AT ALL", that question belongs in a
determinism gate on identical configurations, not here.


The two arms run the same seed with the same RNG stream. The only intended
difference is the integrity check, so any divergence has exactly two possible
causes and they need opposite verdicts:

  RNG CONTAMINATION -- the arms have desynchronised and the comparison between
      them is meaningless. Every claim built on an arm-vs-arm difference is void.
      This is what G3 was built to catch and it must fail the gate.

  AN INTEGRITY REJECTION -- the check fired, the estimator took a different
      update, and every later row inherits that. This is the mechanism working.
      Failing the gate for it trains the reader to ignore G3.

The old gate could not tell them apart. It diffed the two traces and failed on
any difference, with a comment asserting that a failure "means RNG contamination
has returned" -- a cause it never actually tested for. When the check began
firing (mahalUseR defaults to false, so the legacy over-confident S produces
genuine outliers) G3 went red and stayed red, and the only remedy on offer was
CATBL_SKIP_G3=1: a waiver that suppresses the contamination detector too.

THE DISCRIMINATOR IS CAUSAL ORDER, not which columns moved. A rejection cannot
explain a divergence that predates it. So:

    divergence with no rejection anywhere            -> FAIL
    first divergence STRICTLY BEFORE first rejection -> FAIL
    first divergence AT OR AFTER first rejection     -> PASS, and say so

Column classification was the obvious alternative and it is weaker: tier
selection and anchor counts look like RNG-driven inputs but are downstream of
policy decisions, so a contaminated run could hide in "expected" columns.
Ordering needs no such judgement call.

Columns are matched BY NAME. The previous gate hardcoded `cut -f1,3-15,17-37`,
which silently shifts if a column is ever added -- and columns were added twice
during this project.
"""
import csv
import sys

IGNORED = ("arm", "mahalanobis")  # differ by construction between these arms


def load(p):
    with open(p) as f:
        rows = list(csv.reader(f))
    if not rows:
        sys.exit(f"FATAL: {p} is empty")
    return rows[0], rows[1:]


def main():
    if len(sys.argv) < 3:
        sys.exit("usage: catbl_g3_ablation.py <full.csv> <no_integrity.csv>")
    hf, full = load(sys.argv[1])
    hn, noint = load(sys.argv[2])
    if hf != hn:
        sys.exit("FATAL: the two arms have different columns -- not comparable")

    for c in IGNORED + ("suspect", "rejected"):
        if c not in hf:
            sys.exit(f"FATAL: column '{c}' absent; the gate cannot run")
    ign = {hf.index(c) for c in IGNORED}
    si, ri = hf.index("suspect"), hf.index("rejected")

    if len(full) != len(noint):
        print(f"G3 FAIL: row counts differ ({len(full)} vs {len(noint)}). The arms "
              f"did not even step together; this is desynchronisation, not a "
              f"rejection cascade.")
        return 1

    first_div = first_rej = None
    ndiv = 0
    for i, (a, b) in enumerate(zip(full, noint)):
        if any(x != y for j, (x, y) in enumerate(zip(a, b)) if j not in ign):
            ndiv += 1
            if first_div is None:
                first_div = i
        if first_rej is None and (a[si] != b[si] or a[ri] != b[ri]):
            first_rej = i

    if first_div is None:
        print(f"G3 PASS: the arms are identical over {len(full)} rows "
              f"(ignoring {', '.join(IGNORED)}). The integrity check never fired.")
        return 0

    tdiv = full[first_div][0]
    if first_rej is None:
        print(f"G3 FAIL: {ndiv} rows diverge, the first at t={tdiv}, but suspect "
              f"and rejected are identical in every row -- so no integrity "
              f"rejection can account for it. This is the RNG contamination "
              f"signature: the arms differ without the mechanism that is supposed "
              f"to be the only difference.")
        return 1

    trej = full[first_rej][0]
    if first_div < first_rej:
        print(f"G3 FAIL: divergence begins at t={tdiv} but the first integrity "
              f"rejection is later, at t={trej}. A rejection cannot cause a "
              f"difference that precedes it, so something else did.")
        return 1

    print(f"G3 PASS: {ndiv} of {len(full)} rows diverge, beginning at t={tdiv}, "
          f"which is exactly where the first integrity rejection occurs "
          f"(t={trej}). The divergence is the rejection propagating through the "
          f"estimator -- the mechanism under test, not contamination. No row "
          f"before t={trej} differs.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
