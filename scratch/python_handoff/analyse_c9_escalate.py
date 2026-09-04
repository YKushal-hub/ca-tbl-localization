#!/usr/bin/env python3
"""A2 stage 1 — detect-and-escalate under injected fault. Applies
report/PREREG_C9_ESCALATE.md verbatim, including where it returns no verdict.

TWO OUTPUTS.

1. Availability difference ca_tbl_full - ca_tbl_no_integrity, as the MEAN OF
   PER-SEED DIFFERENCES over 20 seeds with a sign test beside it, F1 and F3
   reported SEPARATELY. Paired per seed because the two arms share a channel
   realisation at a given seed; a single-seed difference is never a result
   (CLAUDE.md rule 8), so only the 20-seed aggregate is reported.

2. CAL-05: detection precision and recall against the injected ground truth.
   The thesis register defers this to Phase 2.1 and the project has never
   produced it.

DENOMINATORS, STATED (CLAUDE.md rule 1). The fault window is contiguous and
seed-derived:

    biasStartS = durationS * (0.25 + 0.5 * (seed % 7) / 7)
    injected   = biasStartS <= t < biasStartS + injectBiasFraction * durationS

so at fraction 0.2 and duration 3600 it is a 720 s window starting mid-run. A
cycle is INJECTED if its trace timestamp falls in that window.

Recall is reported against TWO denominators because they answer different
questions and the difference is the interesting part:
  - recall_all       over every injected cycle
  - recall_evaluable over injected cycles where the gate could fire at all,
                     i.e. hadFix == 1. The integrity check tests a fix against a
                     prior; with no fix there is nothing to test, so counting
                     those as misses measures coverage, not detection.
Truncation: the run ends at durationS and the window is placed to close before
it, so no window is truncated. Verified rather than assumed -- any cell where the
window would run past the end is reported.
"""
import csv
import glob
import math
import os
import sys

ROOT = "results/c9_escalate"
DURATION = 3600.0
FRACTION = 0.2
SEEDS = range(1, 21)
POLICIES = ["none", "inflate", "bounded", "reinit"]
FAULTS = ["F1", "F3"]


def bias_start(seed):
    return DURATION * (0.25 + 0.5 * float(seed % 7) / 7.0)


def sign_test(diffs):
    """Two-sided sign test. Ties are dropped, which is the conservative choice."""
    pos = sum(1 for d in diffs if d > 0)
    neg = sum(1 for d in diffs if d < 0)
    n = pos + neg
    if n == 0:
        return pos, neg, 1.0
    k = min(pos, neg)
    tail = sum(math.comb(n, i) for i in range(0, k + 1))
    p = min(1.0, 2.0 * tail / (2.0 ** n))
    return pos, neg, p


def availability(path):
    """A = fixCycles / steps, from the summary. Architecture.md 1.3: every
    decision cycle counts and a cycle with no fix counts against."""
    with open(path) as fh:
        out = {}
        for row in csv.DictReader(fh):
            steps = float(row["steps"])
            if steps > 0:
                out[row["arm"]] = (float(row["fixCycles"]) / steps,
                                   float(row["cep50M"]), float(row["totalJ"]))
    return out


def detection(trace, seed):
    """TP/FP/FN against the injected window."""
    lo = bias_start(seed)
    hi = lo + FRACTION * DURATION
    tp = fp = fn = 0
    inj_all = inj_eval = 0
    truncated = hi > DURATION
    with open(trace) as fh:
        for row in csv.DictReader(fh):
            t = float(row["t"])
            injected = (t >= lo) and (t < hi)
            susp = row["suspect"].strip() in ("1", "true", "True")
            hadfix = row["hadFix"].strip() in ("1", "true", "True")
            if injected:
                inj_all += 1
                if hadfix:
                    inj_eval += 1
            if susp and injected:
                tp += 1
            elif susp and not injected:
                fp += 1
            elif (not susp) and injected and hadfix:
                fn += 1
    return tp, fp, fn, inj_all, inj_eval, truncated


if not os.path.isdir(ROOT):
    sys.exit("missing %s" % ROOT)

print("A2 STAGE 1 — detect-and-escalate under injection, one bias magnitude (40 m)")
print("Estimator: mean of per-seed differences, 20 seeds, sign test beside it.")
print("=" * 78)

cells_positive = 0
cells_negative = 0
any_truncated = False

for fault in FAULTS:
    for pol in POLICIES:
        diffs, cep_diffs = [], []
        TP = FP = FN = 0
        INJ = INJE = 0
        for s in SEEDS:
            d = os.path.join(ROOT, "%s_%s_seed%d" % (pol, fault, s))
            f = os.path.join(d, "arm_summary.csv")
            if not os.path.isfile(f):
                continue
            a = availability(f)
            if "ca_tbl_full" not in a or "ca_tbl_no_integrity" not in a:
                continue
            diffs.append(a["ca_tbl_full"][0] - a["ca_tbl_no_integrity"][0])
            cep_diffs.append(a["ca_tbl_full"][1] - a["ca_tbl_no_integrity"][1])
            tr = os.path.join(d, "trace_ca_tbl_full_seed%d.csv" % s)
            if os.path.isfile(tr):
                tp, fp, fn, ia, ie, trunc = detection(tr, s)
                TP += tp; FP += fp; FN += fn; INJ += ia; INJE += ie
                any_truncated = any_truncated or trunc

        if not diffs:
            print("\n%-8s %s   NO DATA" % (pol, fault))
            continue

        mean = sum(diffs) / len(diffs)
        cepm = sum(cep_diffs) / len(cep_diffs)
        pos, neg, p = sign_test(diffs)
        prec = TP / (TP + FP) if (TP + FP) else float("nan")
        rec_all = TP / INJ if INJ else float("nan")
        rec_ev = TP / (TP + FN) if (TP + FN) else float("nan")

        if mean > 0:
            cells_positive += 1
        elif mean < 0:
            cells_negative += 1

        print("\n%-8s %s   n=%d seeds" % (pol, fault, len(diffs)))
        print("   availability  full - no_integrity : mean %+0.5f   sign %d+/%d-  p=%.4f"
              % (mean, pos, neg, p))
        print("   cep50M        full - no_integrity : mean %+0.3f m" % cepm)
        print("   CAL-05  precision %.4f   recall_all %.4f   recall_evaluable %.4f"
              % (prec, rec_all, rec_ev))
        print("           TP=%d FP=%d FN=%d   injected cycles=%d (evaluable %d)"
              % (TP, FP, FN, INJ, INJE))

print("\n" + "=" * 78)
print("cells with mean > 0: %d/8      cells with mean < 0: %d/8"
      % (cells_positive, cells_negative))
if any_truncated:
    print("WARNING: at least one fault window ran past the end of its run.")
print("""
REGISTERED DECISION RULE, applied verbatim:
  PASS needs mean > 0 with the sign test in >= 6 of 8 cells AND the direction
       holding at >= 2 bias magnitudes.
  FAIL needs mean < 0 in >= 6 of 8 cells at MORE THAN ONE bias magnitude.
Stage 1 is ONE magnitude (40 m). It therefore cannot return PASS or FAIL on its
own -- both outcomes require a second magnitude, which is stage 2. Whatever the
counts above, the stage-1 verdict is INCONCLUSIVE BY CONSTRUCTION, and the
direction here only decides which magnitudes stage 2 should spend compute on.
Reporting stage 1 as a verdict would be reading the rule backwards.""")
