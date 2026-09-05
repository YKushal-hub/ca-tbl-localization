#!/usr/bin/env python3
"""A2 stage 2 — the CAL-05 detection curve against bias magnitude.

Stage 1 established that availability cannot resolve the treatment (mean per-seed
difference +0.00000 on all four F1 cells, no non-zero seeds at all) while detection
could (precision 0.84-0.98, recall 0.29-0.72). So the curve is the deliverable.

Reads 40 m from results/c9_escalate (policy bounded, already run, not re-run) and
{10,20,80,160} from results/c9_stage2, and reports precision and recall per magnitude
per fault model.

DENOMINATORS, restated because a rate without them is not an observable (rule 1).
The fault window is contiguous and seed-derived:
    biasStartS = durationS * (0.25 + 0.5 * (seed % 7) / 7)
    injected   = biasStartS <= t < biasStartS + 0.2 * durationS
720 s at duration 3600, 288 cycles per seed, 5760 injected cycles per cell over 20
seeds. Placed mid-run so it never straddles EKF convergence and never runs past the
end; truncation is checked per cell rather than assumed.

Recall is reported over injected cycles where the gate COULD fire (hadFix == 1). With
no fix there is no fix to test against a prior, so counting those as misses would
measure coverage rather than detection. The all-cycles denominator is reported beside
it; in stage 1 the two were identical.
"""
import csv
import math
import os
import sys

DURATION = 3600.0
FRACTION = 0.2
SEEDS = range(1, 21)


def bias_start(seed):
    return DURATION * (0.25 + 0.5 * float(seed % 7) / 7.0)


def detect(trace, seed):
    lo = bias_start(seed)
    hi = lo + FRACTION * DURATION
    tp = fp = fn = 0
    inj = inje = 0
    trunc = hi > DURATION
    with open(trace) as fh:
        for row in csv.DictReader(fh):
            t = float(row["t"])
            injected = (t >= lo) and (t < hi)
            susp = row["suspect"].strip() in ("1", "true", "True")
            hadfix = row["hadFix"].strip() in ("1", "true", "True")
            if injected:
                inj += 1
                if hadfix:
                    inje += 1
            if susp and injected:
                tp += 1
            elif susp and not injected:
                fp += 1
            elif (not susp) and injected and hadfix:
                fn += 1
    return tp, fp, fn, inj, inje, trunc


def cell(paths):
    TP = FP = FN = INJ = INJE = 0
    n = 0
    trunc_any = False
    for d, s in paths:
        tr = os.path.join(d, "trace_ca_tbl_full_seed%d.csv" % s)
        if not os.path.isfile(tr):
            continue
        tp, fp, fn, i, ie, tc = detect(tr, s)
        TP += tp; FP += fp; FN += fn; INJ += i; INJE += ie
        trunc_any = trunc_any or tc
        n += 1
    return TP, FP, FN, INJ, INJE, n, trunc_any


print("CAL-05 — detection curve against injected bias magnitude")
print("policy = bounded, arm = ca_tbl_full, 20 seeds per point")
print("=" * 88)
print("%6s %4s %10s %10s %10s %8s %8s %8s"
      % ("bias", "flt", "precision", "recall_ev", "recall_all", "TP", "FP", "FN"))

rows = []
for fault in ("F1", "F3"):
    for bias in (10, 20, 40, 80, 160):
        if bias == 40:
            # already run in stage 1 at policy bounded
            paths = [(os.path.join(os.environ["HOME"]+"/projects/ca-tbl-localization/results/c9_escalate", "bounded_%s_seed%d" % (fault, s)), s)
                     for s in SEEDS]
        else:
            paths = [(os.path.join("results/c9_stage2", "b%d_%s_seed%d" % (bias, fault, s)), s)
                     for s in SEEDS]
        TP, FP, FN, INJ, INJE, n, tc = cell(paths)
        if n == 0:
            print("%6d %4s   NO DATA" % (bias, fault))
            continue
        prec = TP / (TP + FP) if (TP + FP) else float("nan")
        rec_ev = TP / (TP + FN) if (TP + FN) else float("nan")
        rec_all = TP / INJ if INJ else float("nan")
        rows.append((fault, bias, prec, rec_ev))
        print("%6d %4s %10.4f %10.4f %10.4f %8d %8d %8d"
              % (bias, fault, prec, rec_ev, rec_all, TP, FP, FN))
        if tc:
            print("       WARNING: a fault window ran past the end of its run")
    print()

print("=" * 88)
print("SHAPE OF THE CURVE")
for fault in ("F1", "F3"):
    pts = [(b, p, r) for (f, b, p, r) in rows if f == fault]
    if len(pts) < 2:
        continue
    r0 = pts[0][2]
    r1 = pts[-1][2]
    print("  %s: recall %.4f at %d m -> %.4f at %d m   (%s)"
          % (fault, r0, pts[0][0], r1, pts[-1][0],
             "rises with magnitude" if r1 > r0 else "does NOT rise with magnitude"))
    print("      precision %.4f -> %.4f" % (pts[0][1], pts[-1][1]))

print("""
READING IT. A Mahalanobis gate at chi2(0.99) = 9.21 should find a LARGER bias more
easily: a bigger displacement is further from the prior. If recall does not rise with
magnitude, the gate is not limited by the threshold but by something upstream -- the
all-LOS precondition, or the prior it tests against, which is a constant-velocity
prior propagated from previous radio fixes rather than the independent prior the
specification requires.

This is the axis the registered decision rule needs. It does NOT on its own decide
claim 2: the rule is about the availability difference, and stage 1 already showed
that metric cannot resolve the treatment.""")
